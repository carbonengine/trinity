#include "StdAfx.h"

#include "Tr2InteriorStatic.h"

#include "TriLineSet.h"
#include "Resources/TriGeometryRes.h"
#include "Resources/TriGrannyRes.h"

#include "Tr2Mesh.h"
#include "Tr2InteriorCell.h"
#include "Tr2LitPerObjectData.h"
#include "Tr2ShaderMaterial.h"
#include "Curves/TriCurveSet.h"


using namespace Tr2RenderContextEnum;

// ---------------------------------------------------------------------------------------
Tr2InteriorStatic::Tr2InteriorStatic( IRoot* lockobj ):
	PARENTLOCK( m_detailMeshes ),
	m_display( true ),
	m_position( 0.0f, 0.0f, 0.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_parentCell( nullptr ),
	m_shSolver( NULL ),
	PARENTLOCK( m_curveSets ),
	m_depthOffset( 0.f )
{
	// 0
	m_transform = Tr2Renderer::GetIdentityTransform();

	m_uvLinearTransform.x = 1.0f;
	m_uvLinearTransform.y = 0.0f;
	m_uvLinearTransform.z = 0.0f;
	m_uvLinearTransform.w = 1.0f;

	m_uvTranslation.x = 0.0f;
	m_uvTranslation.y = 0.0f;

	// Set the dirty flag
	m_isDirty = true;

	// Initial visualize method
	m_visualizeMethod = VM_NONE;
	
	D3DXMatrixIdentity( &m_mirrorToWorldMatrix );
}

// ---------------------------------------------------------------------------------------
Tr2InteriorStatic::~Tr2InteriorStatic()
{
	RemoveFromCell();

	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
		m_geometryResource.Unlock();
	}
}

void Tr2InteriorStatic::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		for( unsigned i = 0; i != SHADER_TYPE_COUNT; ++i )
		{
			m_perObjectConstantBuffers[i].Destroy();
		}
	}
}

bool Tr2InteriorStatic::OnPrepareResources()
{
	return true;
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::SetInstanceData( const Vector4 &linearTransform, 
										 const Vector2 &translation, 
										 unsigned int instanceInSystemIdx )
{
	m_uvLinearTransform = linearTransform;
	m_uvTranslation = translation;
}

// ---------------------------------------------------------------------------------------
bool Tr2InteriorStatic::GetBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
{
	Vector3 min( FLT_MAX, FLT_MAX, FLT_MAX );
	Vector3 max( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	if( m_geometryResource && m_geometryResource->IsGood() )
	{
		for( unsigned int i = 0; i < m_geometryResource->GetMeshCount(); ++i )
		{
			Vector3 meshMin, meshMax;
			if( !m_geometryResource->GetBoundingBox( i, meshMin, meshMax ) )
			{
				return false;
			}
			BoundingBoxUpdate( min, max, meshMin, meshMax );
		}

		minBounds = min;
		maxBounds = max;
		return true;
	}
	else
	{
		return false;
	}
}

// ---------------------------------------------------------------------------------------
// Description:
//   Gets the bounding box of the static in world space.
// Arguments:
//   minBounds - [output] the minimum world-space extents of the bounding box
//   maxBounds - [output] the maximum world-space extents of the bounding box
// Return Value:
//   true, if the bounding box is ready
//   false, if the bounding box is not ready (i.e. the static is not fully loaded)
// ---------------------------------------------------------------------------------------
bool Tr2InteriorStatic::GetWorldBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
{
	if( m_geometryResource && m_geometryResource->IsGood() )
	{
		minBounds = m_worldBoundingBox.m_min;
		maxBounds = m_worldBoundingBox.m_max;
		return true;
	}

	return true;
}

Matrix Tr2InteriorStatic::GetWorldTransform() const
{
	return m_transform;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorStatic::Initialize()
{
	// calc world transform once on init
	D3DXMatrixTransformation( &m_transform, 
							  NULL, 
							  NULL, 
							  NULL, 
							  NULL, 
							  &m_rotation, 
							  &m_position );

	InitializeGeometryResource();
	m_isDirty = true;
	return true;
}

// ---------------------------------------------------------------------------------------
bool Tr2InteriorStatic::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_position ) || IsMatch( value, m_rotation ) )
	{
		// position & orientation modify: calc new transform matrix...
		D3DXMatrixTransformation( &m_transform, 
								  NULL, 
								  NULL, 
								  NULL, 
								  NULL, 
								  &m_rotation, 
								  &m_position );

		GetBoundingBox( m_worldBoundingBox.m_min, m_worldBoundingBox.m_max );
		m_worldBoundingBox.Transform( m_transform );
		// Mark our dirty flag
		m_isDirty = true;

		if( m_parentCell )
		{
			m_parentCell->RebuildBoundingBox();
		}
	}
	else if( IsMatch( value, m_geometryResPath ) )
	{
		InitializeGeometryResource();
	}

	return true;
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::Update( Be::Time time )
{
	for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		( *it )->Update( TimeAsDouble( time ) );
	}
}

