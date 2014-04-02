#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2IntSkinnedObject.h"

#include "Utilities/BoundingSphere.h"
#include "TriSettingsRegistrar.h"
#include "Apex/Tr2ClothingActor.h"
#include "Tr2InteriorCell.h"
#include "Tr2LitPerObjectData.h"
#include "Apex/Apex.h"
#include "Resources/TriGeometryRes.h"
#include "ITr2UmbraUserData.h"
#include "Tr2ShaderMaterial.h"
#include "Tr2Effect.h"
#include "Tr2Mesh.h"

CCP_STATS_DECLARE( wodIntSkinnedObjectsAlive, "Trinity/wodIntSkinnedObjectsAlive", false, CST_COUNTER_LOW, "Count of Tr2IntSkinnedObjects alive" );

Tr2IntSkinnedObject::Tr2IntSkinnedObject( IRoot* lockobj ) :
	Tr2SkinnedObject( lockobj ),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_lightSet(),
	m_shSampleIndex( -1 ),
	m_SHMatrixRed(),
	m_SHMatrixGreen(),
	m_SHMatrixBlue(),
	m_isVisible( false ),
	m_umbraObjects(),
	m_umbraModel( NULL ),
	m_isDirty( true ),
	m_isInApexScene( false ),
	m_shSolver( NULL ),
	m_cellReflectionTime( 0.0f ),
	m_previousUpdateTime( 0 ),
	m_castsShadows( true ),
	m_probeOffset( 0.f, 0.f, 0.f ),
	m_depthOffset( 0.f )
{
	m_visibleLightCount = 0;
	
	m_currentPosition = Vector3( 0.0f, 0.0f, 0.0f );
	m_currentScaling = Vector3( 1.0f, 1.0f, 1.0f );
	m_currentRotation = Quaternion( 0.0f, 0.0f, 0.0f, 1.0f );
	m_positionSet = false;
	m_scalingSet = false;
	m_rotationSet = false;

	m_variableStore.CreateInstance();
	m_variableStore->SetParentVariableStore( &GlobalStore() );

	m_variableStore->RegisterVariable( "CellReflectionMap", (TriTextureRes*)NULL );
	m_variableStore->RegisterVariable( "CellReflection2ndMap", (TriTextureRes*)NULL );
	m_variableStore->RegisterVariable( "CellReflectionInterpolation", 0.0f );

	D3DXMatrixIdentity( &m_mirrorToWorldMatrix );

	CCP_STATS_INC( wodIntSkinnedObjectsAlive );
}

Tr2IntSkinnedObject::~Tr2IntSkinnedObject()
{
	// Get out of cell and free umbra stuff
	ClearUmbra();

	// Get out of Apex
	RemoveFromApexScene();

	CCP_STATS_DEC( wodIntSkinnedObjectsAlive );
}

bool Tr2IntSkinnedObject::Initialize()
{
	m_lod.PopulateLods();

	return true;
}

bool Tr2IntSkinnedObject::OnModified( Be::Var* value )
{
	if( Tr2SkinnedObject::OnModified( value ) )
	{
		return true;
	}
	
	m_lod.OnModified( value );
	return true;
}

void Tr2IntSkinnedObject::PrePhysicsUpdate( Be::Time time )
{
	Tr2SkinnedObject::PrePhysicsUpdate( time );

	for( PITr2RenderableVector::iterator it = m_attachedObjects.begin(); it != m_attachedObjects.end(); ++it )
	{
		ITr2InteriorDynamic* dynamic = dynamic_cast<ITr2InteriorDynamic*>( ( *it ) );
		if( dynamic )
		{
			dynamic->PrePhysicsUpdate( time );
		}
	}
}

