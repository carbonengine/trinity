#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorFlare.h"
#include "Tr2ShaderMaterial.h"
#include "Tr2InteriorCell.h"
#include "ITr2UmbraUserData.h"
#include "Tr2InteriorFlareData.h"
#include "TriViewport.h"
#include "TriRenderBatch.h"

const unsigned int Tr2InteriorFlare::MAX_FLARE_IMAGES = 16;

using namespace Tr2RenderContextEnum;
	
// --------------------------------------------------------------------------------------
// Description:
//   Flare/occluder render batch. Calls Tr2InteriorLightSource::RenderFlare on
//   SubmitGeometry.
// --------------------------------------------------------------------------------------
class Tr2InteriorFlare::FlareBatch : public TriRenderBatch
{
public:
	// --------------------------------------------------------------------------------------
	// Description:
	//   Sets light source pointer and flare rendering type.
	// Arguments:
	//   light - Owner light source pointer
	//   type - Flare rendering type (passed to light->RenderFlare)
	// --------------------------------------------------------------------------------------
	void SetLightSource( Tr2InteriorFlare* flare, FlareType type )
	{
		m_flare = flare;
		m_type = type;
	}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Implements TriRenderBatch. Calls Tr2InteriorLightSource::RenderFlare.
	// --------------------------------------------------------------------------------------
	void SubmitGeometry( Tr2RenderContext& renderContext )
	{
		if( m_flare )
		{
			m_flare->RenderFlare( m_type, renderContext );
		}
	}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Gets the batch type name for PIX debugging.
	// Return Value:
	//   Name of this class
	// --------------------------------------------------------------------------------------
	virtual const std::string& GetBatchTypeName( void ) const
	{ 
		static const std::string name = "Tr2InteriorFlare::FlareBatch";
		return name; 
	}
private:
	// Owner light source
	Tr2InteriorFlare* m_flare;
	// Flare rendering type
	FlareType m_type;
};

// --------------------------------------------------------------------------------------
// Description:
//   Flare/occluder per-object vertex shader data.
// --------------------------------------------------------------------------------------
struct Tr2InteriorFlare::FlarePerObjectVSData
{
	// Light source position
	Vector3 position;
	// Flare intensity
	float intensity;
	// Occluder size
	Vector2 size;
	// RT aspect ratio
	Vector2 aspectRatio;
	// Light source color
	Vector4 color;
	// Flare transform for fixed and direction-dependant flares
	Matrix transform;
	// Viewport information
	Vector4 viewport;
	