// -------------------------------------------------------------
// Description:
//   Rebinds low level shaders for all meshes of the static. 
//   Uses "StaticObject" and "Interior" situations for detail
//   meshes.
// -------------------------------------------------------------
void Tr2InteriorStatic::BindLowLevelShaders()
{
	Tr2VariableStore* variableStore = NULL;
	if( m_parentCell )
	{
		variableStore = &m_parentCell->GetVariableStore();
	}

	std::vector<unsigned int> localFlags;
	unsigned int h = CcpHashFNV1( "StaticObject", strlen( "StaticObject" ) );
	localFlags.push_back( h );
	h = CcpHashFNV1( "Interior", strlen( "Interior" ) );
	localFlags.push_back( h );

	h = CcpHashFNV1( "EnlightenDirectionalIrradiance", strlen( "EnlightenDirectionalIrradiance" ) );
	localFlags.push_back( h );
	
	for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); it != m_detailMeshes.end(); ++it )
	{
		( *it )->BindLowLevelShaders( localFlags, false, variableStore );
	}
}

//  Description:
//    Helper function to get per-object data for this renderable using an arbitrary 
//	  light-set and object-to-world matrix.  Both GetPerObjectData and 
//    GetPerObjectDataWithPerInstanceLighting are thin wrappers around this function.
//  See Also:
//    GetPerObjectData, GetPerObjectDataWithPerInstanceLighting
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//    lightSet -            The set of lights illuminating this object
//    objectToWorldMatrix - The transformation matrix used to position this object in 
//							world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// ---------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorStatic::GetPerObjectDataWithLightSet( 
										ITriRenderBatchAccumulator* accumulator,
										Tr2InteriorLightSet* lightSet,
										const Matrix& objectToWorldMatrix,
										const Matrix& mirrorToWorldMatrix )
{
	Tr2LitPerObjectData* data = accumulator->Allocate<Tr2LitPerObjectData>();

	if( !data )
	{
		return NULL;
	}

	Tr2InteriorPerObjectPSData perObjectPSBuffer;
	Tr2InteriorPerObjectVSData perObjectVSBuffer;

	// 0
	memset( &perObjectPSBuffer, 0, sizeof( perObjectPSBuffer ) );
	memset( &perObjectVSBuffer, 0, sizeof( perObjectVSBuffer ) );

	// column_major for shaders
	D3DXMatrixTranspose( &perObjectVSBuffer.WorldMat, &objectToWorldMatrix );

	// Copy static object uv transforms
	perObjectVSBuffer.uvLinearTransform = m_uvLinearTransform;
	perObjectVSBuffer.uvTranslation = m_uvTranslation;

	// put pointlights in perobject data
	if( lightSet )
	{
		lightSet->PopulateLightData( &perObjectPSBuffer );
		data->SetLightsActive( lightSet->GetNumOfActiveLights(), lightSet->GetNumOfActiveLights() );
	}

	// Set the mirror-to-world matrix
	perObjectPSBuffer.mirrorToWorldMatrix = mirrorToWorldMatrix;

	// Hijack some per-object parameters, depending on the visualize method
	if( m_visualizeMethod == VM_EN_ONLY || m_visualizeMethod == VM_ALL_LIGHTING )
	{
		perObjectPSBuffer.shadowCaster0 = Vector4( 0.0f, 1.0f, 0.0f, 1.0f );
	}

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );
	data->CopyToVSFloatBuffer( perObjectVSBuffer );

	D3DXMatrixInverse( &m_mirrorToWorldMatrix, NULL, D3DXMatrixTranspose( &m_mirrorToWorldMatrix, &mirrorToWorldMatrix ) );

	return data;
}