void Tr2IntSkinnedObject::PostPhysicsUpdate( Be::Time time, Tr2ApexScene* apexScene )
{
	Tr2SkinnedObject::PostPhysicsUpdate( time, apexScene );

	for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
		 it != m_umbraObjects.end(); ++it )
	{
		Matrix m = GetSkinningTransform();

		if( ( *it )->getCell() )
		{
			Matrix cellTransform;
			( *it )->getCell()->getCellToWorldMatrix( AS_UMBRA_MATRIX( cellTransform ) );
			XMVECTOR det;
			m = XMMatrixMultiply( m, XMMatrixInverse( &det, cellTransform ) );
		}
		( *it )->setObjectToCellMatrix( AS_UMBRA_MATRIX( m ) );
	}

	for( PITr2RenderableVector::iterator it = m_attachedObjects.begin(); it != m_attachedObjects.end(); ++it )
	{
		ITr2InteriorDynamic* dynamic = dynamic_cast<ITr2InteriorDynamic*>( ( *it ) );
		if( dynamic )
		{
			dynamic->PostPhysicsUpdate( time, apexScene );
		}
	}

	m_variableStore->RegisterVariable( "CellReflectionMap", m_cellReflectionMaps[0] );
	m_variableStore->RegisterVariable( "CellReflection2ndMap", m_cellReflectionMaps[1] );
	m_cellReflectionTime += TimeAsFloat( time - m_previousUpdateTime );
	m_variableStore->RegisterVariable( "CellReflectionInterpolation", m_cellReflectionTime );
	m_previousUpdateTime = time;
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds per-cell reflection map to an array of current reflection maps. 
// Arguments:
//   texture - Per-cell reflection map
// --------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::AddReflectionMap( TriTextureRes* texture )
{
	if( texture == NULL )
	{
		return;
	}
	if( texture == m_cellReflectionMaps[0] || texture == m_cellReflectionMaps[1] )
	{
		return;
	}
	if( m_cellReflectionMaps[0] == NULL )
	{
		m_cellReflectionMaps[0] = m_cellReflectionMaps[1] = texture;
		m_cellReflectionTime = 0.0f;
	}
	else if( m_cellReflectionMaps[1] == NULL )
	{
		m_cellReflectionMaps[1] = texture;
		m_cellReflectionTime = 0.0f;
	}
	else
	{
		m_cellReflectionMaps[0] = m_cellReflectionMaps[1];
		m_cellReflectionMaps[1] = texture;
		m_cellReflectionTime = 0.0f;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes per-cell reflection map from an array of current reflection maps. 
// Arguments:
//   texture - Per-cell reflection map
// --------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::RemoveReflectionMap( TriTextureRes* texture )
{
	if( texture == NULL )
	{
		return;
	}
	if( texture == m_cellReflectionMaps[1] )
	{
		m_cellReflectionMaps[1] = m_cellReflectionMaps[0];
		m_cellReflectionMaps[0] = texture;
		m_cellReflectionTime = 0.0f;
	}
}

namespace {

void SetVariableStore( ITr2ShaderMaterial* const effect, const std::vector<unsigned> & localFlags, Tr2VariableStorePtr store )
{
	if( effect )
	{
		effect->SetVariableStore( store );
		if( Tr2ShaderMaterial* material = dynamic_cast<Tr2ShaderMaterial*>( effect ) )
		{
			material->BindLowLevelShaderMaterialOnly( localFlags );
		}
		else if( Tr2Effect* fx = dynamic_cast<Tr2Effect*>( effect ) )
		{
			fx->RebuildCachedData( fx->GetEffectRes() );
		}
	}
}

}

// --------------------------------------------------------------------------------------
// Description:
//   Binds low-level shaders on all meshes of the skinned object. Applies local variable
//	 store.
// --------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::BindLowLevelShaders()
{
	std::vector<unsigned int> localFlags;
	unsigned int h = CcpHashFNV1( "SkinnedObject", strlen( "SkinnedObject" ) );
	localFlags.push_back( h );
	h = CcpHashFNV1( "Interior", strlen( "Interior" ) );
	localFlags.push_back( h );

	for( Tr2ClothingActorVector::iterator it = m_clothMeshes.begin(); it != m_clothMeshes.end(); ++it )
	{
		if( Tr2ClothingActor* ca = *it )
		{
			SetVariableStore( ca->GetEffect(),						localFlags, m_variableStore );
			SetVariableStore( ca->GetEffectReversed(),				localFlags, m_variableStore );
			SetVariableStore( ca->GetDepthEffect(),					localFlags, m_variableStore );
			SetVariableStore( ca->GetDepthEffectReversed(),			localFlags, m_variableStore );
			SetVariableStore( ca->GetDepthNormalEffect(),			localFlags, m_variableStore );
			SetVariableStore( ca->GetDepthNormalEffectReversed(),	localFlags, m_variableStore );
		}
	}

	if( m_visualModel )
	{
		for( PTr2MeshVector::const_iterator meshIt = m_visualModel->GetMeshes().begin(); meshIt != m_visualModel->GetMeshes().end(); ++meshIt )
		{
			( *meshIt )->BindLowLevelShaders( localFlags, false, m_variableStore );
		}
	}
}

void Tr2IntSkinnedObject::SetVisibility( bool bVisible )
{
	m_isVisible = bVisible;
}

bool Tr2IntSkinnedObject::GetBoundingSphere( Vector4& sphere ) const
{
	if( m_boundingSphere.w > 0.f )
	{
		sphere = m_boundingSphere;
		BoundingSphereTransform( GetSkinningTransform(), sphere );
	}
	else
	{
		// TODO: make this more character dependent
		sphere = Vector4( GetPosition(), 2.f );
	}
	return true;
}

bool Tr2IntSkinnedObject::GetWorldBoundingBox( Vector3& min, Vector3& max ) const
{
	if( !GetLocalBoundingBox( min, max ) )
	{
		return false;
	}

	BoundingBoxTransform( min, max, GetSkinningTransform() );

	return true;
}

bool Tr2IntSkinnedObject::IsBoundingBoxReady( void ) const
{
	Vector3 min, max;
	return GetWorldBoundingBox( min, max );
}

bool Tr2IntSkinnedObject::GetShProbePosition( Vector3& position ) const
{
	Vector3 min, max;
	GetWorldBoundingBox( min, max );
	position = ( min + max ) * 0.5f + m_probeOffset;
	return true;
}

void Tr2IntSkinnedObject::AddToApexScene( Tr2ApexScene* apexScene )
{
	if( !m_isInApexScene )
	{
		for( Tr2ClothingActorVector::iterator it = m_clothMeshes.begin(); it != m_clothMeshes.end(); ++it )
		{
			if( Tr2ClothingActor* ca = *it )
			{
				ca->AddToApexScene( apexScene );
			}
		}

		m_isInApexScene = true;
	}
}

void Tr2IntSkinnedObject::RemoveFromApexScene( void )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_isInApexScene )
	{
		for( Tr2ClothingActorVector::iterator it = m_clothMeshes.begin(); it != m_clothMeshes.end(); ++it )
		{
			if( Tr2ClothingActor* ca = *it )
			{
				ca->RemoveFromApexScene();
			}
		}
	}
}

