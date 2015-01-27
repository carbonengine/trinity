// Precompiled header
#include "StdAfx.h"

// Tr2InteriorPlaceable.h header
#include "Tr2InteriorPlaceable.h"

// Trinity headers
#include "Utilities/BoundingSphere.h"
#include "Tr2LitPerObjectData.h"
#include "Tr2InteriorCell.h"
#include "Tr2InteriorMirror.h"
#include "Wod/WodPlaceableRes.h"
#include "TriLineSet.h"
#include "Tr2Mesh.h"
#include "Curves/TriCurveSet.h"
#include "TriFrustum.h"

CCP_STATS_DECLARE( wodInteriorPlaceablesAlive, "Trinity/Tr2InteriorPlaceables", false, CST_COUNTER_LOW, "Count of Tr2InteriorPlaceables alive" );

Tr2InteriorPlaceable::Tr2InteriorPlaceable( IRoot* lockobj ) :
    m_display( true ),
	m_isUniqueInstance( false ),
	PARENTLOCK( m_transform, IInitialize ),
	m_isDirty( true ),
	m_placeableResPath(),
	m_placeableRes(),
	m_lightSet(),
	m_visibilityMode( VISIBILITYMODE_NORMAL ),
	m_SHMatrixRed(),
	m_SHMatrixGreen(),
	m_SHMatrixBlue(),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_shSolver( NULL ),
	m_isStatic( false ),
	m_isBoundingBoxModified( false ),
	m_cellReflectionTime( 0.0f ),
	m_previousUpdateTime( 0 ),
	m_transitionFinished( false ),
	m_probeOffset( 0.f, 0.f, 0.f ),
	m_depthOffset( 0.f )
{
    D3DXMatrixIdentity( &m_transform );

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

	D3DXMatrixIdentity( &m_mirrorToWorldMatrix );

	CCP_STATS_INC( wodInteriorPlaceablesAlive );
}

Tr2InteriorPlaceable::~Tr2InteriorPlaceable()
{
	CCP_STATS_DEC( wodInteriorPlaceablesAlive );
}

bool Tr2InteriorPlaceable::AddToScene( Tr2ApexScene *apexScene )
{
	if( !IsBoundingBoxReady() )
	{
		return false;
	}

	RebuildVolume();
	CreateMirrors();
	
	m_isDirty = true;

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
	return( m_placeableRes && m_placeableRes->IsReady() );
}

bool Tr2InteriorPlaceable::GetShProbePosition( Vector3& position ) const
{
	Vector3 min, max;
	GetWorldBoundingBox( min, max );
	position = ( min + max ) * 0.5f + m_probeOffset;
	return true;
}

void Tr2InteriorPlaceable::PrePhysicsUpdate( Be::Time time )
{
}

void Tr2InteriorPlaceable::PostPhysicsUpdate( Be::Time time, Tr2ApexScene *apexScene )
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