// ---------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorStatic::GetPerObjectData( 
										ITriRenderBatchAccumulator* accumulator )
{
	return GetPerObjectDataWithLightSet( accumulator, 
										 nullptr, 
										 GetWorldTransform(), 
										 Tr2Renderer::GetIdentityTransform() );
}

// ---------------------------------------------------------------------------------------
bool Tr2InteriorStatic::HasTransparentBatches()
{
	for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); 
		it != m_detailMeshes.end(); ++it )
	{
		Tr2Mesh* mesh = *it;

		if( mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT ) && 
			!mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty() )
		{
			return true;
		}
	}

	return false;
}


// ---------------------------------------------------------------------------------------
// Description:
//   Gets the batches for rendering the Tr2InteriorStatic, depending on various settings
//   including if the target / detail meshes are being rendered, and if we're visualizing
//   the albedo
// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::GetBatches( ITriRenderBatchAccumulator* batches,
									TriBatchType batchType,
								    const Tr2PerObjectData* perObjectData )
{
	// If display is off, nothing gets displayed
	if( !m_display )
	{
		return;
	}

	if( batchType == TRIBATCHTYPE_OPAQUE || batchType == TRIBATCHTYPE_OPAQUE_PREPASS )
	{
		GetDetailMeshOpaqueBatches( batches, perObjectData, batchType );
	}
	else if( batchType == TRIBATCHTYPE_DECAL || batchType == TRIBATCHTYPE_DECAL_PREPASS )
	{
		for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); 
			it != m_detailMeshes.end(); 
			++it )
		{
			Tr2Mesh* mesh = *it;

			mesh->GetBatches( batches, 
				mesh->GetAreas( batchType ), 
				perObjectData );
		}
	}
	else if( batchType == TRIBATCHTYPE_TRANSPARENT )
	{
		for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); 
			it != m_detailMeshes.end(); 
			++it )
		{
			Tr2Mesh* mesh = *it;

			GetTransparentBatches( batches, mesh, perObjectData );
		}
	}
	else
	{
		// In any other case, look at the detail meshes
		for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); 
			it != m_detailMeshes.end(); 
			++it )
		{
			Tr2Mesh* mesh = *it;

			mesh->GetBatches( batches, 
				mesh->GetAreas( batchType ), 
				perObjectData );
		}
	}
}
// ---------------------------------------------------------------------------------------
float Tr2InteriorStatic::GetSortValue()
{
	Vector3 min, max;
	GetBoundingBox( min, max );

	Matrix worldTransform = GetWorldTransform();

	// Transform the local BB min & max to world coordinates
	D3DXVec3TransformCoord( &min, &min, &worldTransform );
	D3DXVec3TransformCoord( &max, &max, &worldTransform );

	Vector3 center = 0.5f * ( max + min );

	Vector3 d = Tr2Renderer::GetViewPosition() - center;
	return D3DXVec3Length( &d );
}

#include "TriFrustum.h"

