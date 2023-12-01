#include "StdAfx.h"
#include "EveSpherePin.h"
#include "Eve/EveConstantBufferFormats.h"
#include "Shader/Tr2Effect.h"
#include "Curves/TriCurveSet.h"
#include "Shader/Parameter/Tr2Vector4Parameter.h"
#include "Utilities/BoundingSphere.h"
#include "Resources/TriGeometryRes.h"
#include "Resources/TriTextureRes.h"

#include "EveSpherePinIndexTree.h"
#include "TriPoolAllocator.h"
#include "Tr2HostBitmap.h"
#include "TriRenderBatch.h"
#include "Eve/EveUpdateContext.h"

static const char* PIN_PICK_EFFECT_PATH = "res:/Graphics/Effect/Managed/Space/UI/SpherePinPicking.fx";

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );

using namespace Tr2RenderContextEnum;

std::map<TriGeometryResPtr, EveSpherePinIndexTree*>* EveSpherePin::s_treeMap = new std::map<TriGeometryResPtr, EveSpherePinIndexTree*>();

// ------------------------------------------------------------------------------------------------------
EveSpherePin::EveSpherePin( IRoot* lockobj /*= NULL*/ ):
	PARENTLOCK( m_curveSets ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_translation( 0.f, 0.f, 0.f ),
	m_centerNormal( 0.f, 0.f, 1.f ),
	m_pinRadius( 0.2f ),
	m_pinMaxRadius( 0.2f ),
	m_pinRotation( 0.f ),
	m_pinColor( 1.f, 1.f, 1.f, 1.f ),
	m_pinAlphaThreshold( 0.f ),
	m_enablePicking( true ),
	m_display( true ),
	m_sortValueMultiplier( 1.f ),
	m_primitiveCount( 0 ),
	m_tree( 0 ),
	m_rebuildIndices( 0 ),
	m_uvAtlasScaleOffset( 1.f, 1.f, 0.f, 0.f ),
	m_worldTransform( IdentityMatrix() )
{
	// create pin draw effect
	m_pinEffect.CreateInstance();

	// create picking effect
	m_pickEffect.CreateInstance();
	m_pickEffect->SetEffectPathName( PIN_PICK_EFFECT_PATH );

	// init
	BuildBoundingSphere();
	PrepareResources();
}

// ------------------------------------------------------------------------------------------------------
EveSpherePin::~EveSpherePin()
{
	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
	}
	ReleaseResources( TRISTORAGE_ALL );
	m_pinEffect = NULL;
	m_pickEffect = NULL;
}

// ------------------------------------------------------------------------------------------------------
bool EveSpherePin::Initialize()
{
	InitializeGeometryResource();
	BuildBoundingSphere();
	return true;
}

// ------------------------------------------------------------------------------------------------------
bool EveSpherePin::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_geomResPath ) )
	{
		InitializeGeometryResource();
	}
	else if( IsMatch( value, m_pinEffectResPath ) )
	{
		m_pinEffect->SetEffectPathName( m_pinEffectResPath.c_str() );
	}
	else 
	if( IsMatch( value, m_centerNormal ) ||
		IsMatch( value, m_pinRadius ) ||
		IsMatch( value, m_pinMaxRadius ) ||
		IsMatch( value, m_pinRotation ) ||
		IsMatch( value, m_pinColor ) ||
		IsMatch( value, m_pinAlphaThreshold ) ||
		IsMatch( value, m_uvAtlasScaleOffset ) )
	{
		BuildBoundingSphere();
		if( IsMatch( value, m_centerNormal ) || IsMatch( value, m_pinMaxRadius ) )
		{
			m_rebuildIndices = 1;
		}
	}
	return true;
}

// ------------------------------------------------------------------------------------------------------
void EveSpherePin::ReleaseResources( TriStorage s )
{
}

// ------------------------------------------------------------------------------------------------------
bool EveSpherePin::OnPrepareResources()
{
	if( m_tree && m_tree->IsInitialized() && !m_indexBuffer.IsValid() )
	{
		m_rebuildIndices = 1;
	}
	return true;
}

