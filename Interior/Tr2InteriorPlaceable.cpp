// Precompiled header
#include "StdAfx.h"

#if INTERIORS_ENABLED

// Tr2InteriorPlaceable.h header
#include "Tr2InteriorPlaceable.h"

// Trinity headers
#include "Utilities/BoundingSphere.h"
#include "Tr2LitPerObjectData.h"
#include "Tr2InteriorCell.h"
#include "Tr2InteriorMirror.h"
#include "Wod/WodPlaceableRes.h"
#include "ITr2UmbraUserData.h"
#include "TriLineSet.h"
#include "Tr2Mesh.h"
#include "Curves/TriCurveSet.h"

CCP_STATS_DECLARE( wodInteriorPlaceablesAlive, "Trinity/Tr2InteriorPlaceables", false, CST_COUNTER_LOW, "Count of Tr2InteriorPlaceables alive" );

Tr2InteriorPlaceable::Tr2InteriorPlaceable( IRoot* lockobj ) :
    m_display( true ),
	m_isUniqueInstance( false ),
	m_isVisible( false ),
	m_umbraObjects(),
	m_umbraModel( NULL ),
	PARENTLOCK( m_transform, IInitialize ),
	m_isDirty( true ),
	m_placeableResPath(),
	m_placeableRes(),
	m_lightSet(),
	m_visualizeLightProbes( false ),
	m_visibilityMode( VISIBILITYMODE_NORMAL ),
	m_shSampleIndex( -1 ),
	m_SHMatrixRed(),
	m_SHMatrixGreen(),
	m_SHMatrixBlue(),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_drawLightSpider( false ),
	PARENTLOCK( m_attachedObjects ),
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
	m_visibleLightCount = 0;

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
	// Clear out Umbra data
	ClearUmbra();
	ClearMirrors();

	CCP_STATS_DEC( wodInteriorPlaceablesAlive );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorPlaceable::SetVisibility( bool bVisible )
{
	m_isVisible = bVisible;
}

bool Tr2InteriorPlaceable::DoVisualizeLightProbes( void ) const
{
	return m_visualizeLightProbes;
}

bool Tr2InteriorPlaceable::AddToScene( Tr2ApexScene *apexScene )
{
	if( !IsBoundingBoxReady() )
	{
		return false;
	}

	ClearUmbra();
	ClearMirrors();
	RebuildVolume();
	
	m_isDirty = true;

	return true;
}

void Tr2InteriorPlaceable::RemoveFromScene( void )
{
	ClearUmbra();
	ClearMirrors();
}