// --------------------------------------------------------------------------------------
// Description:
//   Binds low-level shaders on all meshes of the placeable. Applies local variable
//	 store.
// --------------------------------------------------------------------------------------
void Tr2InteriorPlaceable::BindLowLevelShaders()
{
	std::vector<unsigned int> localFlags;
	unsigned int h = CcpHashFNV1( "DynamicObject", strlen( "DynamicObject" ) );
	localFlags.push_back( h );
	h = CcpHashFNV1( "Interior", strlen( "Interior" ) );
	localFlags.push_back( h );

	if( m_placeableRes && m_placeableRes->GetVisualModel() )
	{
		for( PTr2MeshVector::const_iterator meshIt = m_placeableRes->GetVisualModel()->GetMeshes().begin(); 
			 meshIt != m_placeableRes->GetVisualModel()->GetMeshes().end(); 
			 ++meshIt )
		{
			( *meshIt )->BindLowLevelShaders( localFlags, false, m_variableStore );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorPlaceable::TestCellIntersectionAndAdd( Tr2InteriorCell* cell )
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

	bool intersects = cell->IsUnbounded() || cell->IntersectsAABB( minBounds, maxBounds );

	// If we got an intersection, add to the cell
	if( intersects )
	{
		cell->AddDynamic( this );
		AddReflectionMap( cell->GetReflectionMap() );
	}
	else
	{
		// Remove the dynamic from the cell's internal list
		cell->RemoveDynamic( this );
		RemoveReflectionMap( cell->GetReflectionMap() );
	}

	// Return the result of the intersection test
	return intersects;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Query whether the placeable can cast shadows
//  Return Value:
//    true, if the placeable can cast shadows
//    false, if the placeable cannot cast shadows
// --------------------------------------------------------------------------------------
bool Tr2InteriorPlaceable::IsShadowCaster( void ) const
{
	if( m_placeableRes )
	{
		return m_placeableRes->IsShadowCaster();
	}
	return false;
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
	else if( IsMatch( value, m_transform ) )
    {
		m_isDirty = true;
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
			BlueQIPtrAssign( ( IRoot** )&m_placeableRes.p, copyOfOriginal, BlueInterfaceIID<WodPlaceableRes>() );
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
									   const Tr2PerObjectData* data )
{
	if( !m_display )
	{
		return;
	}

	if( batchType == TRIBATCHTYPE_MIRROR )
	{
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
					if( mesh->HasPendingLowLevelShaderBind() )
					{
						mesh->ExecutePendingLowLevelShaderBind();
					}

					if( mesh->IsHidden() )
					{
						continue;
					}
					Tr2MeshAreaVector* areas = mesh->GetAreas( TRIBATCHTYPE_MIRROR );

					TriGeometryRes* geomRes = mesh->GetGeometryResource();
					int meshIx = mesh->GetMeshIndex();

					for( Tr2MeshAreaVector::iterator it = areas->begin(); it != areas->end(); ++it )
					{
						Tr2MeshArea* area = *it;
						ITr2ShaderMaterial* shader = area->GetMaterialInterface();

						if( area->IsHidden() || !shader || 
							( area->GetIndex() != m_stencilParams.m_areaIx ) )
						{
							continue;
						}

						Tr2InteriorStencilMaskBatch* batch = batches->Allocate<Tr2InteriorStencilMaskBatch>();
						// Note that this can fail if the accumulator can't add more batches!
						if( batch )
						{
							const Tr2PerObjectData* perAreaData = data;
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


							batch->SetShaderMaterial( shader );
							batch->SetPerObjectData( perAreaData );
							batch->SetGeometryResource( geomRes );
							batch->SetMeshParameters( meshIx, area->GetIndex(), area->GetCount(), area->GetReversed() );

							batch->SetStencilValues( m_stencilParams.m_stencilWrite, 
								m_stencilParams.m_stencilTest );
							batch->SetDepthClear( m_stencilParams.m_depthClear );
							batch->SetStencilPassState( m_stencilParams.m_stencilPassState );
							batch->SetDisableStencil( false );
							batch->SetColorWrite( m_stencilParams.m_colorWrite );

							batches->Commit( batch );
						}
					}
				}
			}
		}
	}
	else if( m_visibilityMode != VISIBILITYMODE_HIDDEN )
	{
		float maxDepth = Tr2Renderer::GetFrustumRadius();
		Matrix instanceToWorld = m_transform * m_mirrorToWorldMatrix;

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
					if( !mesh->IsHidden() )
					{
						// Get the transparent areas
						Tr2MeshAreaVector* areas = mesh->GetAreas( batchType );

						if( areas )
						{
							// Loop over the transparent areas
							for( Tr2MeshAreaVector::const_iterator it = areas->begin(); it != areas->end(); ++it )
							{
								Tr2MeshArea* area = *it;
								ITr2ShaderMaterial* shader = area->GetMaterialInterface();

								// If the area isn't hidden & has an effect
								if( area->IsHidden() || !shader )
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
										Vector3 bbMin, bbMax;
										mesh->GetAreaBoundingBox( area->GetIndex(), bbMin, bbMax );
										center = 0.5f * ( bbMin + bbMax );
									}
									D3DXVec3TransformCoord( &center, &center, &instanceToWorld );
									center -= Tr2Renderer::GetViewPosition();
									float z = std::min( std::max( ( D3DXVec3Length( &center ) + m_depthOffset ) / maxDepth, 0.f ), 1.f );

									depth = ( unsigned int )( ( float )0xFFFFFFF * ( 1.0f - z ) );
								}

								const Tr2PerObjectData *perAreaData = data;

								if( m_shSolver && area->GetUseSHLighting() )
								{
									Tr2PerAreaSHLightingData* areaData = batches->Allocate<Tr2PerAreaSHLightingData>();
									if( areaData )
									{
										Vector3 minBounds, maxBounds;
										if( m_isBoundingBoxModified )
										{
											minBounds = m_minBounds;
											maxBounds = m_maxBounds;
										}
										else
										{
											mesh->GetAreaBoundingBox( area->GetIndex(), minBounds, maxBounds );
											for( int i = 1; i < area->GetCount(); ++i )
											{
												Vector3 min, max;
												mesh->GetAreaBoundingBox( area->GetIndex() + i, min, max );
												BoundingBoxUpdate( minBounds, maxBounds, min, max );
											}
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
									batch->SetShaderMaterial( shader );
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
	return GetPerObjectDataWithLightSet( accumulator, &m_lightSet, m_transform, Tr2Renderer::GetIdentityTransform() );
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

		for( std::vector<Tr2InteriorMirror*>::iterator mirrorIt = m_mirrors.begin(); 
			 mirrorIt != m_mirrors.end(); ++mirrorIt )
		{
			( *mirrorIt )->SetTransformMatrix( m_transform );
		}

		m_isDirty = true;
	}
}

const Quaternion Tr2InteriorPlaceable::GetRotation( void ) const
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	

	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );

	return tmpRotation;
}

void Tr2InteriorPlaceable::SetRotation( const Quaternion& rotQuat )
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

		for( std::vector<Tr2InteriorMirror*>::iterator mirrorIt = m_mirrors.begin(); 
			 mirrorIt != m_mirrors.end(); ++mirrorIt )
		{
			( *mirrorIt )->SetTransformMatrix( m_transform );
		}

		m_isDirty = true;
	}
}

const Vector3 Tr2InteriorPlaceable::GetScaling( void ) const
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	
	
	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );

	return tmpScale;
}