bool Tr2InteriorStatic::IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const
{
	if( !m_geometryResource || !m_geometryResource->IsGood() )
	{
		return false;
	}
	objectToWorld = m_transform;
	return frustum.IsBoxVisible( m_worldBoundingBox.m_min, m_worldBoundingBox.m_max );
}

// ---------------------------------------------------------------------------------------
CullResult Tr2InteriorStatic::AddToCell()
{
	// if mesh is not set this fails!
	if( !m_geometryResource )
	{
		return CULLRES_FAILED;
	}

	// if this mesh is still getting loaded, we are not able to obtain bounding info yet!
	if( m_geometryResource->IsLoading() )
	{
		return CULLRES_NOTREADY;
	}

	// now we should get bounding info
	Vector3 minBounds, maxBounds;
	if( !GetBoundingBox( minBounds, maxBounds ) )
	{
		return CULLRES_FAILED;
	}

	if( m_parentCell )
	{
		BoundingBoxTransform( minBounds, maxBounds, m_transform );
		m_parentCell->UpdateBoundingBox( minBounds, maxBounds );
	}

	return CULLRES_OK;
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::RemoveFromCell()
{
	m_parentCell = NULL;
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::SetParentCell( Tr2InteriorCell* parentCell )
{
	m_parentCell = parentCell;

	if( m_parentCell )
	{
		AddToCell();
		BindLowLevelShaders();
	}
	else
	{
		RemoveFromCell();
	}
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::SetVisualizeMethod( VisualizeMethod method )
{
	m_visualizeMethod = method;
}

// ---------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the static using a per-instance light-set override 
//    and an arbitrary object-to-world matrix.  Routes the call to helper function 
//    GetPerObjectDataWithLightSet.
//  See Also:
//    GetPerObjectData, GetPerObjectDataWithLightSet
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//    lightSet -            The set of lights illuminating this object
//    objectToWorldMatrix - The transformation matrix used to position this object 
//                          in world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// ---------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorStatic::GetPerObjectDataWithPerInstanceLighting( 
										ITriRenderBatchAccumulator* accumulator,
										Tr2InteriorLightSet* lightSet,
																			  const Matrix& objectToWorldMatrix,
																			  const Matrix& mirrorToWorldMatrix )
{
	return GetPerObjectDataWithLightSet( accumulator, 
										 lightSet, 
										 objectToWorldMatrix, 
										 mirrorToWorldMatrix );
}

// ---------------------------------------------------------------------------------------
TriGeometryRes* Tr2InteriorStatic::GetGeometryResource() const
{
	if( m_geometryResource )
	{
		return m_geometryResource;
	}

	return NULL;
}

std::string Tr2InteriorStatic::GetGeometryResourcePath() const
		{
	return m_geometryResPath;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the dirty flag
// Return Value:
//   true, if the static is idrty
//   false, otherwise
// --------------------------------------------------------------------------------------
bool Tr2InteriorStatic::IsDirty( void ) const
{
	return m_isDirty;
}

// --------------------------------------------------------------------------------------
// Description:
//   Resets the dirty flag
// --------------------------------------------------------------------------------------
void Tr2InteriorStatic::ResetDirtyFlag( void )
{
	m_isDirty = false;
}

namespace {

struct ImmutableHelper : IBlueResManNotifications
{
	void OnResourceCreated( void* res )
	{
		static_cast<TriGeometryRes*>(res)->m_immutable = true;
	}

	void OnResourceFromCache( void* res ) {}
};

}


void Tr2InteriorStatic::InitializeGeometryResource()
{
	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
		m_geometryResource.Unlock();
	}
	if( !m_geometryResPath.empty() )
	{
        auto isImmutable = ImmutableHelper();
		BeResMan->GetResource( m_geometryResPath.c_str(), "", m_geometryResource, &isImmutable );
		m_geometryResource->AddNotifyTarget( this );
	}
}

void Tr2InteriorStatic::ReleaseCachedData( BlueAsyncRes* p )
{
	if( p == m_geometryResource )
	{
		if( m_parentCell )
		{
			m_parentCell->RebuildBoundingBox();
		}
	}
}

void Tr2InteriorStatic::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_geometryResource )
	{
		if( p->IsGood() )
		{
			if( m_parentCell  )
			{
				AddToCell();
				BindLowLevelShaders();
			}
		}
		GetBoundingBox( m_worldBoundingBox.m_min, m_worldBoundingBox.m_max );
		m_worldBoundingBox.Transform( m_transform );
	}
}