bool Tr2InteriorPlaceable::GetBoundingSphere( Vector4& sphere ) const
{
	if( m_boundingSphere.w > 0.f )
	{
		sphere = m_boundingSphere;
		BoundingSphereTransform( m_transform, sphere );
	}
	else
	{
		sphere = Vector4( GetPosition(), 2.0f );
	}
	return true;
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
	bool attachmentsDirty = false;
	for( ITr2RenderableVector::iterator it = m_attachedObjects.begin(); it != m_attachedObjects.end(); ++it )
	{
		ITr2InteriorAttachedObject *attachedObject = dynamic_cast<ITr2InteriorAttachedObject*>( *it );
		if( attachedObject )
		{
			attachedObject->SetSHProbeMatrices( m_SHMatrixRed, m_SHMatrixGreen, m_SHMatrixBlue );
			attachedObject->SetWorldTransform( m_transform );
			Vector3 minBounds, maxBounds;
			if( attachedObject->IsDirty() && attachedObject->GetBoundingBox( minBounds, maxBounds ) )
			{
				attachmentsDirty = true;
				break;
			}
		}
	}
	if( attachmentsDirty && IsBoundingBoxReady() )
	{
		RebuildVolume();
	}

	// Update Umbra
	if( !m_umbraObjects.empty() )
	{
		Matrix m = m_transform;

		
		for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
			 it != m_umbraObjects.end(); ++it )
		{
			Umbra::Object* object = *it;
			if( object->getCell() )
			{
				Matrix cellTransform;
				object->getCell()->getCellToWorldMatrix( AS_UMBRA_MATRIX( cellTransform ) );
				XMVECTOR det;
				m = XMMatrixMultiply( m, XMMatrixInverse( &det, cellTransform ) );
			}
			object->setObjectToCellMatrix( AS_UMBRA_MATRIX( m ) );
		}
	}

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
	Vector3 scale, translation;
	Quaternion rotation;
	D3DXMatrixDecompose( &scale, &rotation, &translation, &cell->GetWorldTransform() );
	Vector3 center = ( cellMinBounds + cellMaxBounds ) / 2 + translation;
	Vector3 extents = cellMaxBounds - center;

	bool intersects = cell->IsUnbounded() || 
		IntersectOrientedBoxAxisAlignedBox( center, extents, rotation, minBounds, maxBounds );

	// If we got an intersection, add to the cell
	if( intersects )
	{
		XMVECTOR det;
		Matrix m( XMMatrixMultiply( m_transform, 
			XMMatrixInverse( &det, cell->GetWorldTransform() ) ) );

		if( cell->AddDynamic( this ) )
		{
			// Add to the Umbra cell
			Umbra::Object* object = Umbra::Object::create( m_umbraModel );
			object->setCell( cell->GetUmbraCell() );

			object->setObjectToCellMatrix( AS_UMBRA_MATRIX( m ) );
			object->setUserPointer( CONVERT_TO_UMBRA_USER_DATA( this ) );
			m_umbraObjects.push_back( object );

			EnableMirrors( cell->GetUmbraCell() );
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

			for( std::vector<Tr2InteriorMirror*>::iterator it = m_mirrors.begin(); it != m_mirrors.end(); ++it )
			{
				( *it )->SetTransformMatrix( ( *it )->GetTransformMatrix() );
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
		std::vector<Tr2InteriorMirror*>::iterator it = m_mirrors.begin();
		while( it != m_mirrors.end() )
		{
			if( ( *it )->GetCell() == cell->GetUmbraCell() )
			{
				Tr2InteriorMirror* mirror = *it;
				it = m_mirrors.erase( it );
				CCP_DELETE( mirror );
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
void Tr2InteriorPlaceable::CellRemoved( Tr2InteriorCell* cell )
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
}

// --------------------------------------------------------------------------------------
// Description:
//   Clears the dirty flag.  Also determines whether to enable or disable mirrors.
//   Mirrors are only enabled when the placeable is in exactly one cell.  Any other cell
//   membership situation disables mirrors.
// --------------------------------------------------------------------------------------
void Tr2InteriorPlaceable::ClearDirty( void )
{
	m_isDirty = false;

	if( m_umbraObjects.size() == 1 )
	{
		EnableMirrors( m_umbraObjects[0]->getCell() );
	}
	else
	{
		DisableMirrors();
	}
}

// -------------------------------------------------------------
// Description:
//   Creates or updates an Umbra object and adds it to specified
//   cell. Used for doors by physical portals.
// Arguments:
//   cell - The cell to add object to
//   object - Umbra object that supposed to represent this dynamic
// -------------------------------------------------------------
void Tr2InteriorPlaceable::UpdateUmbraObject( Umbra::Cell* cell, Umbra::Object*& object ) const
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
	D3DXMatrixInverse( &transformInv, NULL, &transformInv );

	Matrix m = m_transform * transformInv;

	object->setObjectToCellMatrix( AS_UMBRA_MATRIX( m ) );
}

// --------------------------------------------------------------------------------------
//  Description:
//    Query whether the placeable can be used as a background proxy (e.g. a stand-in
//    for drawing a city-scape outside an interior scene)
//  Return Value:
//    true, if the placeable can be used as a background proxy
//    false, if the placeable cannot be used as a background proxy
// --------------------------------------------------------------------------------------
bool Tr2InteriorPlaceable::IsBackgroundProxy( void ) const
{
	if( m_placeableRes )
	{
		return m_placeableRes->IsBackgroundProxy();
	}
	return false;
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

// --------------------------------------------------------------------------------------
//  Description:
//    Adds the placeable to a cell as a background proxy.  This gets called by the 
//    interior scene during OnCellImmediateReport, which is early enough in the visibility
//    query to add objects to the cell.
//  Arguments:
//    cell - the current cell, which will contain the background proxy
// --------------------------------------------------------------------------------------
void Tr2InteriorPlaceable::AddToCellAsBackgroundProxy( Umbra::Cell* cell )
{
	for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
		 it != m_umbraObjects.end(); ++it )
	{
		( *it )->setCell( cell );
	}
}

// --------------------------------------------------------------------------------------
//  Description:
//    Adds the placeable to the root cell. This happens as a fallback when a placeable
//    happens to be outside of any normal cell.
//  Arguments:
//    cell - the root cell
// --------------------------------------------------------------------------------------
void Tr2InteriorPlaceable::AddToRootCell( Umbra::Cell* cell )
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

void Tr2InteriorPlaceable::SetLOD( const TriFrustum* frustum )
{
	// TODO_delder: implement LOD?
}

void Tr2InteriorPlaceable::SetVisibleLightCount( int visibleLightCount )
{
	m_visibleLightCount = visibleLightCount;
}

void Tr2InteriorPlaceable::SetVisibleLightSet( const Tr2InteriorLightSet& visibleLightSet )
{
	if( m_drawLightSpider )
	{
		visibleLightSet.GetLightPositions( m_visibleLights );
	}
}

void Tr2InteriorPlaceable::RenderDebugInfo( TriLineSetPtr lines ) const
{
	if( m_drawLightSpider )
	{
		Color color;
		if( m_visibleLights.size() > MAX_INTERIOR_LIGHTS_PER_OBJECT )
		{
			color = Color( 1.0f, 0.5f, 0.0f, 1.0f );
		}
		else
		{
			color = Color( 0.2f, 1.0f, 0.2f, 1.0f );
		}
		Vector3 center( m_transform._41, m_transform._42, m_transform._43 );
		for( std::vector<Vector3>::const_iterator it = m_visibleLights.begin(); it != m_visibleLights.end(); ++it )
		{
			lines->Add( center, color, *it, color );
		}
	}
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

		UpdateUmbraTransforms();

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

		UpdateUmbraTransforms();

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

		UpdateUmbraTransforms();

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

void Tr2InteriorPlaceable::ClearUmbra( void )
{
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

void Tr2InteriorPlaceable::ClearMirrors( void )
{
	for( std::vector<Tr2InteriorMirror*>::iterator it = m_mirrors.begin(); it != m_mirrors.end(); ++it )
	{
		Tr2InteriorMirror* mirror = *it;
		CCP_DELETE( mirror );
	}

	m_mirrors.clear();
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

	for( ITr2RenderableVector::iterator it = m_attachedObjects.begin(); it != m_attachedObjects.end(); ++it )
	{
		ITr2InteriorAttachedObject *attachedObject = dynamic_cast<ITr2InteriorAttachedObject*>( *it );
		if( attachedObject )
		{
			if( attachedObject->GetBoundingBox( min, max ) )
			{
				BoundingBoxUpdate( minBounds, maxBounds, min, max );
				attachedObject->ClearDirtyFlag();
			}
		}
	}
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
	
	if( m_umbraModel )
	{
		m_umbraModel->release();
		m_umbraModel = NULL;
	}

	// everything is there, so use bounding box
	m_umbraModel = ( Umbra::Model* )Umbra::OBBModel::create( AS_UMBRA_VECTOR3( minBounds ), AS_UMBRA_VECTOR3( maxBounds ) );
	for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
		 it != m_umbraObjects.end(); ++it )
	{
		( *it )->setTestModel( m_umbraModel );
		( *it )->setUserPointer( CONVERT_TO_UMBRA_USER_DATA( this ) );
	}
}

void Tr2InteriorPlaceable::UpdateUmbraTransforms( void )
{
	for( std::vector<Umbra::Object*>::iterator it = m_umbraObjects.begin();
		it != m_umbraObjects.end(); ++it )
	{
		Matrix m = m_transform;
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

void Tr2InteriorPlaceable::EnableMirrors( Umbra::Cell* cell )
{
	ClearMirrors();

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

			// Set Umbra cell
			mirror->SetCell( cell );
			
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

			// Finally, build the mirror
			mirror->BuildUmbraMirror();
		}
	}
}

void Tr2InteriorPlaceable::DisableMirrors( void )
{
	ClearMirrors();
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
// Description
//   Enable or disable Umbra portals for all mirrors on this placeable.
// Arguments:
//   enable - If true - enable portals
//			  If false - disable portals
// --------------------------------------------------------------------------------------
void Tr2InteriorPlaceable::EnableMirrorPortals( bool enable )
{
	for( std::vector<Tr2InteriorMirror*>::iterator it = m_mirrors.begin(); it != m_mirrors.end(); ++it )
	{
		( *it )->EnablePortals( enable );
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
	const Matrix& objectToWorldMatrix,
	const Matrix& mirrorToWorldMatrix )
{
	return GetPerObjectDataWithLightSet( accumulator, 
										lightSet,
										objectToWorldMatrix, 
										mirrorToWorldMatrix );
}

// ---------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the placeable using a reduced per-object data optimized for
//    pre-pass.
//  See Also:
//    GetPerObjectData, GetPerObjectDataWithPerInstanceLighting
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//    objectToWorldMatrix - The transformation matrix used to position this object 
//                          in world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// ---------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorPlaceable::GetPerObjectDataForPrePass( 
	ITriRenderBatchAccumulator* accumulator,
	const Matrix& objectToWorldMatrix )
{
	Tr2PerObjectDataPrePass* data = accumulator->Allocate<Tr2PerObjectDataPrePass>();

	if( !data )
	{
		return NULL;
	}

	// Standard vertex shader data
	Tr2PerObjectVSData perObjectVSBuffer;

	// 0
	memset( &perObjectVSBuffer, 0, sizeof( perObjectVSBuffer ) );

	// column_major for shaders
	D3DXMatrixTranspose( &perObjectVSBuffer.WorldMat, &objectToWorldMatrix );

	// Do the copy
	data->CopyToVSFloatBuffer( perObjectVSBuffer );

	return data;
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

// -------------------------------------------------------------
// Description:
//   Implements ITr2Renderable method. Returns a vector of
//   renderable objects attached to this object.
// Return value:
//   Vector of attached renderable objects
// -------------------------------------------------------------
const ITr2RenderableVector* Tr2InteriorPlaceable::GetAttachedRenderables()
{
	return &m_attachedObjects;
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

	BlueQIPtrAssign( ( IRoot** )&m_placeableRes, p, BlueInterfaceIID<WodPlaceableRes>() );
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

#endif