	// Per-flare image position/size data (see Tr2InteriorFlareData)
	Vector4 flarePositions[MAX_FLARE_IMAGES];
	// Per-flare image texture data (see Tr2InteriorFlareData)
	Vector4 flareTexcoords[MAX_FLARE_IMAGES];
	// Per-flare misc flare data (see Tr2InteriorFlareData)
	Vector4 flareMiscData[MAX_FLARE_IMAGES];
	// Per-flare misc flare data (see Tr2InteriorFlareData)
	Vector4 flareMiscData2[MAX_FLARE_IMAGES];
};

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorFlare default constructor
// --------------------------------------------------------------------------------------
Tr2InteriorFlare::Tr2InteriorFlare( IRoot* lockobj )
:	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_isVisible( true ),
	m_umbraObject( NULL ),
	m_umbraModel( NULL ),
	m_isDirty( true ),
	m_flareIntensity( 0.f ),
	m_occluderSize( 0.f ),
	m_isTotalQueryIssued( false ),
	m_isActualQueryIssued( false ),
	m_actualNumOfPixels( 0 ),
	m_totalNumOfPixels( 1 ),
	m_vertexDecl( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_shSampleIndex( -1 ),
	m_updateVisibility( true ),
	m_debugIsHidden( false ),
	PARENTLOCK( m_transparentFlareData ),
	PARENTLOCK( m_flareData ),
	PARENTLOCK( m_curveSets )
{
	D3DXMatrixIdentity( &m_transform );

	m_occluderMaterial.CreateInstance();
	m_occluderMaterial->SetHighLevelShaderName( "FlareOccluder" );

	Tr2ShaderSituation situation;
	m_occluderMaterial->BindLowLevelShader( situation );

	PrepareResources();
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorFlare destructor.
// --------------------------------------------------------------------------------------
Tr2InteriorFlare::~Tr2InteriorFlare()
{
	ClearUmbra();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface.  Builds the Umbra culling volume after object 
//   initialization.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InteriorFlare::Initialize()
{
	PrepareResources();
	RebuildVolume();
	if( m_transparentFlareMaterial )
	{
		std::vector<unsigned int> situation;
		m_transparentFlareMaterial->BindLowLevelShaderMaterialOnly( situation );
	}
	if( m_flareMaterial )
	{
		std::vector<unsigned int> situation;
		m_flareMaterial->BindLowLevelShaderMaterialOnly( situation );
	}

    return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements INotify interface.  Allows the flare object to respond to parameter 
//   changes generated in Python. 
// Arguments:
//   value - The Blue-exposed parameter that changed
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InteriorFlare::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_transform ) )
	{
		m_isDirty = true;
		UpdateUmbraObjectTransform();
	}
	else if( IsMatch( value, m_occluderSize ) )
	{
		m_isDirty = true;
		UpdateUmbraObjectTransform();
		if( m_occluderSize > 0.f )
		{
			CreateOcclusionObjects();
		}
		else
		{
			m_totalQuery.Destroy();
			m_actualQuery.Destroy();
			m_billboardVB.Destroy();
		}
	}
	else if( IsMatch( value, m_updateVisibility ) )
	{
		if( m_umbraObject )
		{
			m_umbraObject->set( Umbra::Object::UNBOUNDED, !m_updateVisibility );
		}
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IBluePlacementObserver interface. Updates flare position whenever 
//   "position" CEF component is changed.
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	D3DXVec3Normalize( &m_transform.GetY(), &top );
	D3DXVec3Normalize( &m_transform.GetX(), D3DXVec3Cross( &m_transform.GetX(), &front, &top ) );
	D3DXVec3Cross( &m_transform.GetZ(), &m_transform.GetX(), &m_transform.GetY() );
	m_transform.GetTranslation() = pos;

	m_isDirty = true;
	UpdateUmbraObjectTransform();
}

// --------------------------------------------------------------------------------------
// Description:
//   Get position of the flare.
// Return value:
//   Flare position
// --------------------------------------------------------------------------------------
Vector3 Tr2InteriorFlare::GetPosition() const
{
	return m_transform.GetTranslation();
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns new position to the flare.
// Arguments:
//   position - Flare new position
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::SetPosition( const Vector3& position )
{
	m_transform.GetTranslation() = position;

	m_isDirty = true;
	UpdateUmbraObjectTransform();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriDeviceResource interface. Invalidates flare's geometry vertex 
//   declaration, occlusion queries and billboard vertex buffer.
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::ReleaseResources( TriStorage s )
{
	m_vertexDecl = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_billboardVB.Destroy();

	m_totalQuery.Destroy();
	m_actualQuery.Destroy();

	m_occlusionVSConst.Destroy();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriDeviceResource interface. Re-creates flares's D3D objects.
// --------------------------------------------------------------------------------------
bool Tr2InteriorFlare::OnPrepareResources()
{
	Tr2VertexDefinition vd;
	vd.Add( vd.FLOAT32_4, vd.POSITION );

	m_vertexDecl = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );

	CreateOcclusionObjects();

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets visibility flag for the object.
// Arguments:
//   bVisible - Visibility flag: true if the object should render, false if the object
//              should be hidden
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::SetVisibility( bool bVisible )
{
	m_isVisible = bVisible;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns visibility flag.
// Return Value:
//   true If the object should render
//   false if the object should be hidden
// --------------------------------------------------------------------------------------
bool Tr2InteriorFlare::IsVisible( void ) const
{
	return m_isVisible;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the object using a per-instance light-set override and 
//    an arbitrary object-to-world matrix. Flares don't use lighting and don't render
//    in mirrors, so this method is equivalent to ITr2Renderable::GetPerObjectData.
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//    lightSet -            The set of lights illuminating this object
//    objectToWorldMatrix - The transformation matrix used to position this object 
//                          in world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorFlare::GetPerObjectDataWithPerInstanceLighting( 
											ITriRenderBatchAccumulator* accumulator,
											Tr2InteriorLightSet* lightSet, 
											const Matrix& objectToWorldMatrix, 
											const Matrix& mirrorToWorldMatrix )
{
	return GetPerObjectData( accumulator );
}

// --------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the object using a per-instance light-set override and 
//    an arbitrary object-to-world matrix.  Flares don't render during prepass phase, 
//    so this method is empty.
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//    lightSet -            The set of lights illuminating this object
//    objectToWorldMatrix - The transformation matrix used to position this object 
//                          in world coordinates
//  Return Value:
//    NULL always.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorFlare::GetPerObjectDataForPrePass(
											ITriRenderBatchAccumulator* accumulator,
											const Matrix& objectToWorldMatrix
)
{
	return NULL;
}

// -------------------------------------------------------------
// Description:
//   Returns bounding sphere for this object.
// Arguments:
//   sphere (out) - Bounding sphere (center.xyz, radius)
// Return value:
//   true Always
// -------------------------------------------------------------
bool Tr2InteriorFlare::GetBoundingSphere( Vector4& sphere ) const
{
	sphere.x = m_transform._41;
	sphere.y = m_transform._42;
	sphere.z = m_transform._43;
	sphere.w = sqrt( 2.f ) * m_occluderSize;
	return true;
}

// -------------------------------------------------------------
// Description:
//   Returns bounding box in local space for this object.
// Arguments:
//   minBounds (out) - Bounding box min bounds
//   maxBounds (out) - Bounding box max bounds
// Return value:
//   true Always
// -------------------------------------------------------------
bool Tr2InteriorFlare::GetLocalBoundingBox( Vector3& min, Vector3& max ) const
{
	min = Vector3( -m_occluderSize, -m_occluderSize, -m_occluderSize );
	max = Vector3( m_occluderSize, m_occluderSize, m_occluderSize );
	return true;
}

// -------------------------------------------------------------
// Description:
//   Returns bounding box in world space for this object.
// Arguments:
//   minBounds (out) - Bounding box min bounds
//   maxBounds (out) - Bounding box max bounds
// Return value:
//   true Always
// -------------------------------------------------------------
bool Tr2InteriorFlare::GetWorldBoundingBox( Vector3& min, Vector3& max ) const
{
	min = Vector3( -m_occluderSize, -m_occluderSize, -m_occluderSize );
	max = Vector3( m_occluderSize, m_occluderSize, m_occluderSize );
	BoundingBoxTransform( min, max, m_transform );
	return true;
}

// -------------------------------------------------------------
// Description:
//   Returns if bounding box is ready (always true).
// -------------------------------------------------------------
bool Tr2InteriorFlare::IsBoundingBoxReady( void ) const
{
	return true;
}

// -------------------------------------------------------------
// Description:
//   Returns position of Enlighten SH probe in world space.
// -------------------------------------------------------------
bool Tr2InteriorFlare::GetShProbePosition( Vector3& position ) const
{
	Vector3 min, max;
	GetWorldBoundingBox( min, max );
	position = ( min + max ) * 0.5f;
	return true;
}

// -------------------------------------------------------------
// Description:
//   Updates an object before running physics update. Does nothing.
// Arguments:
//   time - Current system time
// -------------------------------------------------------------
void Tr2InteriorFlare::PrePhysicsUpdate( Be::Time time )
{
}

// -------------------------------------------------------------
// Description:
//   Updates an object after running physics update. Does nothing.
// Arguments:
//   time - Current system time
// -------------------------------------------------------------
void Tr2InteriorFlare::PostPhysicsUpdate( Be::Time time, Tr2ApexScene *apexScene )
{
}

// -------------------------------------------------------------
// Description:
//   Returns a reference to red SH probe matrix. Flares don't
//   use lighting, so this method returns an unused matrix
//   variable.
// Return value:
//   Reference to red SH probe matrix
// -------------------------------------------------------------
Matrix& Tr2InteriorFlare::GetRedLightProbeMatrix( void )
{
	return m_shMatrix;
}

// -------------------------------------------------------------
// Description:
//   Returns a reference to green SH probe matrix. Flares don't
//   use lighting, so this method returns an unused matrix
//   variable.
// Return value:
//   Reference to green SH probe matrix
// -------------------------------------------------------------
Matrix& Tr2InteriorFlare::GetGreenLightProbeMatrix( void )
{
	return m_shMatrix;
}

// -------------------------------------------------------------
// Description:
//   Returns a reference to blue SH probe matrix. Flares don't
//   use lighting, so this method returns an unused matrix
//   variable.
// Return value:
//   Reference to blue SH probe matrix
// -------------------------------------------------------------
Matrix& Tr2InteriorFlare::GetBlueLightProbeMatrix( void )
{
	return m_shMatrix;
}

// -------------------------------------------------------------
// Description:
//   Called when object is added to the scene. Updates Umbra 
//   objects.
// Return value:
//   true If the object has valid bounds and can be added to a cell
//   false Otherwise
// -------------------------------------------------------------
bool Tr2InteriorFlare::AddToScene( Tr2ApexScene *apexScene )
{
	ClearUmbra();
	RebuildVolume();

	return true;
}

// -------------------------------------------------------------
// Description:
//   Called when object is removed from the scene. Clears Umbra 
//   objects.
// -------------------------------------------------------------
void Tr2InteriorFlare::RemoveFromScene( void )
{
	ClearUmbra();
}

// -------------------------------------------------------------
// Description:
//   Query if the dynamic object has valid Umbra objects.
// Return value:
//   true If Umbra objects are ready
//   false Otherwise
// -------------------------------------------------------------
bool Tr2InteriorFlare::IsUmbraReady( void ) const
{
	return m_umbraObject != NULL;
}

// -------------------------------------------------------------
// Description:
//   Checks if the object intersects a cell and if it does adds
//   itself the the cell.
// Arguments:
//   cell - A cell to test intersection with
// Return value:
//   true If the object intersects the cell
//   false Otherwise
// -------------------------------------------------------------
bool Tr2InteriorFlare::TestCellIntersectionAndAdd( Tr2InteriorCell* cell )
{
	// Bail out if the cell is invalid
	if( cell == NULL )
	{
		// No cell, return no intersection
		return false;
	}


	// Get our bounding box
	Vector3 minBounds, maxBounds;
	if( !GetWorldBoundingBox( minBounds, maxBounds ) )
	{
		// Our bounding box is not ready, return false (no intersection)
		return false;
	}

	bool intersects = cell->IntersectsAABB( minBounds, maxBounds );

	// If we got an intersection, add to the cell
	if( intersects )
	{
		if( cell->AddDynamic( this ) )
		{
			// Add to the Umbra cell
			m_umbraObject->setCell( cell->GetUmbraCell() );
		}

		UpdateUmbraObjectTransform();
	}
	else
	{
		cell->RemoveDynamic( this );
	}

	// Return the result of the intersection test
	return intersects;
}

// --------------------------------------------------------------------------------------
// Description:
//   Called when a cell is removed from the scene. If the object belongs to this cell
//   the object clears its Umbra stuff.
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::CellRemoved( Tr2InteriorCell* cell )
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

	if( m_umbraObject && ( m_umbraObject->getCell() == ucell ) )
	{
		m_umbraObject->release();
		m_umbraObject = NULL;
	}
}

// -------------------------------------------------------------
// Description:
//   Query if the object has dirty bounds. 
// Return value:
//   true If object has dirty bounds
//   false Otherwise
// -------------------------------------------------------------
bool Tr2InteriorFlare::IsDirty( void ) const
{
	return m_isDirty;
}

// -------------------------------------------------------------
// Description:
//   Called after a dirty object was re-added to the scene.
// -------------------------------------------------------------
void Tr2InteriorFlare::ClearDirty( void )
{
	m_isDirty = false;
}

// -------------------------------------------------------------
// Description:
//   Directly set the bounds dirty flag. 
// Arguments:
//   isDirty - New diry flag
// -------------------------------------------------------------
void Tr2InteriorFlare::SetDirtyFlag( bool isDirty )
{
	m_isDirty = isDirty;
}

// -------------------------------------------------------------
// Description:
//   Query if the object acts as a background object. Flares
//   don't support that.
// Return value:
//   false Always
// -------------------------------------------------------------
bool Tr2InteriorFlare::IsBackgroundProxy( void ) const
{
	return false;
}

// -------------------------------------------------------------
// Description:
//   Add an object as a background object. Flares
//   don't support that.
// Arguments:
//   cell - Root cell to add an object to
// -------------------------------------------------------------
void Tr2InteriorFlare::AddToCellAsBackgroundProxy( Umbra::Cell* cell )
{
}

// --------------------------------------------------------------------------------------
//  Description:
//    Adds the object to the root cell. This happens as a fallback when an object
//    happens to be outside of any normal cell.
//  Arguments:
//    cell - the root cell
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::AddToRootCell( Umbra::Cell* cell )
{
	m_umbraObject->setCell( cell );
	UpdateUmbraObjectTransform();
}

// -------------------------------------------------------------
// Description:
//   Query if the object casts shadows. Flares don't.
// Return value:
//   false Always
// -------------------------------------------------------------
bool Tr2InteriorFlare::IsShadowCaster( void ) const
{
	return false;
}

// -------------------------------------------------------------
// Description:
//   Creates or updates an Umbra object and adds it to specified
//   cell. Used for doors by physical portals.
// Arguments:
//   cell - The cell to add object to
//   object - Umbra object that supposed to represent this dynamic
// -------------------------------------------------------------
void Tr2InteriorFlare::UpdateUmbraObject( Umbra::Cell* cell, Umbra::Object*& object ) const
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
	Matrix scaling;
	D3DXMatrixScaling( &scaling, m_occluderSize, m_occluderSize, m_occluderSize );

	Matrix m = scaling * m_transform * transformInv;

	object->setObjectToCellMatrix( AS_UMBRA_MATRIX( m ) );
}

// -------------------------------------------------------------
// Description:
//   Assignes LOD based on camera. Not implemented yet.
// Arguments:
//   frustum - Camera frustum
// -------------------------------------------------------------
void Tr2InteriorFlare::SetLOD( const TriFrustum* frustum )
{
	// TODO_delder: implement LOD?
}

// --------------------------------------------------------------------------------------
//  Description:
//    Accumulates batches for rendering.
//  Arguments:
//    batches - The batch accumulator used to allocate memory for per-object data
//    batchType - Type batches to collect
//    perObjectData - Per-object data for this object
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::GetBatches( ITriRenderBatchAccumulator* batches, 
						 TriBatchType batchType, 
						 const Tr2PerObjectData* perObjectData )
{
	if( m_debugIsHidden )
	{
		return;
	}
	if( batchType == TRIBATCHTYPE_TRANSPARENT || batchType == TRIBATCHTYPE_ADDITIVE )
	{
		UpdateFlareVisibility( batches );
		if( m_flareIntensity > 0.f && m_transparentFlareMaterial )
		{
			GetFlareBatches( batches, TRANSPARENT_FLARE );
		}
	}
	else if( batchType == TRIBATCHTYPE_FLARE )
	{
		if( m_flareIntensity > 0.f && m_flareMaterial )
		{
			GetFlareBatches( batches, NON_POSTPROCCESSED_FLARE );
		}
	}
}

// --------------------------------------------------------------------------------------
//  Description:
//    Checks if the mesh has transparent areas.  
//  Return Value:
//    true Always.
// --------------------------------------------------------------------------------------
bool Tr2InteriorFlare::HasTransparentBatches()
{
	return true;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Implements ITr2Renderable interface. Does nothing.  
//  Return Value:
//    0.0f always.
// --------------------------------------------------------------------------------------
float Tr2InteriorFlare::GetSortValue()
{
	return 0;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the object. Returns NULL since flares don't use per-object
//    data in a normal way.
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// --------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorFlare::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return NULL;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Updates Umbra object to cell transform matrix.  
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::UpdateUmbraObjectTransform()
{
	if( m_umbraObject )
	{
		Umbra::Cell* cell = m_umbraObject->getCell();
		if( cell )
		{
			Matrix transformInv;
			cell->getWorldToCellMatrix( AS_UMBRA_MATRIX( transformInv ) );
			Matrix scaling;
			D3DXMatrixScaling( &scaling, m_occluderSize, m_occluderSize, m_occluderSize );

			Matrix m = scaling * m_transform * transformInv;

			m_umbraObject->setObjectToCellMatrix( AS_UMBRA_MATRIX( m ) );
		}
	}
}

// -------------------------------------------------------------
// Description:
//   Destroys Umbra objects.
// -------------------------------------------------------------
void Tr2InteriorFlare::ClearUmbra()
{
	if( m_umbraObject )
	{
		m_umbraObject->setCell( NULL );
		m_umbraObject->release();
		m_umbraObject = NULL;
	}

	if( m_umbraModel )
	{
		m_umbraModel->release();
		m_umbraModel = NULL;
	}
}

// -------------------------------------------------------------
// Description:
//   Re-create Umbra objects.
// -------------------------------------------------------------
void Tr2InteriorFlare::RebuildVolume()
{
	Umbra::Cell *cell = NULL;
	if( m_umbraObject )
	{
		cell = m_umbraObject->getCell();
	}

	ClearUmbra();
	
	// everything is there, so use bounding box
	m_umbraModel = ( Umbra::Model* )Umbra::OBBModel::create( AS_UMBRA_VECTOR3( Vector3( -1.f, -1.f, -1.f ) ), AS_UMBRA_VECTOR3( Vector3( 1.f, 1.f, 1.f ) ) );
	m_umbraObject = Umbra::Object::create( m_umbraModel );
	m_umbraObject->setCell( cell );
	m_umbraObject->set( Umbra::Object::UNBOUNDED, !m_updateVisibility );

	m_umbraObject->setUserPointer( CONVERT_TO_UMBRA_USER_DATA( this ) );

	UpdateUmbraObjectTransform();
}

// --------------------------------------------------------------------------------------
// Description:
//   Creates occlusion queries and billboards vertex buffer for flares if necessary.
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::CreateOcclusionObjects()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( Tr2Renderer::IsResourceCreationAllowed() && m_occluderSize > 0.f )
	{
		if( !m_totalQuery.IsValid() )
		{
			CR_RETURN( m_totalQuery.Create( renderContext ) );
			m_isTotalQueryIssued = false;
		}

		if( !m_actualQuery.IsValid() )
		{
			CR_RETURN( m_actualQuery.Create( renderContext ) );
			m_isActualQueryIssued = false;
		}

		if( !m_billboardVB.IsValid() )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();

			Vector4 vertexes[6 * MAX_FLARE_IMAGES]; 
			for( unsigned int i = 0; i < MAX_FLARE_IMAGES; ++i )
			{
				vertexes[6 * i + 0] = Vector4( -1.f, 1.f, 0.f, float( i ) );
				vertexes[6 * i + 1] = Vector4( -1.f, -1.f, 0.f, float( i ) );
				vertexes[6 * i + 2] = Vector4( 1.f, 1.f, 0.f, float( i ) );
				vertexes[6 * i + 3] = Vector4( 1.f, 1.f, 0.f, float( i ) );
				vertexes[6 * i + 4] = Vector4( -1.f, -1.f, 0.f, float( i ) );
				vertexes[6 * i + 5] = Vector4( 1.f, -1.f, 0.f, float( i ) );
			}
			CR_RETURN( m_billboardVB.Create(	6 * sizeof( Vector4 ) * MAX_FLARE_IMAGES, 
												USAGE_IMMUTABLE, 
												vertexes, 
												renderContext ) );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Overrides viewport. Usefull for large screenshots.
// Arguments:
//   viewport - Overriden viewport or NULL to turn override off.
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::OverrideViewport( TriViewport* viewport )
{
	m_overridenViewport = viewport;
}

// --------------------------------------------------------------------------------------
// Description:
//   Renders flare/occluder. Is called from Tr2InteriorLightSource::FlareBatch::SubmitGeometry.
// Arguments:
//   type - Flare rendering type
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::RenderFlare( FlareType type, Tr2RenderContext& renderContext )
{
	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDecl );
	renderContext.m_esm.ApplyStreamSource( 0, m_billboardVB, 0, sizeof( Vector4 ) );

	// We upload per-object VS data here because the data depends on batch type
	// and because it is possible to have more per-object data than allowed
	// for per-object data chunk.
	FlarePerObjectVSData vsData;
	vsData.position = m_transform.GetTranslation();
	D3DXMatrixTranspose( &vsData.transform, &m_transform );
	vsData.intensity = m_flareIntensity;
	vsData.size = Vector2( m_occluderSize, m_occluderSize );
	vsData.aspectRatio = Vector2( float( Tr2Renderer::GetRenderTargetHeight() ) / 
		float( Tr2Renderer::GetRenderTargetWidth() ), 1.f );
	vsData.color = TriGammaToLinear( Vector4( m_color.r, m_color.g, m_color.b, 1.f ) );
	if( m_overridenViewport )
	{
		vsData.viewport.x = float( Tr2Renderer::GetRenderTargetWidth() ) / m_overridenViewport->width;
		vsData.viewport.y = float( Tr2Renderer::GetRenderTargetHeight() ) / m_overridenViewport->height;
		vsData.viewport.z = vsData.viewport.x + float( m_overridenViewport->x ) / m_overridenViewport->width * 2.f - 1.f; 
		vsData.viewport.w = ( vsData.viewport.y + float( m_overridenViewport->y ) / m_overridenViewport->height * 2.f - 1.f ) * -1.f;
	}
	else
	{
		vsData.viewport = Vector4( 1.0f, 1.0f, 0.0f, 0.0f );
	}

	switch( type )
	{
	case OCCLUSION_FLARE:
		{
			FillAndSetConstants( m_occlusionVSConst, vsData, VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
			
			uint32_t oldColorWriteState;
			uint32_t oldZEnableState;
			renderContext.GetRenderState( RS_COLORWRITEENABLE, &oldColorWriteState);
			renderContext.GetRenderState( RS_ZENABLE, &oldZEnableState);

			ON_BLOCK_EXIT( [&]{ renderContext.SetRenderState( RS_COLORWRITEENABLE, oldColorWriteState ); } );
			ON_BLOCK_EXIT( [&]{ renderContext.SetRenderState( RS_ZENABLE, oldZEnableState ); } );

			renderContext.SetRenderState( RS_COLORWRITEENABLE, 0 );

			// Render un-occluded version (without Z-test)
			if( SUCCEEDED( m_totalQuery.Begin( renderContext ) ) )
			{
				renderContext.SetRenderState( RS_ZENABLE, FALSE );
				renderContext.SetTopology( TOP_TRIANGLES );
				renderContext.DrawPrimitive( 0, 2 );
				m_totalQuery.End( renderContext );
				m_isTotalQueryIssued = true;
			}

			// Render occluded version (with Z-test)
			if( SUCCEEDED( m_actualQuery.Begin( renderContext ) ) )
			{
				renderContext.SetRenderState( RS_ZENABLE, TRUE );
				renderContext.SetTopology( TOP_TRIANGLES );
				renderContext.DrawPrimitive( 0, 2 );
				m_actualQuery.End( renderContext );
				m_isActualQueryIssued = true;
			}
		}
		break;
	case TRANSPARENT_FLARE:
		{
			renderContext.SetTopology( TOP_TRIANGLES );

			unsigned int index = 0;
			for( PTr2InteriorFlareDataVector::const_iterator it = m_transparentFlareData.begin();
				it != m_transparentFlareData.end();
				++it )
			{
				vsData.flarePositions[index] = Vector4( (*it)->m_positionWeight.x, 
														(*it)->m_positionWeight.y, 
														(*it)->m_size.x, 
														(*it)->m_size.y );
				vsData.flareTexcoords[index] = Vector4( (*it)->m_textureOffset.x, 
														(*it)->m_textureOffset.y,
														(*it)->m_textureSize.x, 
														(*it)->m_textureSize.y );
				vsData.flareMiscData[index] = Vector4( (*it)->m_rotation ? 1.0f : 0.0f, 
														(*it)->m_edgeFadeDistance,
														(*it)->m_centerFadeMinRadius, 
														(*it)->m_centerFadeMaxRadius );
				vsData.flareMiscData2[index] = Vector4( (*it)->m_directionalStretch.x, 
														(*it)->m_directionalStretch.y,
														0.0f, 
														0.0f );
				if( ++index >= MAX_FLARE_IMAGES )
				{
					FillAndSetConstants( m_occlusionVSConst, vsData, VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
					
					renderContext.DrawPrimitive( 0, 2 * MAX_FLARE_IMAGES );
					index = 0;
					break;
				}
			}

			FillAndSetConstants( m_occlusionVSConst, vsData, VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );

			renderContext.DrawPrimitive( 0, 2 * index );
		}
		break;
	case NON_POSTPROCCESSED_FLARE:
		{

			unsigned int index = 0;

			renderContext.SetTopology( TOP_TRIANGLES );

			for( PTr2InteriorFlareDataVector::const_iterator it = m_flareData.begin();
				it != m_flareData.end();
				++it )
			{
				vsData.flarePositions[index] = Vector4( (*it)->m_positionWeight.x, 
														(*it)->m_positionWeight.y, 
														(*it)->m_size.x, 
														(*it)->m_size.y );
				vsData.flareTexcoords[index] = Vector4( (*it)->m_textureOffset.x, 
														(*it)->m_textureOffset.y,
														(*it)->m_textureSize.x, 
														(*it)->m_textureSize.y );
				vsData.flareMiscData[index] = Vector4( (*it)->m_rotation ? 1.0f : 0.0f, 
														(*it)->m_edgeFadeDistance,
														(*it)->m_centerFadeMinRadius, 
														(*it)->m_centerFadeMaxRadius );
				vsData.flareMiscData2[index] = Vector4( (*it)->m_directionalStretch.x, 
														(*it)->m_directionalStretch.y,
														0.0f, 
														0.0f );
				if( ++index >= MAX_FLARE_IMAGES )
				{
					FillAndSetConstants( m_occlusionVSConst, vsData, VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );

					renderContext.DrawPrimitive( 0, 2 * MAX_FLARE_IMAGES );
					index = 0;
					break;
				}
			}

			FillAndSetConstants( m_occlusionVSConst, vsData, VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
			
			renderContext.DrawPrimitive( 0, 2 * index );
		}
		break;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds render batches for flares/occluder to batch accumulator.
// Arguments:
//   batches - Render batch accumulator
//   type - Flare rendering type
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::GetFlareBatches( ITriRenderBatchAccumulator* batches, FlareType type )
{
	FlareBatch* batch = batches->Allocate<FlareBatch>();
	if( batch )
	{
		batches->SetRenderingMode( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
		switch( type )
		{
		case OCCLUSION_FLARE:
			batch->SetShaderMaterial( m_occluderMaterial );
			break;
		case TRANSPARENT_FLARE:
			batch->SetShaderMaterial( m_transparentFlareMaterial );
			break;
		case NON_POSTPROCCESSED_FLARE:
			batch->SetShaderMaterial( m_flareMaterial );
			break;
		}
		batch->SetPerObjectData( NULL );
		batch->SetDepth( 0xffffffff );
		batch->SetLightSource( this, type );
		batches->Commit( batch );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Fetches data from occlusion queries for flares and creates new render batches for 
//   occluder rendering.
// Arguments:
//   batches - Render batch accumulator
// --------------------------------------------------------------------------------------
void Tr2InteriorFlare::UpdateFlareVisibility( ITriRenderBatchAccumulator* batches )
{
	if( !m_updateVisibility || !m_totalQuery.IsValid() || !m_actualQuery.IsValid() )
	{
		return;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	// fetch query results
	unsigned queryValue;
	if( m_isTotalQueryIssued )
	{
		HRESULT hr = m_totalQuery.GetPixelCount( renderContext, queryValue );
		if( hr == S_OK )
		{
			m_isTotalQueryIssued = false;
			m_totalNumOfPixels = queryValue;
		}
	}
	if( m_isActualQueryIssued )
	{
		HRESULT hr = m_actualQuery.GetPixelCount( renderContext, queryValue );
		if( hr == S_OK )
		{
			m_isActualQueryIssued = false;
			m_actualNumOfPixels = queryValue;
		}
	}

	// only update mValue when both queries have succesfully finished
	if( !m_isTotalQueryIssued && !m_isActualQueryIssued )
	{
		if( m_totalNumOfPixels > 0 && m_totalNumOfPixels >= m_actualNumOfPixels )
		{
			m_flareIntensity = (float)m_actualNumOfPixels / (float)m_totalNumOfPixels;
		}
	}

	bool issueQueries = !m_isTotalQueryIssued && !m_isActualQueryIssued;
	if( issueQueries )
	{
		GetFlareBatches( batches, OCCLUSION_FLARE );
	}
}

// -------------------------------------------------------------
// Description:
//   Blue-exposed function that returns AABB for the flare in its
//   local coordinate space.
// Return value:
//   AABB for the flare in its local coordinate space
// -------------------------------------------------------------
AxisAlignedBoundingBox Tr2InteriorFlare::GetBoundingBoxInLocalSpace() const
{
	AxisAlignedBoundingBox result( Vector3( 0.f, 0.f, 0.f ), Vector3( 0.f, 0.f, 0.f ) );
	GetLocalBoundingBox( result.m_min, result.m_max );
	return result;
}

// -------------------------------------------------------------
// Description:
//   Blue-exposed function that returns AABB for the flare in the
//   world coordinate space.
// Return value:
//   AABB for the flare in the world coordinate space
// -------------------------------------------------------------
AxisAlignedBoundingBox Tr2InteriorFlare::GetBoundingBoxInWorldSpace() const
{
	AxisAlignedBoundingBox result( Vector3( 0.f, 0.f, 0.f ), Vector3( 0.f, 0.f, 0.f ) );
	GetWorldBoundingBox( result.m_min, result.m_max );
	return result;
}

#endif