void Tr2InteriorStatic::GetDetailMeshOpaqueBatches( 
							ITriRenderBatchAccumulator* batches, 
							const Tr2PerObjectData* perObjectData,
							TriBatchType batchType )
{
	for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); it != m_detailMeshes.end(); ++it )
	{
		Tr2Mesh* mesh = *it;
		mesh->GetBatches( batches, mesh->GetAreas( batchType ), perObjectData );
	}
}

void Tr2InteriorStatic::GetTransparentBatches( ITriRenderBatchAccumulator* batches,
											   Tr2Mesh* mesh,
											   const Tr2PerObjectData* data )
{
	// Only gather transparent batches if the mesh isn't hidden
	if( !mesh->IsHidden() )
{
		// Get the transparent areas
		Tr2MeshAreaVector* areas = mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT );

		if( areas )
		{
			float maxDepth = Tr2Renderer::GetFrustumRadius();
			Matrix instanceTransform = m_transform * m_mirrorToWorldMatrix;

			// Loop over the transparent areas
			for( Tr2MeshAreaVector::const_iterator it = areas->begin(); it != areas->end(); ++it )
{
				Tr2MeshArea* area = *it;
				ITr2ShaderMaterial* shaderMat = area->GetMaterialInterface();

				if( area->IsHidden() )
				{
					continue;
}

				if( !shaderMat )
{
					continue;
}

				// Compute the depth
				Vector3 bbMin, bbMax;
				mesh->GetAreaBoundingBox( area->GetIndex(), bbMin, bbMax );
				Vector3 center = 0.5f * ( bbMin + bbMax );
				D3DXVec3TransformCoord( &center, &center, &instanceTransform );
				center -= Tr2Renderer::GetViewPosition();
				float z = std::min( std::max( ( D3DXVec3Length( &center ) + m_depthOffset ) / maxDepth, 0.f ), 1.f );

				unsigned int depth = ( unsigned int )( ( float )0xFFFFFFF * ( 1.0f - z ) );

				const Tr2PerObjectData *perAreaData = data;

				if( m_shSolver && area->GetUseSHLighting() )
				{
					Tr2PerAreaSHLightingData* areaData = batches->Allocate<Tr2PerAreaSHLightingData>();
					if( areaData )
					{
						Vector3 minBounds, maxBounds;
						mesh->GetAreaBoundingBox( area->GetIndex(), minBounds, maxBounds );
						for( int i = 1; i < area->GetCount(); ++i )
						{
							Vector3 min, max;
							mesh->GetAreaBoundingBox( area->GetIndex() + i, min, max );
							BoundingBoxUpdate( minBounds, maxBounds, min, max );
						}

						areaData->SetPerObjectData( data );
						m_shSolver->AddVolume( minBounds, maxBounds, m_transform, areaData );
						perAreaData = areaData;
					}
				}

				TriGeometryBatch* batch = batches->Allocate<TriGeometryBatch>();
				// Note that this can fail if the accumulator can't add more batches!
				if( batch )
				{
					batch->SetShaderMaterial( shaderMat ); 
					batch->SetPerObjectData( perAreaData );
					batch->SetGeometryResource( mesh->GetGeometryResource() );

					batch->SetMeshParameters( mesh->GetMeshIndex(), area->GetIndex(), area->GetCount(), area->GetReversed() );
					batch->SetDepth( depth );

					batches->Commit( batch );
				}
			}
		}
	}
}

