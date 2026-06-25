// Copyright © 2023 CCP ehf.

// Precompiled header
#include "StdAfx.h"

// Tr2InteriorPlaceable.h header
#include "Tr2InteriorPlaceable.h"

// Trinity headers
#include "Utilities/BoundingSphere.h"
#include "Tr2PerObjectData.h"
#include "Wod/WodPlaceableRes.h"
#include "Tr2Mesh.h"
#include "Curves/TriCurveSet.h"
#include "TriFrustum.h"
#include "Resources/TriTextureRes.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2VariableStore.h"
#include "Tr2Renderer.h"


CCP_STATS_DECLARE( wodInteriorPlaceablesAlive, "Trinity/Tr2InteriorPlaceables", false, CST_COUNTER_LOW, "Count of Tr2InteriorPlaceables alive" );

Tr2InteriorPlaceable::Tr2InteriorPlaceable( IRoot* lockobj ) :
	m_display( true ),
	m_isUniqueInstance( false ),
	PARENTLOCK( m_transform, IInitialize ),
	m_placeableResPath(),
	m_placeableRes(),
	m_lightSet(),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_isBoundingBoxModified( false ),
	m_cellReflectionTime( 0.0f ),
	m_previousUpdateTime( 0 ),
	m_transitionFinished( false ),
	m_probeOffset( 0.f, 0.f, 0.f ),
	m_depthOffset( 0.f )
{
	static_cast<Matrix&>( m_transform ) = IdentityMatrix();

	m_currentPosition = Vector3( 0.0f, 0.0f, 0.0f );
	m_currentScaling = Vector3( 1.0f, 1.0f, 1.0f );
	m_currentRotation = Quaternion( 0.0f, 0.0f, 0.0f, 1.0f );
	m_positionSet = false;
	m_scalingSet = false;
	m_rotationSet = false;

	BoundingBoxInitialize( m_minBounds, m_maxBounds );

	m_variableStore.CreateInstance();

	m_variableStore->RegisterVariable( "CellReflectionMap", (TriTextureRes*)NULL );
	m_variableStore->RegisterVariable( "CellReflection2ndMap", (TriTextureRes*)NULL );
	m_variableStore->RegisterVariable( "CellReflectionInterpolation", 0.0f );

	CCP_STATS_INC( wodInteriorPlaceablesAlive );
}

Tr2InteriorPlaceable::~Tr2InteriorPlaceable()
{
	CCP_STATS_DEC( wodInteriorPlaceablesAlive );
}

bool Tr2InteriorPlaceable::AddToScene( Tr2ApexScene* apexScene )
{
	if( !IsBoundingBoxReady() )
	{
		return false;
	}

	RebuildVolume();

	return true;
}

void Tr2InteriorPlaceable::RemoveFromScene( void )
{
}