Tr2PerObjectData* Tr2IntSkinnedObject::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	// we support cpu AND gpu skinning, so decide which ::GetPerObjectData() to call based on the type of the Tr2 class
	if( m_visualModel )
	{
		const Matrix& idMatrix = Tr2Renderer::GetIdentityTransform();
		
		if( m_visualModel->ClassType() == Tr2SkinnedModel::ClassType_() )
		{
			return GetPerObjectDataGpuSkinning( accumulator, 
												&m_lightSet, 
												GetSkinningTransform(), 
												idMatrix );
		}
		else
		{
			return GetPerObjectDataCpuSkinning( accumulator, 
												&m_lightSet, 
												GetSkinningTransform(), 
												idMatrix );
		}
	}
	return NULL;
}

extern int g_maxClothLod;

void Tr2IntSkinnedObject::GetBatches( ITriRenderBatchAccumulator* batches,
								   TriBatchType batchType,
								   const Tr2PerObjectData* perObjectData )
{
	unsigned int depth = 0xFFFFFFFF;
	if( DoDisplay() && batchType == TRIBATCHTYPE_TRANSPARENT )
	{
		float maxDepth = Tr2Renderer::GetFrustumRadius();
		Matrix instanceTransform = m_transform * m_mirrorToWorldMatrix;

		// Compute the depth
		Vector3 bbMin, bbMax;
		GetLocalBoundingBox( bbMin, bbMax );
		Vector3 center = 0.5f * ( bbMin + bbMax );
		D3DXVec3TransformCoord( &center, &center, &instanceTransform );
		center -= Tr2Renderer::GetViewPosition();
		float z = std::min( std::max( ( D3DXVec3Length( &center ) + m_depthOffset ) / maxDepth, 0.f ), 1.f );

		depth = ( unsigned int )( ( float )0xFFFFFFF * ( 1.0f - z ) );
	}

	if( DoDisplay() && m_lod.IsSimulatingCloth( g_maxClothLod ) && 
			( batchType == TRIBATCHTYPE_DECAL		||
			batchType == TRIBATCHTYPE_DECAL_PREPASS	||
			 batchType == TRIBATCHTYPE_TRANSPARENT	||
			 batchType == TRIBATCHTYPE_DEPTHNORMAL	||
			 batchType == TRIBATCHTYPE_DEPTH )
		)
	{
		g_Tr2Apex->ApexGatherBatches( m_clothMeshes, batches, batchType, perObjectData, m_shSolver, depth );
	}

	if( DoDisplay() )
	{
		Matrix* pm = batches->Allocate<Matrix>();

		CCP_ASSERT_M( pm, "No memory available for allocation of batches." );
		if( pm == NULL )
		{
			return;
		}
		*pm = GetSkinningTransform();

		Tr2PerObjectDataSkinned* skinnedData = ( Tr2PerObjectDataSkinned* )perObjectData;

		for( PTr2MeshVector::const_iterator meshIt = m_visualModel->GetMeshes().begin(); meshIt != m_visualModel->GetMeshes().end(); ++meshIt )
		{
			Tr2Mesh* mesh = *meshIt;
			if( mesh )
			{
				if( mesh->HasPendingLowLevelShaderBind() )
				{
					mesh->ExecutePendingLowLevelShaderBind();
				}	

				Tr2MeshAreaVector* areas = mesh->GetAreas( batchType );
				if( areas )
				{
					if( mesh->IsHidden() )
					{
						continue;
					}

					TriGeometryRes* geomRes = mesh->GetGeometryResource();
					if( !geomRes )
					{
						continue;
					}
					int meshIx = mesh->GetMeshIndex();

					TriGeometryResMeshData* meshData = geomRes->GetMeshData( meshIx );
					if( !meshData )
					{
						continue;
					}

					for( PTr2MeshAreaVector::iterator it = areas->begin(); it != areas->end(); ++it )
					{
						Tr2MeshArea* area = *it;
						ITr2ShaderMaterial* shader = area->GetMaterialInterface();
						if( area->IsHidden() || ( !shader ) )
						{
							continue;
						}
						TriGeometryBatch* batch = batches->Allocate<TriGeometryBatch>();
						Tr2PerAreaDataSkinned* areaData = batches->Allocate<Tr2PerAreaDataSkinned>();

						// Note that this can fail if the accumulator can't add more batches!
						if( batch && areaData )
						{
							unsigned int n = 0;

							// if we have bone-remapping, get jointbindings from area
							TriGeometryResAreaData* areaRes = geomRes->GetAreaData( meshIx, area->GetIndex() );
							if( areaRes && meshData->m_hasPerMeshAreaBoneBindings )
							{
								// number of per this area
								n = (unsigned int)areaRes->m_jointBindings.size();
							}
							else
							{
								n = area->GetJointCount();
							}

							// Much more than this would trash the per-frame data
							if( n > TR2_MAX_BONES_PER_MESHAREA )
							{
								continue;
							}

							areaData->SetUserData( skinnedData->GetUserData() );
							unsigned int* animMapping = area->GetJointMappingAnimRig();
							if( ( m_visualModel->GetSkeleton() != NULL ) && ( animMapping != NULL ) )
							{
								areaData->SetJointCount( n );

								for( unsigned int joint = 0; joint < n; ++joint )
								{
									// apply a lookup to change the bone-index from per-mesharea to per-mesh, if we have per-mesharea
									int meshBoneIx = 0;
									if( meshData->m_hasPerMeshAreaBoneBindings )
									{
										meshBoneIx = areaRes->m_jointBindings[joint];
									}
									else
									{
										meshBoneIx = joint;
									}
									// ... then from per-mesh into the skeleton
									float* m = skinnedData->GetSkinningMatrix( animMapping[meshBoneIx] );
									areaData->SetJointTransform( joint, m );
								}
							}
							else
							{
								areaData->SetJointCount( TR2_MAX_BONES_PER_MESHAREA );
								float idMatrix[4 * 3];
								memset( idMatrix, 0, 4 * 3 * sizeof( float ) );
								idMatrix[0] = idMatrix[5] = idMatrix[10] = 1.f;
								for( unsigned int joint = 0; joint < TR2_MAX_BONES_PER_MESHAREA; ++joint )
								{
									areaData->SetJointTransform( joint, idMatrix );
								}
							}
							areaData->SetPerObjectData( *skinnedData );

							Tr2PerObjectData *perAreaData = areaData;

							if( m_shSolver && area->GetUseSHLighting() )
							{
								Tr2PerAreaSHLightingData* shAreaData = batches->Allocate<Tr2PerAreaSHLightingData>();
								if( shAreaData )
								{
									// Calculate area bounding box
									XMVECTOR boundsMin = areaRes->m_minBounds;
									XMVECTOR boundsMax = areaRes->m_maxBounds;

									if( areaData->GetJointCount() )
									{
										XMVECTOR corners[8];
										corners[0] = areaRes->m_minBounds;
										corners[1] = Vector3( areaRes->m_minBounds.x, areaRes->m_minBounds.y, areaRes->m_maxBounds.z );
										corners[2] = Vector3( areaRes->m_minBounds.x, areaRes->m_maxBounds.y, areaRes->m_minBounds.z );
										corners[3] = Vector3( areaRes->m_minBounds.x, areaRes->m_maxBounds.y, areaRes->m_maxBounds.z );
										corners[4] = Vector3( areaRes->m_maxBounds.x, areaRes->m_minBounds.y, areaRes->m_minBounds.z );
										corners[5] = Vector3( areaRes->m_maxBounds.x, areaRes->m_minBounds.y, areaRes->m_maxBounds.z );
										corners[6] = Vector3( areaRes->m_maxBounds.x, areaRes->m_maxBounds.y, areaRes->m_minBounds.z );
										corners[7] = areaRes->m_maxBounds;

										for( unsigned joint = 0; joint < areaData->GetJointCount(); ++joint )
										{
											XMMATRIX transform;
											memcpy( &transform, areaData->GetMatrices() + joint * 12, 12 * sizeof( float ) );
											transform.r[3] = Vector4( 0.0f, 0.0f, 0.0f, 1.0f );
											transform = XMMatrixTranspose( transform );

											for( int i = 0; i < 8; ++i )
											{
												XMVECTOR cornerTransformed = XMVector3Transform( corners[i], transform );
												XMVECTOR compare = XMVectorLess( cornerTransformed, boundsMin );
												boundsMin = XMVectorSelect( boundsMin, cornerTransformed, compare );
												compare = XMVectorGreater( cornerTransformed, boundsMax );
												boundsMax = XMVectorSelect( boundsMax, cornerTransformed, compare );
											}
										}
									}

									Vector3 minBounds( boundsMin );
									Vector3 maxBounds( boundsMax );

									shAreaData->SetPerObjectData( areaData );
									m_shSolver->AddVolume( minBounds, maxBounds, m_transform, shAreaData );
									perAreaData = shAreaData;
								}
							}

							batch->SetTransform( pm );
							batch->SetShaderMaterial( shader );
							batch->SetPerObjectData( perAreaData );
							batch->SetGeometryResource( geomRes );
							batch->SetMeshParameters( meshIx, area->GetIndex(), area->GetCount(), area->GetReversed() );
							batch->SetDepth( depth );

							batches->Commit( batch );
						}
					}
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for populating per-object data with instanced lighting.
// Arguments:
//   accumulator		 - The accumulator used to allocate the per-object data
//   lightSet			 - The instanced lights
//   objectToWorldMatrix - The world transform of the object
//   mirrorToWorldMatrix - The matrix mapping from mirrored space to world-space
// Return Value:
//   The allocated per-object data, or NULL if the allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2IntSkinnedObject::GetPerObjectDataCpuSkinning( 
	ITriRenderBatchAccumulator* accumulator,
	Tr2InteriorLightSet* lightSet,
	const Matrix& objectToWorldMatrix,
	const Matrix& mirrorToWorldMatrix )
{
	UpdatePerObjectData();

	Tr2LitPerObjectData* data = accumulator->Allocate<Tr2LitPerObjectData>();

	if( !data )
	{
		return NULL;
	}

	// Pixel Shader Light information
	Tr2InteriorPerObjectPSData perObjectPSBuffer;
	// standard vertex shader data
	Tr2PerObjectVSData perObjectVSBuffer;

	// 0
	memset( &perObjectPSBuffer, 0, sizeof( perObjectPSBuffer ) );
	memset( &perObjectVSBuffer, 0, sizeof( perObjectVSBuffer ) );

	// put worldmatrix to identity: translation comes from gameworld in the skinning matrices
	//D3DXMatrixIdentity( &perObjectVSBuffer.WorldMat );
	perObjectVSBuffer.WorldMat = objectToWorldMatrix;

	// put pointlights in perobject data
	if( lightSet )
	{
		lightSet->PopulateLightData( &perObjectPSBuffer );
		data->SetLightsActive( lightSet->GetNumOfActiveLights(), lightSet->GetNumOfActiveLights() );
	}

	// Copy the SH matrices
	D3DXMatrixTranspose( &perObjectPSBuffer.redMat, &m_SHMatrixRed );
	D3DXMatrixTranspose( &perObjectPSBuffer.greenMat, &m_SHMatrixGreen );
	D3DXMatrixTranspose( &perObjectPSBuffer.blueMat, &m_SHMatrixBlue );

	// Copy the mirror-to-world matrix
	perObjectPSBuffer.mirrorToWorldMatrix = mirrorToWorldMatrix;

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );
	data->CopyToVSFloatBuffer( perObjectVSBuffer );

	return data;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for populating per-object data with a skinning matrix palette for 
//   GPU-skinning.  This uses instanced lighting.
// Arguments:
//   accumulator		 - The accumulator used to allocate the per-object data
//   lightSet			 - The instanced lights
//   objectToWorldMatrix - The world transform of the object
//   mirrorToWorldMatrix - The matrix mapping from mirrored space to world-space
// Return Value:
//   The allocated per-object data, or NULL if the allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2IntSkinnedObject::GetPerObjectDataGpuSkinning( 
	ITriRenderBatchAccumulator* accumulator,
	Tr2InteriorLightSet* lightSet,
	const Matrix& objectToWorldMatrix,
	const Matrix& mirrorToWorldMatrix )
{
	UpdatePerObjectData();

	Tr2LitPerObjectDataSkinned* data = accumulator->Allocate<Tr2LitPerObjectDataSkinned>();

	if( !data )
	{
		CCP_ASSERT( !"Not enough memory for skinning!" );
		return NULL;	// should not happen (allocator out of memory??) but if it does, let's not crash.
	}

	data->SetSkinningMatrices( m_skinningMatrixCount, GetSkinningMatrices() );
	data->SetWorldMatrix( objectToWorldMatrix );

	Matrix mirrorMatrix;
	D3DXMatrixTranspose( &mirrorMatrix, &mirrorToWorldMatrix );
	D3DXMatrixInverse( &mirrorMatrix, NULL, &mirrorMatrix );
	D3DXMatrixTranspose( &mirrorMatrix, &mirrorMatrix );

	data->SetMirrorMatrix( mirrorMatrix );


	// Pixel Shader Light information
	Tr2InteriorPerObjectPSData perObjectPSBuffer;

	// 0
	memset( &perObjectPSBuffer, 0, sizeof( perObjectPSBuffer ) );

	// put pointlights in perobject data
	if( lightSet )
	{
		lightSet->PopulateLightData( &perObjectPSBuffer );
		data->SetLightsActive( lightSet->GetNumOfActiveLights(), lightSet->GetNumOfActiveLights() );
	}

	// Copy the SH matrices
	D3DXMatrixTranspose( &perObjectPSBuffer.redMat, &m_SHMatrixRed );
	D3DXMatrixTranspose( &perObjectPSBuffer.greenMat, &m_SHMatrixGreen );
	D3DXMatrixTranspose( &perObjectPSBuffer.blueMat, &m_SHMatrixBlue );

	// Copy the mirror-to-world matrix
	perObjectPSBuffer.mirrorToWorldMatrix = mirrorToWorldMatrix;

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	return data;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for populating pre-pass optimized per-object data
// Arguments:
//   accumulator		 - The accumulator used to allocate the per-object data
//   objectToWorldMatrix - The world transform of the object
//   mirrorToWorldMatrix - The transform from mirror space to world space
// Return Value:
//   The allocated per-object data, or NULL if the allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2IntSkinnedObject::GetPerObjectDataForPrePassCpuSkinning( 
	ITriRenderBatchAccumulator* accumulator,
	const Matrix& objectToWorldMatrix, 
	const Matrix& mirrorToWorldMatrix )
{
	UpdatePerObjectData();

	Tr2PerObjectDataPrePass* data = accumulator->Allocate<Tr2PerObjectDataPrePass>();

	if( !data )
	{
		return NULL;
	}

	// standard vertex shader data
	Tr2PerObjectVSData perObjectVSBuffer;

	// 0
	memset( &perObjectVSBuffer, 0, sizeof( perObjectVSBuffer ) );

	// put worldmatrix to identity: translation comes from gameworld in the skinning matrices
	//D3DXMatrixIdentity( &perObjectVSBuffer.WorldMat );
	perObjectVSBuffer.WorldMat = objectToWorldMatrix;

	// Do the copy
	data->CopyToVSFloatBuffer( perObjectVSBuffer );

	return data;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for populating pre-pass optimized per-object data with a skinning
//   matrix palette for GPU-skinning.
// Arguments:
//   accumulator		 - The accumulator used to allocate the per-object data
//   objectToWorldMatrix - The world transform of the object
//   mirrorToWorldMatrix - The transform from mirror space to world space
// Return Value:
//   The allocated per-object data, or NULL if the allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2IntSkinnedObject::GetPerObjectDataForPrePassGpuSkinning( 
	ITriRenderBatchAccumulator* accumulator,
	const Matrix& objectToWorldMatrix, 
	const Matrix& mirrorToWorldMatrix )
{
	UpdatePerObjectData();

	Tr2PerObjectDataPrePassSkinned* data = 
		accumulator->Allocate<Tr2PerObjectDataPrePassSkinned>();

	if( !data )
	{
		return NULL;
	}

	data->SetSkinningMatrices( m_skinningMatrixCount, GetSkinningMatrices() );
	data->SetWorldMatrix( objectToWorldMatrix );
	data->SetMirrorMatrix( mirrorToWorldMatrix );

	return data;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2IntSkinnedObject::TestCellIntersectionAndAdd( Tr2InteriorCell* cell )
{
	// Bail out if the cell is invalid
	if( cell == NULL )
	{
		// No cell, return no intersection
		return false;
	}

	// Get the cell's bounding box
	Vector3 cellMinBounds, cellMaxBounds;
	if( !cell->IsUnbounded() && !cell->GetBoundingBox( cellMinBounds, cellMaxBounds ) )
	{
		// Cell's bounding box not up-to-date, return false (no intersection)
		return false;
	}

	// Get our bounding box
	Vector3 minBounds, maxBounds;
	if( !GetWorldBoundingBox( minBounds, maxBounds ) )
	{
		// Our bounding box is not ready, return false (no intersection)
		return false;
	}

	XMVECTOR xmScale, xmRot, xmTrans;
	Matrix m = cell->GetWorldTransform();
	XMMatrixDecompose( &xmScale, &xmRot, &xmTrans, m );

	Vector3 center = 0.5f * ( cellMinBounds + cellMaxBounds ) + Vector3( xmTrans );
	Vector3 extents = cellMaxBounds - center;

	bool intersects = cell->IsUnbounded() || 
		IntersectOrientedBoxAxisAlignedBox( center, extents, Quaternion( xmRot ), 
			minBounds, maxBounds );

	// If we got an intersection, add to the cell
	if( intersects )
	{
		XMVECTOR det;
		Matrix m( XMMatrixMultiply( GetSkinningTransform(), 
			XMMatrixInverse( &det, cell->GetWorldTransform() ) ) );

		if( cell->AddDynamic( this ) )
		{
			// Add to the Umbra cell
			Umbra::Object* object = Umbra::Object::create( m_umbraModel );
			object->setCell( cell->GetUmbraCell() );

			object->setObjectToCellMatrix( AS_UMBRA_MATRIX( m ) );
			object->setUserPointer( CONVERT_TO_UMBRA_USER_DATA( this ) );
			m_umbraObjects.push_back( object );
		}
		else
		{
			for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
				it != m_umbraObjects.end(); ++it )
			{
				if( ( *it )->getCell() == cell->GetUmbraCell() )
				{
					( *it )->setObjectToCellMatrix( AS_UMBRA_MATRIX( m ) );
				}
			}
		}
		AddReflectionMap( cell->GetReflectionMap() );
	}
	else
	{
		// Remove the dynamic from the cell's internal list
		cell->RemoveDynamic( this );

		// Iterate over the umbra object list and destroy any object in the cell we
		// are vacating
		for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
			it != m_umbraObjects.end(); )
		{
			if( ( *it )->getCell() == cell->GetUmbraCell() )
			{
				( *it )->setCell( NULL );
				( *it )->release();
				( *it ) = NULL;
				it = m_umbraObjects.erase( it );
			}
			else
			{
				++it;
			}
		}
		RemoveReflectionMap( cell->GetReflectionMap() );
	}

	// Return the result of the intersection test
	return intersects;
}

// --------------------------------------------------------------------------------------
// Description:
//   Blah.  Too hungover to write decent documentation.  FML.
// --------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::CellRemoved( Tr2InteriorCell* cell )
{
	// Bail out if the cell is NULL
	if( !cell )
	{
		return;
	}

	// Get the Umbra cell from the interior cell
	Umbra::Cell* ucell = cell->GetUmbraCell();
	if( !ucell )
	{
		return;
	}

	for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
		it != m_umbraObjects.end(); )
	{
		if( ucell == ( *it )->getCell() )
		{
			( *it )->release();
			it = m_umbraObjects.erase( it );		
		}
		else
		{
			++it;
		}
	}
	RemoveReflectionMap( cell->GetReflectionMap() );
}

// --------------------------------------------------------------------------------------
//  Description:
//    Adds the object to the root cell. This happens as a fallback when an object
//    happens to be outside of any normal cell.
//  Arguments:
//    cell - the root cell
// --------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::AddToRootCell( Umbra::Cell* cell )
{
	for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
		it != m_umbraObjects.end(); )
	{
		if( m_umbraObjects.size() <= 1 )
		{
			break;
		}
		( *it )->setCell( NULL );
		( *it )->release();
		( *it ) = NULL;
		it = m_umbraObjects.erase( it );
	}
	if( m_umbraObjects.empty() )
	{
		Umbra::Object* object = Umbra::Object::create( m_umbraModel );
		object->setUserPointer( CONVERT_TO_UMBRA_USER_DATA( this ) );
		m_umbraObjects.push_back( object );
	}
	m_umbraObjects.front()->setCell( cell );
	m_umbraObjects.front()->setObjectToCellMatrix( AS_UMBRA_MATRIX( m_transform ) );

	m_SHMatrixRed = m_SHMatrixGreen = m_SHMatrixBlue = Matrix( 
		0.f, 0.f, 0.f, 0.f, 
		0.f, 0.f, 0.f, 0.f, 
		0.f, 0.f, 0.f, 0.f, 
		0.5f, 0.5f, 0.5f, 0.5f );
}

