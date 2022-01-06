#include "StdAfx.h"

#include "Tr2IntSkinnedObject.h"

#include "Utilities/BoundingSphere.h"
#include "TriSettingsRegistrar.h"
#include "Tr2PerObjectData.h"
#include "Resources/TriGeometryRes.h"
#include "Resources/TriTextureRes.h"
#include "Shader/Tr2Effect.h"
#include "Tr2Mesh.h"

CCP_STATS_DECLARE( wodIntSkinnedObjectsAlive, "Trinity/wodIntSkinnedObjectsAlive", false, CST_COUNTER_LOW, "Count of Tr2IntSkinnedObjects alive" );

Tr2IntSkinnedObject::Tr2IntSkinnedObject( IRoot* lockobj ) :
	Tr2SkinnedObject( lockobj ),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_lightSet(),
	m_isInApexScene( false ),
	m_cellReflectionTime( 0.0f ),
	m_previousUpdateTime( 0 ),
	m_depthOffset( 0.f ),
	m_currentPosition( 0.0f, 0.0f, 0.0f ),
	m_currentScaling( 1.0f, 1.0f, 1.0f ),
	m_currentRotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_positionSet( false ),
	m_scalingSet( false ),
	m_rotationSet( false )
{
	m_variableStore.CreateInstance();
	m_variableStore->SetParentVariableStore( &GlobalStore() );

	m_variableStore->RegisterVariable( "CellReflectionMap", (TriTextureRes*)NULL );
	m_variableStore->RegisterVariable( "CellReflection2ndMap", (TriTextureRes*)NULL );
	m_variableStore->RegisterVariable( "CellReflectionInterpolation", 0.0f );

	CCP_STATS_INC( wodIntSkinnedObjectsAlive );
}

Tr2IntSkinnedObject::~Tr2IntSkinnedObject()
{
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
}

void Tr2IntSkinnedObject::PostPhysicsUpdate( Be::Time time, Tr2ApexScene* apexScene )
{
	Tr2SkinnedObject::PostPhysicsUpdate( time, apexScene );

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

void Tr2IntSkinnedObject::AddToApexScene( Tr2ApexScene* apexScene )
{
	if( !m_isInApexScene )
	{
		m_isInApexScene = true;
	}
}

void Tr2IntSkinnedObject::RemoveFromApexScene( void )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_isInApexScene )
	{
	}
}

Tr2PerObjectData* Tr2IntSkinnedObject::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	if( m_visualModel && DoDisplay() )
	{
		return GetPerObjectDataGpuSkinning( accumulator, &m_lightSet, GetSkinningTransform() );
	}
	return NULL;
}

extern int g_maxClothLod;