bool Tr2InteriorPlaceable::GetLocalBoundingBox( Vector3& min, Vector3& max ) const
{
	if( m_isBoundingBoxModified )
	{
		min = m_minBounds;
		max = m_maxBounds;
		return true;
	}
	// Pass down to the WodPlaceableRes
	if( m_placeableRes && m_placeableRes->IsReady() )
	{
		m_placeableRes->GetBoundingBox( min, max );
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Overrides the object's bounding box with the one provided
// Arguments:
//   min - bounding box's minimum bounds
//   max - bounding box's minimum bounds
// --------------------------------------------------------------------------------
void Tr2InteriorPlaceable::BoundingBoxOverride( Vector3& min, Vector3& max )
{
	m_minBounds = min;
	m_maxBounds = max;
	m_isBoundingBoxModified = true;

	RebuildVolume();
}

// --------------------------------------------------------------------------------
// Description:
//   Resets the object's bounding box, removing any overrides.
// --------------------------------------------------------------------------------
void Tr2InteriorPlaceable::BoundingBoxReset()
{
	m_isBoundingBoxModified = false;
	BoundingBoxInitialize( m_minBounds, m_maxBounds );

	RebuildVolume();
}

bool Tr2InteriorPlaceable::GetWorldBoundingBox( Vector3& min, Vector3& max ) const
{
	// Get the local bounding box min & max
	if( !GetLocalBoundingBox( min, max ) )
	{
		return false;
	}

	BoundingBoxTransform( min, max, m_transform );

	return true;
}

bool Tr2InteriorPlaceable::IsBoundingBoxReady( void ) const
{
	return ( m_placeableRes && m_placeableRes->IsReady() );
}

void Tr2InteriorPlaceable::PrePhysicsUpdate( Be::Time time )
{
}

void Tr2InteriorPlaceable::PostPhysicsUpdate( Be::Time time, Tr2ApexScene* apexScene )
{
	if( m_placeableRes )
	{
		for( TriCurveSetVector::const_iterator it = m_placeableRes->GetCurveSets()->begin(); it != m_placeableRes->GetCurveSets()->end(); ++it )
		{
			( *it )->Update( TimeAsDouble( time ) );
		}
	}

	m_cellReflectionTime += TimeAsFloat( time - m_previousUpdateTime );
	if( m_cellReflectionTime > 1.0f )
	{
		// This is to support 1 reflection map per placeable
		if( !m_transitionFinished )
		{
			std::swap( m_cellReflectionMaps[0], m_cellReflectionMaps[1] );
			m_transitionFinished = true;
		}
	}
	m_variableStore->RegisterVariable( "CellReflectionMap", m_cellReflectionMaps[0] );
	m_variableStore->RegisterVariable( "CellReflection2ndMap", m_cellReflectionMaps[1] );
	m_variableStore->RegisterVariable( "CellReflectionInterpolation", m_cellReflectionTime );
	m_previousUpdateTime = time;
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds per-cell reflection map to an array of current reflection maps.
// Arguments:
//   texture - Per-cell reflection map
// --------------------------------------------------------------------------------------
void Tr2InteriorPlaceable::AddReflectionMap( TriTextureRes* texture )
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
		m_transitionFinished = true;
	}
	else if( m_cellReflectionMaps[1] == NULL )
	{
		m_cellReflectionMaps[1] = texture;
		m_cellReflectionTime = 0.0f;
		m_transitionFinished = false;
	}
	else
	{
		m_cellReflectionMaps[0] = m_cellReflectionMaps[1];
		m_cellReflectionMaps[1] = texture;
		m_cellReflectionTime = 0.0f;
		m_transitionFinished = false;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes per-cell reflection map from an array of current reflection maps.
// Arguments:
//   texture - Per-cell reflection map
// --------------------------------------------------------------------------------------
void Tr2InteriorPlaceable::RemoveReflectionMap( TriTextureRes* texture )
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
		m_transitionFinished = false;
	}
}

void Tr2InteriorPlaceable::SetLOD( const TriFrustum* frustum )
{
	// TODO_delder: implement LOD?
}

// --------------------------------------------------------------------------------------
//  Description:
//    Used to notify the Tr2InteriorPlaceable when the placeable res path has changed,
//    or if we're turning this placeable into a unique placeable.
//  @@ INotify
// --------------------------------------------------------------------------------------
bool Tr2InteriorPlaceable::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_placeableResPath ) )
	{
		LoadPlaceableRes();
	}
	else if( IsMatch( value, m_isUniqueInstance ) )
	{
		if( m_placeableRes && m_isUniqueInstance )
		{
			// not worth doing anything if we don't already have something loaded
			// if we do, take a copy of what's currently there
			IRootPtr copyOfOriginal = NULL;
			BeClasses->CloneTo( m_placeableRes, &copyOfOriginal.p );
			m_placeableRes.Unlock();
			BlueQIPtrAssign( (IRoot**)&m_placeableRes.p, copyOfOriginal, BlueInterfaceIID<WodPlaceableRes>() );
		}
		else
		{
			LoadPlaceableRes();
		}
	}

	return true;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Function called when a Tr2InteriorPlaceable is created in Python.
//  Return Value:
//    true, for successful initialization
//    false, for initialization failure
//  @@ IInitialize
// --------------------------------------------------------------------------------------
bool Tr2InteriorPlaceable::Initialize( void )
{
	LoadPlaceableRes();
	return true;
}

bool Tr2InteriorPlaceable::HasTransparentBatches( void )
{
	if( m_placeableRes )
	{
		return m_placeableRes->HasTransparency();
	}
	return false;
}

