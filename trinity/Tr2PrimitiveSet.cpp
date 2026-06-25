// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2PrimitiveSet.h"
#include "TriSettingsRegistrar.h"
#include "TriRenderBatch.h"
#include "Shader/Tr2Effect.h"
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
float g_primitiveDistanceScaleMultiplier = 1.0f / 7.0f;
TRI_REGISTER_SETTING( "primitiveDistanceScaleMultiplier", g_primitiveDistanceScaleMultiplier );

Tr2PrimitiveSet::Tr2PrimitiveSet( IRoot* lockobj ) :
	m_scaleByDistanceToView( false ),
	m_scale( 1.0f ),
#if BLUE_WITH_PYTHON
	m_pythonUserData( NULL ),
#endif
	m_viewOriented( false ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_localTransform( IdentityMatrix() ),
	m_worldTransform( IdentityMatrix() ),
	m_boundingSphere( Vector4( 0.0f, 0.0f, 0.0f, 0.0f ) ),
	m_color( Color( 0.5f, 0.5f, 0.5f, 1.0f ) )
{
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
								  const Tr2PerObjectData* perObjectData,
								  Tr2RenderReason reason )
{
	// Is only rendered as transparent or additive.
	if( batchType == TRIBATCHTYPE_OPAQUE && m_effect )
	{
		GetBatchesImpl( accumulator, perObjectData, m_effect, GetBatchesReason::Draw );
	}
	else if( batchType == TRIBATCHTYPE_PICKING && m_pickEffect )
	{
		GetBatchesImpl( accumulator, perObjectData, m_pickEffect, GetBatchesReason::Picking );
	}
}

float Tr2PrimitiveSet::GetSortValue()
{ /*
	Primitives are sorted by the distance to the bounding sphere
 */
	Vector4 bound = GetBoundingSphere();
	Vector3 d = Tr2Renderer::GetViewPosition() - Vector3( bound.x, bound.y, bound.z );
	float distance = Length( d ) - bound.w;
	return distance;
}

Vector4 Tr2PrimitiveSet::GetBoundingSphere( void )
{ /*
	The primitives bounding sphere needs to be scaled along with the model
 */
	Vector4 boundingCenter;
	boundingCenter.GetXYZ() = TransformCoord( m_boundingSphere.GetXYZ(), m_worldTransform );
	boundingCenter.w = ( m_boundingSphere.w * m_scale );
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
	perObjectVSBuffer.WorldMat = Transpose( m_worldTransform );
	perObjectPSBuffer.WorldMat = Transpose( m_worldTransform );

	data->CopyToVSFloatBuffer( perObjectVSBuffer );
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	return data;
}

void Tr2PrimitiveSet::UpdateTransform( void )
{
	Matrix finalTransform;
	Matrix scale_mat = IdentityMatrix();
	Matrix view = Tr2Renderer::GetViewTransform();

	if( m_scaleByDistanceToView )
	{
		// How much to scale the lineset based on the distance to the eye position
		Vector3 viewPos = Tr2Renderer::GetViewPosition();
		Vector3 lineSetPos = Vector3( m_localTransform._41, m_localTransform._42, m_localTransform._43 );
		Vector3 normal = Vector3( view._13, view._23, view._33 );
		Vector3 dir( viewPos - lineSetPos );
		m_scale = fabs( Dot( dir, normal ) * g_primitiveDistanceScaleMultiplier * Tr2Renderer::GetFieldOfView() );
		scale_mat = ScalingMatrix( m_scale, m_scale, m_scale );
		finalTransform = scale_mat * m_localTransform;
	}
	else
	{
		m_scale = 1.0f;
	}

	if( m_viewOriented )
	{
		// The primitives need to be oriented in such a way
		// as you were drawing on a x,y plane
		Matrix rotation = IdentityMatrix();
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
		finalTransform = scale_mat * rotation;

		finalTransform._41 = m_localTransform._41;
		finalTransform._42 = m_localTransform._42;
		finalTransform._43 = m_localTransform._43;
	}
	else
	{
		if( m_scaleByDistanceToView )
		{
			finalTransform = scale_mat * m_localTransform;
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
bool Tr2PrimitiveSet::OnModified( Be::Var* value )
{
	// SetCurrentColor is overwritten by any class that inherits
	if( IsMatch( value, m_color ) )
	{
		SetCurrentColor( m_color );
	}
	return true;
}

void Tr2PrimitiveSet::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
{
	if( ( pickTypes & PICK_TYPE_PICKING ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_PICKING, perObjectData );
	}
	if( ( pickTypes & PICK_TYPE_OPAQUE ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_OPAQUE, perObjectData );
	}
	if( ( pickTypes & PICK_TYPE_TRANSPARENT ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_TRANSPARENT, perObjectData );
		GetBatches( batches, TRIBATCHTYPE_ADDITIVE, perObjectData );
	}
}