void Tr2IntSkinnedObject::GetBatches( ITriRenderBatchAccumulator* batches,
								   TriBatchType batchType,
								   const Tr2PerObjectData* perObjectData, 
								   Tr2RenderReason reason)
{
	if( !DoDisplay() )
	{
		return;
	}
	unsigned int depth = 0xFFFFFFFF;
	if( batchType == TRIBATCHTYPE_TRANSPARENT )
	{
		float maxDepth = Tr2Renderer::GetFrustumRadius();
		Matrix instanceTransform = m_transform;

		// Compute the depth
		Vector3 bbMin, bbMax;
		GetLocalBoundingBox( bbMin, bbMax );
		Vector3 center = TransformCoord( 0.5f * ( bbMin + bbMax ), instanceTransform );
		center -= Tr2Renderer::GetViewPosition();
		float z = std::min( std::max( ( Length( center ) + m_depthOffset ) / maxDepth, 0.f ), 1.f );

		depth = ( unsigned int )( ( float )0xFFFFFFF * ( 1.0f - z ) );
	}
	Matrix* pm = batches->Allocate<Matrix>();

	CCP_ASSERT_M( pm, "No memory available for allocation of batches." );
	if( pm == NULL )
	{
		return;
	}
	*pm = GetSkinningTransform();

	const Tr2PerObjectDataSkinned* skinnedData = static_cast<const Tr2PerObjectDataSkinned*>( perObjectData );

	for( PTr2MeshVector::const_iterator meshIt = m_visualModel->GetMeshes().begin(); meshIt != m_visualModel->GetMeshes().end(); ++meshIt )
	{
		Tr2Mesh* mesh = *meshIt;
		if( mesh )
		{
			Tr2MeshAreaVector* areas = mesh->GetAreas( batchType );
			if( areas )
			{
				if( !mesh->GetDisplay() )
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
					auto shader = area->GetMaterialInterface();
					if( !area->GetDisplay() || ( !shader ) )
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
								if( areaRes && meshData->m_hasPerMeshAreaBoneBindings )
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

						batch->SetShaderMaterial( shader );
						batch->SetPerObjectData( areaData );
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

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for populating per-object data with a skinning matrix palette for 
//   GPU-skinning.  This uses instanced lighting.
// Arguments:
//   accumulator		 - The accumulator used to allocate the per-object data
//   lightSet			 - The instanced lights
//   objectToWorldMatrix - The world transform of the object
// Return Value:
//   The allocated per-object data, or NULL if the allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2IntSkinnedObject::GetPerObjectDataGpuSkinning( 
	ITriRenderBatchAccumulator* accumulator,
	Tr2InteriorLightSet* lightSet,
	const Matrix& objectToWorldMatrix )
{
	UpdatePerObjectData();

	Tr2PerObjectDataSkinned* data = accumulator->Allocate<Tr2PerObjectDataSkinned>();

	if( !data )
	{
		CCP_ASSERT( !"Not enough memory for skinning!" );
		return NULL;	// should not happen (allocator out of memory??) but if it does, let's not crash.
	}

	data->SetSkinningMatrices( m_skinningMatrixCount, GetSkinningMatrices() );
	data->SetWorldMatrix( objectToWorldMatrix );
	data->SetMirrorMatrix( IdentityMatrix() );


	// Pixel Shader Light information
	Tr2InteriorPerObjectPSData perObjectPSBuffer;

	// 0
	memset( &perObjectPSBuffer, 0, sizeof( perObjectPSBuffer ) );

	// put pointlights in perobject data
	if( lightSet )
	{
		lightSet->PopulateLightData( &perObjectPSBuffer );
	}

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	return data;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2IntSkinnedObject::AddToScene( Tr2ApexScene* apexScene )
{
	if( !IsBoundingBoxReady() )
	{
		return false;
	}

	AddToApexScene( apexScene );

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::RemoveFromScene( void )
{
	// Get out of the Apex scene
	RemoveFromApexScene();
}

// --------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the skinned object using a per-instance light-set override 
//    and an arbitrary object-to-world matrix.  
//  See Also:
//    GetPerObjectData, GetPerObjectDataGpuSkinning
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
	const Matrix& objectToWorldMatrix )
{
	if( m_visualModel )
	{
		return GetPerObjectDataGpuSkinning( accumulator, lightSet, objectToWorldMatrix );
	}
	return NULL;
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

		Decompose( tmpScale, tmpRotation, tmpTranslation, m_transform );
		static_cast<Matrix&>( m_transform ) = TransformationMatrix( tmpScale, rotQuat, tmpTranslation );
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

		Decompose( tmpScale, tmpRotation, tmpTranslation, m_transform );
		static_cast<Matrix&>( m_transform ) = TransformationMatrix( scaleVec, tmpRotation, tmpTranslation );
	}
}

// -----------------------------------------------------------------------------------------------------
void Tr2IntSkinnedObject::SetLOD( const TriFrustum* frustum )
{
	return Tr2SkinnedObject::SetLOD( frustum );
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
	q = RotationQuaternion( yaw, 0, 0 );

	SetRotation(q);
	SetPosition(pos);
}

#include "TriFrustum.h"

bool Tr2IntSkinnedObject::IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const
{
	Vector3 minBounds, maxBounds;
	if( !GetWorldBoundingBox( minBounds, maxBounds ) )
	{
		return false;
	}
	objectToWorld = m_transform;
	return frustum.IsBoxVisible( minBounds, maxBounds );
}

void Tr2IntSkinnedObject::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
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