void Tr2InteriorPlaceable::GetBatches( ITriRenderBatchAccumulator* batches,
									   TriBatchType batchType,
									   const Tr2PerObjectData* data,
									   Tr2RenderReason reason )
{
	if( !m_display )
	{
		return;
	}

	float maxDepth = Tr2Renderer::GetFrustumRadius();
	Matrix instanceToWorld = m_transform;

	if( m_placeableRes )
	{
		// Get the model from the res
		Tr2Model* model = m_placeableRes->GetVisualModel();
		if( model )
		{
			unsigned int numMeshes = model->GetNumOfMeshes();

			for( unsigned int i = 0; i < numMeshes; ++i )
			{
				Tr2Mesh* mesh = model->GetMesh( i );

				// Only gather transparent batches if the mesh isn't hidden
				if( !mesh->GetDisplay() )
				{
					continue;
				}
				auto geometry = mesh->GetGeometryResource();
				if( !geometry || !geometry->IsGood() )
				{
					continue;
				}
				auto lod = geometry->GetMeshLod( mesh->GetMeshIndex(), 0 );
				if( !lod || !lod->m_allocationsValid )
				{
					continue;
				}

				// Get the transparent areas
				Tr2MeshAreaVector* areas = mesh->GetAreas( batchType );

				if( areas )
				{
					// Loop over the transparent areas
					for( auto& area : *areas )
					{
						auto shader = area->GetMaterialInterface();

						// If the area isn't hidden & has an effect
						if( !area->GetDisplay() || !shader )
						{
							continue;
						}

						unsigned int depth = 0;
						if( batchType == TRIBATCHTYPE_TRANSPARENT )
						{
							// Compute the depth
							Vector3 center;
							if( m_isBoundingBoxModified )
							{
								center = 0.5f * ( m_minBounds + m_maxBounds );
							}
							else
							{
								center = mesh->GetAreaBounds( area->GetIndex() ).Center();
							}
							center = TransformCoord( center, instanceToWorld );
							center -= Tr2Renderer::GetViewPosition();
							float z = std::min( std::max( ( Length( center ) + m_depthOffset ) / maxDepth, 0.f ), 1.f );

							depth = (unsigned int)( (float)0xFFFFFFF * ( 1.0f - z ) );
						}

						Tr2RenderBatch batch = CreateGeometryBatch( lod, area, data );
						batch.m_depth = depth;
						batches->Commit( batch );
					}
				}
			}
		}
	}
}

float Tr2InteriorPlaceable::GetSortValue( void )
{
	return CalculateCameraDistance();
}

Tr2PerObjectData* Tr2InteriorPlaceable::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return GetPerObjectDataWithLightSet( accumulator, &m_lightSet, m_transform );
}

std::string Tr2InteriorPlaceable::GetPlaceableResPath( void ) const
{
	return m_placeableResPath;
}

void Tr2InteriorPlaceable::SetPlaceableResPath( const std::string& val )
{
	m_placeableResPath = val;
	LoadPlaceableRes();
}

void Tr2InteriorPlaceable::SetPosition( const Vector3& pos )
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

const Quaternion Tr2InteriorPlaceable::GetRotation( void ) const
{
	Vector3 tmpScale;
	Quaternion tmpRotation;
	Vector3 tmpTranslation;

	Decompose( tmpScale, tmpRotation, tmpTranslation, m_transform );

	return tmpRotation;
}

void Tr2InteriorPlaceable::SetRotation( const Quaternion& rotQuat )
{
	if( ( m_currentRotation != rotQuat ) || !m_rotationSet )
	{
		m_currentRotation = rotQuat;
		m_rotationSet = true;

		Vector3 tmpScale;
		Quaternion tmpRotation;
		Vector3 tmpTranslation;

		Decompose( tmpScale, tmpRotation, tmpTranslation, m_transform );
		static_cast<Matrix&>( m_transform ) = TransformationMatrix( tmpScale, rotQuat, tmpTranslation );
	}
}

const Vector3 Tr2InteriorPlaceable::GetScaling( void ) const
{
	Vector3 tmpScale;
	Quaternion tmpRotation;
	Vector3 tmpTranslation;

	Decompose( tmpScale, tmpRotation, tmpTranslation, m_transform );

	return tmpScale;
}

void Tr2InteriorPlaceable::SetScaling( const Vector3& scaleVec )
{
	if( ( m_currentScaling != scaleVec ) || !m_scalingSet )
	{
		m_currentScaling = scaleVec;
		m_scalingSet = true;

		Vector3 tmpScale;
		Quaternion tmpRotation;
		Vector3 tmpTranslation;

		Decompose( tmpScale, tmpRotation, tmpTranslation, m_transform );
		static_cast<Matrix&>( m_transform ) = TransformationMatrix( scaleVec, tmpRotation, tmpTranslation );
	}
}

float Tr2InteriorPlaceable::CalculateCameraDistance( void )
{
	Vector3 cameraPos = Tr2Renderer::GetViewPosition();

	cameraPos.x -= m_transform._41;
	cameraPos.y -= m_transform._42;
	cameraPos.z -= m_transform._43;

	return LengthSq( cameraPos );
}