// -------------------------------------------------------------
// Description:
//   Creates or updates an Umbra object and adds it to specified
//   cell. Used for doors by physical portals.
// Arguments:
//   cell - The cell to add object to
//   object - Umbra object that supposed to represent this dynamic
// -------------------------------------------------------------
void Tr2IntSkinnedObject::UpdateUmbraObject( Umbra::Cell* cell, Umbra::Object*& object ) const
{
	Vector3 minBounds, maxBounds;
	if( !GetLocalBoundingBox( minBounds, maxBounds ) )
	{
		// Our bounding box is not ready
		return;
	}

	if( object == NULL )
	{
		object = Umbra::Object::create( m_umbraModel );
		object->setCell( cell );
		object->setUserPointer( CONVERT_TO_UMBRA_USER_DATA( GetRawRoot() ) );
	}

	Matrix transformInv;
	cell->getCellToWorldMatrix( AS_UMBRA_MATRIX( transformInv ) );
	
	XMVECTOR det;
	Matrix m( XMMatrixMultiply( GetSkinningTransform(), XMMatrixInverse( &det, transformInv ) ) );

	object->setObjectToCellMatrix( AS_UMBRA_MATRIX( m ) );
}

// ------------------------------------------------------------------------------------------------------
bool Tr2IntSkinnedObject::DoVisualizeLightProbes( void ) const
{
	// For now, we only support visualizing light probes on Tr2InteriorPlaceables, not
	// Tr2IntSkinnedObjects
	return false;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2IntSkinnedObject::AddToScene( Tr2ApexScene* apexScene )
{
	if( !IsBoundingBoxReady() )
	{
		return false;
	}

	ClearUmbra();
	RebuildVolume();

	AddToApexScene( apexScene );

	m_isDirty = true;

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::RemoveFromScene( void )
{
	// Clean up our Umbra data
	ClearUmbra();

	// Get out of the Apex scene
	RemoveFromApexScene();
}

// ------------------------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::SetVisibleLightCount( int visibleLightCount )
{
	m_visibleLightCount = visibleLightCount;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the skinned object using a per-instance light-set override 
//    and an arbitrary object-to-world matrix.  Routes the call to either 
//    GetPerObjectDataGpuSkinning or GetPerObjectDataCpuSkinning, depending on the 
//    skinnning strategy used.
//  See Also:
//    GetPerObjectData, GetPerObjectDataGpuSkinning, GetPerObjectDataCpuSkinning
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//    lightSet -            The set of lights illuminating this object
//    objectToWorldMatrix - The transformation matrix used to position this object 
//                          in world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2IntSkinnedObject::GetPerObjectDataWithPerInstanceLighting( 
	ITriRenderBatchAccumulator* accumulator,
	Tr2InteriorLightSet* lightSet,
	const Matrix& objectToWorldMatrix,
	const Matrix& mirrorToWorldMatrix )
{
	// We support cpu AND gpu skinning, so decide which ::GetPerObjectData() to call 
	// based on the type of the Tr2 class
	if( m_visualModel )
	{
		D3DXMatrixInverse( &m_mirrorToWorldMatrix, NULL, D3DXMatrixTranspose( &m_mirrorToWorldMatrix, &mirrorToWorldMatrix ) );

		if( m_visualModel->ClassType() == Tr2SkinnedModel::ClassType_() )
		{
			return GetPerObjectDataGpuSkinning( accumulator, 
												lightSet, 
												objectToWorldMatrix, 
												mirrorToWorldMatrix );
		}
		else
		{
			return GetPerObjectDataCpuSkinning( accumulator, 
												lightSet, 
												objectToWorldMatrix, 
												mirrorToWorldMatrix );
		}
	}
	return NULL;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the skinned object using prepass-optimized per-object data 
//    and an arbitrary object-to-world matrix.  Routes the call to either 
//    GetPerObjectDataGpuSkinning or GetPerObjectDataCpuSkinning, depending on the 
//    skinnning strategy used.
//  See Also:
//    GetPerObjectData, GetPerObjectDataGpuSkinning, GetPerObjectDataCpuSkinning
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//    objectToWorldMatrix - The transformation matrix used to position this object 
//                          in world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2IntSkinnedObject::GetPerObjectDataForPrePass( 
	ITriRenderBatchAccumulator* accumulator,
	const Matrix& objectToWorldMatrix )
{
	// We support cpu AND gpu skinning, so decide which ::GetPerObjectData() to call 
	// based on the type of the Tr2 class
	if( m_visualModel )
	{
		Matrix mirrorMatrix;
		D3DXMatrixInverse( &mirrorMatrix, NULL, &m_transform );
		mirrorMatrix = mirrorMatrix * objectToWorldMatrix;
		D3DXMatrixTranspose( &mirrorMatrix, &mirrorMatrix );

		if( m_visualModel->ClassType() == Tr2SkinnedModel::ClassType_() )
		{
			return GetPerObjectDataForPrePassGpuSkinning( 
				accumulator, 
				objectToWorldMatrix,
				mirrorMatrix );
		}
		else
		{
			return GetPerObjectDataForPrePassCpuSkinning( 
				accumulator, 
				objectToWorldMatrix,
				mirrorMatrix );
		}
	}
	return NULL;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Called by Tr2SkinnedObject whenever its explicit bounding box changes. Reconstructs
//    Umbra model/object and marks this object as dirty.
// --------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::ExplicitBoundsChanged()
{
	m_isDirty = true;

	RebuildVolume();

	for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
		it != m_umbraObjects.end(); ++it )
	{
		Matrix m = GetSkinningTransform();
		XMVECTOR det;

		if( ( *it )->getCell() )
		{
			Matrix cellTransform;
			( *it )->getCell()->getCellToWorldMatrix( AS_UMBRA_MATRIX( cellTransform ) );
			m = XMMatrixMultiply( m, XMMatrixInverse( &det, cellTransform ) );
		}

		( *it )->setObjectToCellMatrix( AS_UMBRA_MATRIX( m ) );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::SetPosition( const Vector3 &pos )
{
	if( ( m_currentPosition != pos ) || !m_positionSet )
	{
		m_currentPosition = pos;
		m_positionSet = true;

		m_transform._41 = pos.x;
		m_transform._42 = pos.y;
		m_transform._43 = pos.z;

		UpdateUmbraTransforms();

		m_isDirty = true;
	}
}

// -----------------------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::SetRotation( const Quaternion& rotQuat )
{
	if( ( m_currentRotation != rotQuat ) || !m_rotationSet )
	{
		m_currentRotation = rotQuat;
		m_rotationSet = true;

		Vector3		tmpScale;		
		Quaternion	tmpRotation;	
		Vector3		tmpTranslation;	

		D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );
		D3DXMatrixTransformation( &m_transform, NULL, NULL, &tmpScale, NULL, &rotQuat, &tmpTranslation );

		UpdateUmbraTransforms();

		m_isDirty = true;
	}
}

// -----------------------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::SetScaling( const Vector3& scaleVec )
{
	if( ( m_currentScaling != scaleVec ) || !m_scalingSet )
	{
		m_currentScaling = scaleVec;
		m_scalingSet = true;

		Vector3		tmpScale;		
		Quaternion	tmpRotation;	
		Vector3		tmpTranslation;	

		D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );
		D3DXMatrixTransformation( &m_transform, NULL, NULL, &scaleVec, NULL, &tmpRotation, &tmpTranslation );

		UpdateUmbraTransforms();

		m_isDirty = true;
	}
}

// -----------------------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::SetLOD( const TriFrustum* frustum )
{
	return Tr2SkinnedObject::SetLOD( frustum );
}

// -----------------------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::ClearUmbra( void )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
		 it != m_umbraObjects.end(); ++it )
	{
		( *it )->setCell( NULL );
		( *it )->release();
		( *it ) = NULL;
	}

	m_umbraObjects.clear();

	if( m_umbraModel )
	{
		m_umbraModel->release();
		m_umbraModel = NULL;
	}
}