// ------------------------------------------------------------------------------------------------------
void EveSpherePin::CreateIndexBuffer()
{
	std::vector<unsigned short> indices;
	int success = m_tree->GetIndices( m_centerNormal, m_pinMaxRadius, m_primitiveCount, indices );
	
	if( !success )
	{
		return;
	}

	m_indexBuffer = Tr2BufferAL();

	if( m_primitiveCount <= 0 )
	{
		return;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	CR_RETURN( m_indexBuffer.Create( 
		2, 
		m_primitiveCount * 3, 
		Tr2GpuUsage::INDEX_BUFFER,
		Tr2CpuUsage::NONE,
		&indices[0], 
		renderContext ) );

	m_rebuildIndices = 0;
}

// ------------------------------------------------------------------------------------------------------
void EveSpherePin::InitializeGeometryResource()
{
	m_tree = 0;

	// Remove existing callback setup if any, get new geometry resource and attach callback
	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
		m_geometryResource.Unlock();
	}
	if( !m_geomResPath.empty() )
	{
		BeResMan->GetResource( m_geomResPath.c_str(), "", BlueInterfaceIID<TriGeometryRes>(), (void**)&m_geometryResource );
	}
	if( m_geometryResource )
	{
		m_geometryResource->AddNotifyTarget( this );
	}
}

// ------------------------------------------------------------------------------------------------------
void EveSpherePin::RebuildCachedData( BlueAsyncRes* p )
{
}

// ------------------------------------------------------------------------------------------------------
void EveSpherePin::ReleaseCachedData( BlueAsyncRes* p )
{
}

// ------------------------------------------------------------------------------------------------------
void EveSpherePin::UpdateSyncronous( EveUpdateContext& updateContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_tree )
	{
		if( m_geometryResource && !m_geomResPath.empty() && m_geometryResource->IsGood() )
		{
			if( !(*s_treeMap)[m_geometryResource] ) 
			{
				(*s_treeMap)[m_geometryResource] = new EveSpherePinIndexTree();
			}
			m_tree = (*s_treeMap)[m_geometryResource];

			m_rebuildIndices = 1;
		}
	}
	if( m_tree && m_rebuildIndices )
	{
		if( !m_tree->IsInitialized() && m_geometryResource )
		{
			if( m_tree->Initialize( m_geometryResource->GetMeshData( 0 ) ) )
			{
				CreateIndexBuffer();
			}
		}
		else
		{
			CreateIndexBuffer();
		}
	}

}

// ------------------------------------------------------------------------------------------------------
void EveSpherePin::UpdateAsyncronous( EveUpdateContext& updateContext )
{
	// don't forget the curves
	if( !m_curveSets.empty() )
	{
		for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
		{
			(*it)->Update( TimeAsDouble( updateContext.GetTime() ) );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void EveSpherePin::Update( EveUpdateContext& updateContext )
{
	UpdateSyncronous( updateContext );
	UpdateAsyncronous( updateContext );
}

// ------------------------------------------------------------------------------------------------------
void EveSpherePin::UpdateViewDependentData( const TriFrustum& frustum, const Matrix& parentTransform )
{
	// local transform
	Matrix localTransform = TransformationMatrix( m_scaling, m_rotation, m_translation );

	// store final world transform
	m_worldTransform = localTransform * parentTransform;
}

void EveSpherePin::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform )
{
	if( !m_display )
	{
		return;
	}

	// position the lines with parent transform
	UpdateViewDependentData( frustum, parentTransform );
	Vector4 boundingSphere = m_boundingSphere;
	BoundingSphereTransform( m_worldTransform, boundingSphere );
}

void EveSpherePin::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	GetRenderables( renderables, nullptr );
}

// ------------------------------------------------------------------------------------------------------
void EveSpherePin::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
	if( !m_display )
	{
		return;
	}
	// cull!
//	if( frustum.IsSphereVisible( &boundingSphere ) )
	{
		renderables.push_back( this );
	}
}

// ------------------------------------------------------------------------------------------------------
bool EveSpherePin::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	sphere = m_boundingSphere;
	return true;
}

// ------------------------------------------------------------------------------------------------------
void EveSpherePin::BuildBoundingSphere()
{
	// build a bounding sphere around this pin
	m_boundingSphere = Vector4( m_centerNormal, m_pinRadius );
}