void Tr2InteriorPlaceable::SetScaling( const Vector3& scaleVec )
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

		for( std::vector<Tr2InteriorMirror*>::iterator mirrorIt = m_mirrors.begin(); mirrorIt != m_mirrors.end(); ++mirrorIt )
		{
			( *mirrorIt )->SetTransformMatrix( m_transform );
		}

		m_isDirty = true;
	}
}

float Tr2InteriorPlaceable::CalculateCameraDistance( void )
{
	Vector3 cameraPos = Tr2Renderer::GetViewPosition();

	cameraPos.x -= m_transform._41;
	cameraPos.y -= m_transform._42;
	cameraPos.z -= m_transform._43;

	return D3DXVec3LengthSq( &cameraPos );
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

void Tr2InteriorPlaceable::CreateMirrors()
{
	for( auto it = m_mirrors.begin(); it != m_mirrors.end(); ++it )
	{
		Tr2InteriorMirror* mirror = *it;
		CCP_DELETE( mirror );
	}
	m_mirrors.clear();

	Vector3 minBounds, maxBounds;

	Tr2ModelPtr model = m_placeableRes->GetVisualModel();

	for( unsigned int meshIx = 0; meshIx < model->GetNumOfMeshes(); ++meshIx )
	{
		Tr2Mesh* mesh = model->GetMesh( meshIx );
		if( !mesh )
			continue;

		Tr2MeshAreaVector* mirrorAreas = mesh->GetAreas( TRIBATCHTYPE_MIRROR );
		for( PTr2MeshAreaVector::iterator it = mirrorAreas->begin(); it != mirrorAreas->end(); ++it )
		{
			if( !mesh->GetAreaBoundingBox( ( *it )->GetIndex(), minBounds, maxBounds ) )
				continue;

			Tr2InteriorMirror* mirror = CCP_NEW("Tr2InteriorMirror" ) Tr2InteriorMirror();

			if( !mirror )
			{
				continue;
			}

			m_mirrors.push_back( mirror );

			// Set mirror bounding box
			mirror->SetBoundingBox( minBounds, maxBounds );

			// Set mesh and area indices
			mirror->SetMeshIndex( mesh->GetMeshIndex() );
			mirror->SetAreaIndex( ( *it )->GetIndex() );

			// Set placeable
			mirror->SetPlaceable( this );

			// Set index
			mirror->SetMirrorIndex( (unsigned int)m_mirrors.size() - 1 );

			// Compute warp matrices
			Matrix warpMatrixFront( XMMatrixIdentity() );
			Matrix warpMatrixBack( XMMatrixIdentity() );
			
			Vector3 edge1, edge2, pointOnTriangle;
			if( !mesh->GetAreaBasis( ( *it )->GetIndex(), pointOnTriangle, edge1, edge2 ) )
			{
				m_isDirty = true;
				return;
			}
			edge1 = XMVector3Normalize( edge1 );
			edge2 = XMVector3Normalize( edge2 );
			Vector3 normal( XMVector3Normalize( XMVector3Cross( edge1, edge2 ) ) );
			warpMatrixFront.GetX() = edge1;
			warpMatrixFront.GetY() = edge2;
			warpMatrixFront.GetZ() = normal;
			warpMatrixFront.GetTranslation() = pointOnTriangle;

			warpMatrixBack.GetX() = edge1;
			warpMatrixBack.GetY() = edge2;
			warpMatrixBack.GetZ() = -normal;
			warpMatrixBack.GetTranslation() = pointOnTriangle;

			mirror->SetWarpMatrixFront( warpMatrixFront );
			mirror->SetWarpMatrixBack( warpMatrixBack );

			// Set transformation
			mirror->SetTransformMatrix( m_transform );
		}
	}
}

Tr2InteriorMirror* Tr2InteriorPlaceable::GetMirror( size_t index ) const
{
	if( index < m_mirrors.size() )
	{
		return m_mirrors[index];
	}

	return NULL;
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
	const Matrix& objectToWorldMatrix,
	const Matrix& mirrorToWorldMatrix )
{
	return GetPerObjectDataWithLightSet( accumulator, 
										lightSet,
										objectToWorldMatrix, 
										mirrorToWorldMatrix );
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
																	  const Matrix& objectToWorldMatrix,
																	  const Matrix& mirrorToWorldMatrix )
{
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

	// column_major for shaders
	D3DXMatrixTranspose( &perObjectVSBuffer.WorldMat, &objectToWorldMatrix );

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

	D3DXMatrixInverse( &m_mirrorToWorldMatrix, NULL, D3DXMatrixTranspose( &m_mirrorToWorldMatrix, &mirrorToWorldMatrix ) );

	return data;
}

// --------------------------------------------------------------------------------------
// Description:
//   Loads a copy of a placeableRes from the res path.
// --------------------------------------------------------------------------------------
void Tr2InteriorPlaceable::LoadPlaceableRes()
{
	m_placeableRes.Unlock();

	IRootPtr p;
	p.Attach( BeResMan->LoadObject( m_placeableResPath.c_str() ) );

	m_placeableRes = BlueCastPtr( p );
	BindLowLevelShaders();
}

bool Tr2InteriorPlaceable::IsStatic( void ) const
{
	return m_isStatic;
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

