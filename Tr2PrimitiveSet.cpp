#include "StdAfx.h"
#include "Tr2PrimitiveSet.h"
#include "TriSettingsRegistrar.h"
#include "TriRenderBatch.h"
#include "Tr2Effect.h"
#include "Tr2PerObjectData.h"

struct PerObjectVSData
{
	Matrix WorldMat;
};

struct PerObjectPSData
{
	Matrix WorldMat;
};

// A global multiplier for allowing artists or other to scale all the 
// fixed scale primitives on screen.
float g_primitiveDistanceScaleMultiplier = 1.0f/7.0f;
TRI_REGISTER_SETTING( "primitiveDistanceScaleMultiplier", g_primitiveDistanceScaleMultiplier );

Tr2PrimitiveSet::Tr2PrimitiveSet( IRoot* lockobj ):
	m_scaleByDistanceToView( false ),
	m_scale( 1.0f ),
#if BLUE_WITH_PYTHON
	m_pythonUserData( NULL ),
#endif
	m_viewOriented( false ),
	m_isDrawingForPicking( false ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
{	
	D3DXMatrixIdentity( &m_localTransform );	
	D3DXMatrixIdentity( &m_worldTransform );
	m_boundingSphere = Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
	m_color = Color( 0.5f, 0.5f, 0.5f, 1.0f );
}

Tr2PrimitiveSet::~Tr2PrimitiveSet()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2Renderable
bool Tr2PrimitiveSet::HasTransparentBatches()
{
	return true;
}

void Tr2PrimitiveSet::GetBatches( ITriRenderBatchAccumulator* accumulator, 
							 TriBatchType batchType,
							 const Tr2PerObjectData* perObjectData )
{
	m_isDrawingForPicking = false;
	// Is only rendered as transparent or additive.
	if( batchType == TRIBATCHTYPE_OPAQUE && m_effect )
	{
		GetBatchesImpl( accumulator, perObjectData, m_effect );
	}
	else if( batchType == TRIBATCHTYPE_PICKING )
	{// we might be drawing for picking but the primitive might not have a picking effect
		m_isDrawingForPicking = true; 
		if( m_pickEffect )
		{
			GetBatchesImpl( accumulator, perObjectData, m_pickEffect );
		}		
	}
}

void Tr2PrimitiveSet::GetBatchesImpl( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData, Tr2Effect* effect )
{

	TriForwardingBatch* batch = accumulator->Allocate<TriForwardingBatch>();
	if( batch )
	{
		batch->SetPerObjectData( perObjectData );
		batch->SetShaderMaterial( effect );
		batch->SetGeometryProvider( this );

		accumulator->Commit( batch );
	}
}

float Tr2PrimitiveSet::GetSortValue()
{/*
	Primitives are sorted by the distance to the bounding sphere
 */
	Vector4 bound = GetBoundingSphere();
	Vector3 d = Tr2Renderer::GetViewPosition() - Vector3(bound.x, bound.y, bound.z);
	float distance = D3DXVec3Length( &d ) - bound.w;
	return distance;
}

Vector4 Tr2PrimitiveSet::GetBoundingSphere( void )
{/*
	The primitives bounding sphere needs to be scaled along with the model
 */
	Vector4 boundingCenter;
	Vector3 bs( m_boundingSphere.x, m_boundingSphere.y, m_boundingSphere.z );
	D3DXVec3TransformCoord((Vector3*)&boundingCenter, &bs, &m_worldTransform );
	boundingCenter.w = (m_boundingSphere.w*m_scale);
	return boundingCenter;
}

Tr2PerObjectData* Tr2PrimitiveSet::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	Tr2PerObjectDataStandard* data = accumulator->Allocate<Tr2PerObjectDataStandard>();

	if( !data )
	{
		return nullptr;
	}

	PerObjectPSData perObjectPSBuffer;
	PerObjectVSData perObjectVSBuffer;

	// column_major for shaders
	D3DXMatrixTranspose( &perObjectVSBuffer.WorldMat, &m_worldTransform );	
	D3DXMatrixTranspose( &perObjectPSBuffer.WorldMat, &m_worldTransform );

	data->CopyToVSFloatBuffer( perObjectVSBuffer );
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	return data;
}

void Tr2PrimitiveSet::UpdateTransform( void )
{
	Matrix finalTransform;
	Matrix scale_mat;
	Matrix view = Tr2Renderer::GetViewTransform();
	D3DXMatrixIdentity( &scale_mat );

	if( m_scaleByDistanceToView )
	{
		// How much to scale the lineset based on the distance to the eye position
		Vector3 viewPos = Tr2Renderer::GetViewPosition();
		Vector3 lineSetPos = Vector3( m_localTransform._41, m_localTransform._42, m_localTransform._43 );
		Vector3 normal = Vector3( view._13, view._23, view._33 );
		Vector3 dir(viewPos - lineSetPos);
		m_scale = fabs(D3DXVec3Dot( &dir, &normal )*g_primitiveDistanceScaleMultiplier*Tr2Renderer::GetFieldOfView());
		D3DXMatrixScaling( &scale_mat, m_scale, m_scale, m_scale );
		D3DXMatrixMultiply( &finalTransform, &scale_mat, &m_localTransform );
	}
	else
	{
		m_scale = 1.0f;
	}

	if ( m_viewOriented )
	{
		// The primitives need to be oriented in such a way
		// as you were drawing on a x,y plane
		Matrix rotation;
		D3DXMatrixIdentity( &rotation );
		rotation._11 = view._11;
		rotation._12 = view._21;
		rotation._13 = view._31;
		rotation._21 = view._12;
		rotation._22 = view._22;
		rotation._23 = view._32;
		rotation._31 = view._13;
		rotation._32 = view._23;
		rotation._33 = view._33;

		// Need to scale the matrix before we translate it...
		D3DXMatrixMultiply( &finalTransform, &scale_mat, &rotation );

		finalTransform._41 = m_localTransform._41;
		finalTransform._42 = m_localTransform._42;
		finalTransform._43 = m_localTransform._43;
	}
	else
	{
		if( m_scaleByDistanceToView )
		{
			D3DXMatrixMultiply( &finalTransform, &scale_mat, &m_localTransform );
		}
		else
		{
			finalTransform = m_localTransform;
		}
	}

	m_worldTransform = finalTransform;
}

/////////////////////////////////////////////////////////////////////////////
// INotify
bool Tr2PrimitiveSet::OnModified( Be::Var * value )
{
	// SetCurrentColor is overwritten by any class that inherits
	if( IsMatch( value, m_color ) )
	{
		SetCurrentColor( m_color );
	}
	return true;
}