// --------------------------------------------------------------------------------
// Description:
//   Calculates actual bounds for the object using the placeable res and
//   attached objects
// Arguments:
//   minBounds - bounding box's minimum bounds
//   maxBounds - bounding box's minimum bounds
// --------------------------------------------------------------------------------
void Tr2InteriorPlaceable::CalculateBoundingBox( Vector3& minBounds, Vector3& maxBounds )
{
	Vector3 min, max;
	GetLocalBoundingBox( min, max );
	BoundingBoxUpdate( minBounds, maxBounds, min, max );
}

void Tr2InteriorPlaceable::RebuildVolume( void )
{
	Vector3 minBounds( FLT_MAX, FLT_MAX, FLT_MAX ), maxBounds( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	if( !IsBoundingBoxReady() )
	{
		return;
	}

	if( m_isBoundingBoxModified )
	{
		minBounds = m_minBounds;
		maxBounds = m_maxBounds;
	}
	else
	{
		CalculateBoundingBox( minBounds, maxBounds );
	}
}

// --------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the placeable using a per-instance light-set override and
//    an arbitrary object-to-world matrix.  Routes the call to helper function
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
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorPlaceable::GetPerObjectDataWithPerInstanceLighting(
	ITriRenderBatchAccumulator* accumulator,
	Tr2InteriorLightSet* lightSet,
	const Matrix& objectToWorldMatrix )
{
	return GetPerObjectDataWithLightSet( accumulator,
										 lightSet,
										 objectToWorldMatrix );
}

// ------------------------------------------------------------------------------------------------------
//  Description:
//    Helper function to get per-object data for this renderable using an arbitrary light-set and object-
//    to-world matrix.  Both GetPerObjectData and GetPerObjectDataWithPerInstanceLighting are thin
//    wrappers around this function.
//  See Also:
//    GetPerObjectData, GetPerObjectDataWithPerInstanceLighting
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//    lightSet -            The set of lights illuminating this object
//    objectToWorldMatrix - The transformation matrix used to position this object in world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// -----------------------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorPlaceable::GetPerObjectDataWithLightSet( ITriRenderBatchAccumulator* accumulator,
																	  Tr2InteriorLightSet* lightSet,
																	  const Matrix& objectToWorldMatrix )
{
	Tr2PerObjectDataStandard* data = accumulator->Allocate<Tr2PerObjectDataStandard>();

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

	// column_major for shaders
	perObjectVSBuffer.WorldMat = Transpose( objectToWorldMatrix );

	// put pointlights in perobject data
	if( lightSet )
	{
		lightSet->PopulateLightData( &perObjectPSBuffer );
	}

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );
	data->CopyToVSFloatBuffer( perObjectVSBuffer );

	return data;
}

// --------------------------------------------------------------------------------------
// Description:
//   Loads a copy of a placeableRes from the res path.
// --------------------------------------------------------------------------------------
void Tr2InteriorPlaceable::LoadPlaceableRes()
{
	m_placeableRes.Unlock();
	m_placeableRes = BeResMan->LoadObject<WodPlaceableRes>( m_placeableResPath.c_str() );
}

// -------------------------------------------------------------
// Description:
//   Blue-exposed function that returns AABB for the object in its
//   local coordinate space.
// Return value:
//   AABB for the object in its local coordinate space
// -------------------------------------------------------------
AxisAlignedBoundingBox Tr2InteriorPlaceable::GetBoundingBoxInLocalSpace() const
{
	AxisAlignedBoundingBox result( Vector3( 0.f, 0.f, 0.f ), Vector3( 0.f, 0.f, 0.f ) );
	GetLocalBoundingBox( result.m_min, result.m_max );
	return result;
}

// -------------------------------------------------------------
// Description:
//   Blue-exposed function that returns AABB for the object in the
//   world coordinate space.
// Return value:
//   AABB for the object in the world coordinate space
// -------------------------------------------------------------
AxisAlignedBoundingBox Tr2InteriorPlaceable::GetBoundingBoxInWorldSpace() const
{
	AxisAlignedBoundingBox result( Vector3( 0.f, 0.f, 0.f ), Vector3( 0.f, 0.f, 0.f ) );
	GetWorldBoundingBox( result.m_min, result.m_max );
	return result;
}

bool Tr2InteriorPlaceable::IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const
{
	Vector3 minBounds, maxBounds;
	if( !GetWorldBoundingBox( minBounds, maxBounds ) )
	{
		return false;
	}
	objectToWorld = m_transform;
	return frustum.IsBoxVisible( minBounds, maxBounds );
}

void Tr2InteriorPlaceable::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
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