// -----------------------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::RebuildVolume( void )
{
	Vector3 minBound( 0.0f, 0.0f, 0.0f );
	Vector3 maxBound( 0.0f, 0.0f, 0.0f );

	// if there is no bounding box available, we have to delay the adding...
	GetLocalBoundingBox( minBound, maxBound );

	// everything is there, so use bounding box
	if( m_umbraModel )
	{
		m_umbraModel->release();
		m_umbraModel = NULL;
	}

	m_umbraModel = ( Umbra::Model* )Umbra::OBBModel::create( 
		AS_UMBRA_VECTOR3( minBound ), AS_UMBRA_VECTOR3( maxBound ) );

	for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
		 it != m_umbraObjects.end(); ++it )
	{
		( *it )->setTestModel( m_umbraModel );
		( *it )->setUserPointer( CONVERT_TO_UMBRA_USER_DATA( this ) );
	}
}

void Tr2IntSkinnedObject::UpdateUmbraTransforms( void )
{
	for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
		it != m_umbraObjects.end(); ++it )
	{
		Matrix m = GetSkinningTransform();
		XMVECTOR det;

		if( ( *it )->getCell() )
		{
			Matrix cellTransform;
			( *it )->getCell()->getCellToWorldMatrix( AS_UMBRA_MATRIX( cellTransform ) );
			m = XMMatrixMultiply( m, XMMatrixInverse( &det, cellTransform ) );
		}

		( *it )->setObjectToCellMatrix( AS_UMBRA_MATRIX( m ) );
	}
}


// IBluePlacementObserver
// -----------------------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	// Terrible workaround since the UpdatePlacement takes in a position and a front vector. 
	// We have to work out way back to a quaternion
	Vector3 temp = Vector3(front);
	if (temp.z >= 1.0f)
	{
		temp.z = 1.0f;
	}
	else if (temp.z <= -1.0f)
	{
		temp.z = -1.0f;
	}

	float yaw = acosf(temp.z);
	// This is actually two answers, we need to determine which side of the axis we're on. 
	// Correct for the other half of the circle
	if (temp.x < 0.0f)
	{
		yaw = ( 2 * 3.14159265f ) - yaw;
	}

	// OK, this should go away once our rotations are sorted out !!!!
	yaw += 3.14159265f;

	Quaternion q;
	Vector3 up = Vector3(0, 1, 0);
	D3DXQuaternionRotationYawPitchRoll( &q, yaw, 0, 0 );

	SetRotation(q);
	SetPosition(pos);
}

#endif