// ------------------------------------------------------------------------------------------------------
bool EveSpherePin::HasTransparentBatches()
{
	return true;
}

// -----------------------------------------------------------------------------
void EveSpherePin::GetBatches( ITriRenderBatchAccumulator* accumulator, 
							   TriBatchType batchType, 
							   const Tr2PerObjectData* perObjectData,
							   Tr2RenderReason reason )
{
	if( batchType == TRIBATCHTYPE_TRANSPARENT && m_pinEffect )
	{
		GetBatchWithEffect( accumulator, perObjectData, m_pinEffect );
	}
	else if( batchType == TRIBATCHTYPE_PICKING && m_pickEffect )
	{
		if( m_enablePicking )
		{
			GetBatchWithEffect( accumulator, perObjectData, m_pickEffect );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
float EveSpherePin::GetSortValue()
{
	// center of bounding sphere is "check point"
	Vector3 center = TransformCoord( BoundingSphereGetCenter( m_boundingSphere ), m_worldTransform );

	// distance from viewer to sort z
	Vector3 d = Tr2Renderer::GetViewPosition() - center;
	float distance = Length( d );
	return distance * m_sortValueMultiplier;
}

// ------------------------------------------------------------------------------------------------------
Tr2PerObjectData* EveSpherePin::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	// allocate only once
	auto perObjectData = accumulator->Allocate<EveSpherePinPerObjectData>();
	if( !perObjectData )
	{
		return NULL;
	}

	// set world matrix
	perObjectData->m_worldMatrix = Transpose( m_worldTransform );
	// set all other pin data
	perObjectData->m_pinPosition = Vector4( m_centerNormal, m_pinRadius );
	perObjectData->m_pinRotation =  Vector4( m_pinRotation, 0.f, 0.f, 0.f );
	perObjectData->m_pinColor = Vector4( m_pinColor.r, m_pinColor.g, m_pinColor.b, m_pinColor.a );
	perObjectData->m_pinThreshold = Vector4( m_pinAlphaThreshold, 0.f, 0.f, 0.f );
	perObjectData->m_pinRadiusPrecalc = Vector4( sinf( m_pinRadius ), cosf( m_pinRadius ), sinf( m_pinRotation ), cosf( m_pinRotation ) );
	perObjectData->m_pinUV = m_uvAtlasScaleOffset;

	return perObjectData;
}

// ------------------------------------------------------------------------------------------------------
void EveSpherePin::SubmitGeometry( Tr2RenderContext& renderContext )
{
	if( !m_geometryResource )
	{
		return;
	}

	if( !m_geometryResource->IsGood() )
	{
		return;
	}

	if( m_geometryResource->GetMeshCount() < 1 )
	{
		return;
	}

	const TriGeometryResMeshData* meshData = m_geometryResource->GetMeshData( 0 );
	if( !meshData )
	{
		return;
	}

	if( !m_indexBuffer.IsValid() )
	{
		return;
	}

	if( !meshData->m_vertexBuffer.IsValid() )
	{
		return;
	}

	// render
	renderContext.m_esm.ApplyVertexDeclaration( meshData->m_vertexDeclaration );
	renderContext.m_esm.ApplyStreamSource( 0, meshData->m_vertexBuffer, 0, meshData->m_bytesPerVertex );
	renderContext.m_esm.ApplyIndexBuffer( m_indexBuffer );
	
	renderContext.SetTopology( TOP_TRIANGLES );
	renderContext.DrawIndexedPrimitive( meshData->m_vertexCount, 0, m_primitiveCount );
}

void EveSpherePin::GetBatchWithEffect( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData, Tr2Effect* effect )
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

void EveSpherePin::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
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
// --------------------------------------------------------------------------------
// Description:
//   Copy all the matrices and other data to HW
// --------------------------------------------------------------------------------
void EveSpherePinPerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// set to VS
	int vsConstantCount = 4 + 6;
	FillAndSetConstants( *buffers[VERTEX_SHADER], &m_worldMatrix, vsConstantCount * 16, VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	// set to PS
	int psConstantCount = 4 + 6;
	FillAndSetConstants( *buffers[PIXEL_SHADER], &m_worldMatrix, psConstantCount * 16, PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
}
