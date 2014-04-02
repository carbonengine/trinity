////////////////////////////////////////////////////////////
//
//    Created:   October 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorCylinderLight.h"

#include "umbraTypes.h"

#include "Tr2InteriorLightGeometryRenderBatch.h"
#include "Tr2InteriorCell.h"
#include "Tr2KelvinColor.h"
#include "ITr2UmbraUserData.h"
#include "Tr2InteriorOrientedBoundingBox.h"
#include "Tr2InteriorLightSource.h"
#include "Tr2ShaderMaterial.h"
#include "Tr2ConstGeometry.h"
#include "Curves/TriCurveSet.h"
#include "Tr2InteriorConstantBufferFormats.h"

CCP_STATS_DECLARED_ELSEWHERE( wodIntLightsAlive );

using namespace Tr2RenderContextEnum;
	
namespace {
    unsigned int const unitCylinderIndices[] =
    {
		0, 6, 1,
		6, 7, 1,
			
		1, 7, 2,
		7, 8, 2,
			
		2, 8, 3,
		8, 9, 3,
			
		3, 9, 4,
		9, 10, 4,
			
		4, 10, 5,
		10, 11, 5,

		5, 11, 0,
		11, 6, 0,

		0,	1,	2,
		0,	2,	3,
		0,	3,	4,
		0,	4,	5,

		6,	8,	7,
		6,	9,	8,
		6,	10,	9,
		6,	11,	10,
    };
    unsigned int const unitCylinderWireIndices[] =
    {
		0, 1,
		1, 2,
		2, 3, 
		3, 4, 
		4, 5, 
		5, 0, 

		6, 7,
		7, 8, 
		8, 9, 
		9, 10, 
		10, 11, 
		11, 6,

		0, 6, 
		1, 7, 
		2, 8, 
		3, 9, 
		4, 10, 
		5, 11,
    };
    Vector4 const unitCylinderVertices[] =
    {
		Vector4( 0.0f,	 0.5773503f, -0.5f, 1.0f ),
		Vector4( 0.5f,	 0.28867515f, -0.5f, 1.0f ),
		Vector4( 0.5f,	-0.28867515f, -0.5f, 1.0f ),
		Vector4( 0.0f,	-0.5773503f, -0.5f, 1.0f ),
		Vector4( -0.5f,	-0.28867515f, -0.5f, 1.0f ),
		Vector4( -0.5f,	 0.28867515f, -0.5f, 1.0f ),

		Vector4( 0.0f,	 0.5773503f, 0.5f, 1.0f ),
		Vector4( 0.5f,	 0.28867515f, 0.5f, 1.0f ),
		Vector4( 0.5f,	-0.28867515f, 0.5f, 1.0f ),
		Vector4( 0.0f,	-0.5773503f, 0.5f, 1.0f ),
		Vector4( -0.5f,	-0.28867515f, 0.5f, 1.0f ),
		Vector4( -0.5f,	 0.28867515f, 0.5f, 1.0f ),
    };

	static const unsigned int unitCylinderVertexCount = sizeof( unitCylinderVertices ) / sizeof( Vector4 );
	static const unsigned int unitCylinderTriangleCount = sizeof( unitCylinderIndices ) / sizeof( unsigned int ) / 3;
	static const unsigned int unitCylinderWireLineCount = sizeof( unitCylinderWireIndices ) / sizeof( unsigned int ) / 2;
} // anonymous

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorCylinderLight default constructor
// --------------------------------------------------------------------------------------
Tr2InteriorCylinderLight::Tr2InteriorCylinderLight( IRoot* lockobj ) :
	m_name(),
	m_radius( 1.f ),
	m_length( 1.f ),
	m_position( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_falloff( 1.f ),
	m_specularIntensity( 1.f ),
	m_sectorAngleOuter( 180.f ),
	m_sectorAngleInner( 180.f ),
	m_primaryLighting( true ),
	m_secondaryLighting( true ),
	m_secondaryLightingMultiplier( 1.0f ),
	m_affectTransparentObjects( true ),
	m_enableROIs( true ),
	m_isDirty( true ),
	m_isStatic( true ),
	m_staticFlagChanged( true ),
	m_umbraModel( NULL ),
	m_umbraRoiList(),
	m_lightVertexDecl( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_renderDebugInfo( false ),
	m_renderDebugType( DI_LIGHT_COLOR ),
	PARENTLOCK( m_curveSets ),
	m_vertexes( NULL ),
	m_vertexCount( 0 )
{
	CCP_STATS_INC( wodIntLightsAlive );

	Vector3 scale( m_radius * 2.f, m_radius * 2.f, m_length + 2 * m_radius );		
	D3DXMatrixTransformation( &m_transform, NULL, NULL, &scale, NULL, &m_rotation, &m_position );

	m_material.CreateInstance();	
	m_material->SetHighLevelShaderName( "CylinderLight" );

	m_shMaterial.CreateInstance();	
	m_shMaterial->SetHighLevelShaderName( "CylinderLight" );

	m_lightmapMaterial.CreateInstance();	
	m_lightmapMaterial->SetHighLevelShaderName( "CylinderLight" );

	m_kelvinColor.CreateInstance();
	m_useKelvinColor = false;

	PrepareResources();

	RebuildVolume();

	ChooseLightEffect();

	m_sharedVB = Tr2ConstGeometry::GetVB( unitCylinderVertices, sizeof( unitCylinderVertices ) );
	m_sharedIB = Tr2ConstGeometry::GetIB( IB_32BIT, unitCylinderIndices, sizeof( unitCylinderIndices ) / sizeof( unitCylinderIndices[0] ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorCylinderLight destructor.
// --------------------------------------------------------------------------------------
Tr2InteriorCylinderLight::~Tr2InteriorCylinderLight()
{
	if( m_projectedTextureRes )
	{
		m_projectedTextureRes->RemoveNotifyTarget( this );
		m_projectedTextureRes.Unlock();
	}

	// Clear Umbra data
	ClearUmbra();

	CCP_STATS_DEC( wodIntLightsAlive );

	CCP_DELETE []m_vertexes;
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from IInitialize interface.  Builds the Umbra culling volume after object 
//   initialization.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InteriorCylinderLight::Initialize()
{
	TransformModified();

	RebuildVolume();

	ChooseLightEffect();

	if( m_projectedTextureRes )
	{
		m_projectedTextureRes->RemoveNotifyTarget( this );
		m_projectedTextureRes.Unlock();
	}
	if( !m_projectedTexturePath.empty() )
	{
		BeResMan->GetResource( m_projectedTexturePath.c_str(), "", m_projectedTextureRes );
	}
	if( m_projectedTextureRes )
	{
		m_projectedTextureRes->AddNotifyTarget( this );
	}

	RebuildGeometry();

    return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from INotify interface.  Allows the light source to respond to parameter 
//   changes generated in Python.  If the light transform changes, the regions of influence 
//   are updated with the new transform matrix & the light is flagged as 'dirty', forcing 
//   a new round of light-cell intersection tests on the next scene Update.  
// Arguments:
//   value - The Blue-exposed parameter that changed
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InteriorCylinderLight::OnModified( Be::Var* value )
{
	// Update the regions of influence if the position changes
	if( IsMatch( value, m_length )		|| 
		IsMatch( value, m_radius )		|| 
		IsMatch( value, m_position )	|| 
		IsMatch( value, m_rotation ) )
	{
		Vector3 scale( m_radius * 2.f, m_radius * 2.f, m_length + 2 * m_radius );		
		D3DXMatrixTransformation( &m_transform, NULL, NULL, &scale, NULL, &m_rotation, &m_position );
		
		TransformModified();
		RebuildGeometry();
	}
	else if( IsMatch( value, m_projectedTexturePath ) )
	{
		if( m_projectedTextureRes )
		{
			m_projectedTextureRes->RemoveNotifyTarget( this );
			m_projectedTextureRes.Unlock();
		}
		if( !m_projectedTexturePath.empty() )
		{
			BeResMan->GetResource( m_projectedTexturePath.c_str(), "", m_projectedTextureRes );
		}
		if( m_projectedTextureRes )
		{
			m_projectedTextureRes->AddNotifyTarget( this );
		}
	}
	else if( IsMatch( value, m_affectTransparentObjects ) )
	{
		for( std::vector<Umbra::RegionOfInfluence*>::iterator it = m_umbraRoiList.begin(); it != m_umbraRoiList.end(); ++it )
		{
			( *it )->set( Umbra::Object::ENABLED, m_affectTransparentObjects && m_enableROIs );
		}
	}
	else if( IsMatch( value, m_sectorAngleOuter ) || IsMatch( value, m_sectorAngleInner ) )
	{
		ChooseLightEffect();
	}
	else if( IsMatch( value, m_boundingBox ) )
	{
		if( m_boundingBox )
		{
			m_boundingBox->AddNotifyTarget( this );
		}
		RebuildGeometry();
		ChooseLightEffect();
	}
	else if( m_boundingBox != NULL && IsMatch( value, m_boundingBox->GetTransform() ) )
	{
		RebuildGeometry();
	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the projected texture resource.
// Arguments:
//   texture - New texture resource
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::SetProjectedTexture( TriTextureRes* texture )
{
	if( m_projectedTextureRes == texture )
	{
		return;
	}

	if( m_projectedTextureRes )
	{
		m_projectedTextureRes->RemoveNotifyTarget( this );
		m_projectedTextureRes.Unlock();
	}
	m_projectedTextureRes = texture;
	if( m_projectedTextureRes )
	{
		m_projectedTextureRes->AddNotifyTarget( this );
	}
	else
	{
		ChooseLightEffect();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Called whenever light's transform changes. Updates regions of influence 
//   with the new transform matrix & flags the light as 'dirty', forcing 
//   a new round of light-cell intersection tests on the next scene Update.  
//   Recalculates cached values for collision functions.
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::TransformModified()
{
	// Update the matrices on the regions of influence
	for( std::vector<Umbra::RegionOfInfluence*>::iterator it = m_umbraRoiList.begin(); 
		 it != m_umbraRoiList.end(); ++it )
	{
		if( ( *it )->getCell() != NULL )
		{
			Matrix cellTransform;
			( *it )->getCell()->getCellToWorldMatrix( AS_UMBRA_MATRIX( cellTransform ) );
			D3DXMatrixInverse( &cellTransform, NULL, &cellTransform );
			cellTransform = m_transform * cellTransform;
			( *it )->setObjectToCellMatrix( AS_UMBRA_MATRIX( cellTransform ) );
		}
		else
		{
			( *it )->setObjectToCellMatrix( AS_UMBRA_MATRIX( m_transform ) );
		}
	}

	// Mark the dirty flag
	m_isDirty = true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Invalidates light's geometry vertex 
//   declaration and shadow maps.
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::ReleaseResources( TriStorage s )
{
	m_lightVertexDecl = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	if( m_VB.GetMemoryClass() & s )
	{
		m_VB.Destroy();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Re-creates light's geometry vertex 
//   declaration and shadow map.
// --------------------------------------------------------------------------------------
bool Tr2InteriorCylinderLight::OnPrepareResources()
{
	Tr2VertexDefinition vd;
	vd.Add( vd.FLOAT32_4, vd.POSITION );
	
	m_lightVertexDecl = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );

	if( m_boundingBox && !m_VB.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_VB.Create( m_vertexCount * sizeof( Vector4 ), USAGE_IMMUTABLE, m_vertexes, renderContext );
	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IBlueAsyncResNotifyTarget interface. Chooses correct material situation 
//   for  projected texture.
// Arguments:
//   p - Blue resource being unloaded (in this case it's a projected texture)
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::ReleaseCachedData( BlueAsyncRes* p )
{
	ChooseLightEffect();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IBlueAsyncResNotifyTarget interface. Chooses correct material situation 
//   for  projected texture.
// Arguments:
//   p - Blue resource being unloaded (in this case it's a projected texture)
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::RebuildCachedData( BlueAsyncRes* p )
{
	ChooseLightEffect();
}

// --------------------------------------------------------------------------------------
// Description:
//   Copies the lighting parameters into the per-object data.
// Arguments:
//   lightData - The per-object light data to populate
//   mirrorToWorldMatrix - Mirror to world space transform
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::PopulateLightData( Tr2InteriorPerObjectLightData* lightData, const Matrix &mirrorToWorldMatrix ) const
{
	Matrix mirrorMatrix;
	D3DXMatrixTranspose( &mirrorMatrix, &mirrorToWorldMatrix );
	D3DXMatrixInverse( &mirrorMatrix, NULL, &mirrorMatrix );

	Matrix worldToLight = m_transform * mirrorMatrix;

	if( m_useKelvinColor )
	{
		Color k = m_kelvinColor->AsRGB();
		lightData->color = Vector3( k.r, k.g, k.b );
	}
	else
	{
		lightData->color = Vector3( m_color.r, m_color.g, m_color.b );
	}
	
	lightData->color = TriGammaToLinear( lightData->color );

	lightData->pointLightFalloff = m_falloff;

	Vector3 position( 0.f, 0.f, 0.f );
	D3DXVec3TransformCoord( &lightData->position, &position, &worldToLight );
 	lightData->radius = m_radius;

	Vector4 direction( 0.f, 0.f, 1.f, 0.f );
	D3DXVec4Transform( reinterpret_cast<Vector4*>( &lightData->spotDirection ), &direction, &worldToLight ); 
	D3DXVec3Normalize( &lightData->spotDirection, &lightData->spotDirection );
	lightData->unused2 = m_length / 2.f;

	float innerAngle = std::max( std::min( m_sectorAngleInner, 180.f ), 0.f );
	float outerAngle = std::max( std::min( m_sectorAngleOuter, 180.f ), 0.f );;
	if( innerAngle > outerAngle )
	{
		innerAngle = outerAngle - 1.f;
	}
	lightData->coneCosAlphaOuter = cosf( XMConvertToRadians( outerAngle ) );
	lightData->coneCosAlphaInner = cosf( XMConvertToRadians( innerAngle ) );
	direction = Vector4( 1.f, 0.f, 0.f, 0.f );
	D3DXVec4Transform( &lightData->boxTransformRow3, &direction, &worldToLight ); 
	D3DXVec3Normalize( reinterpret_cast<Vector3*>( &lightData->boxTransformRow3 ), 
		reinterpret_cast<Vector3*>( &lightData->boxTransformRow3 ) );
	lightData->shadow0Influence = -100.f;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the world-space axis aligned bounding box for the light
// Arguments:
//   minBounds - The min bounds of the BB
//   maxBounds - The max bounds of the BB
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::GetBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
{
	minBounds = Vector3( -0.5f, -0.5f, -0.5f );
	maxBounds = Vector3( 0.5f, 0.5f, 0.5f );

	BoundingBoxTransform( minBounds, maxBounds, m_transform );
}

// ---------------------------------------------------------------------------------------
// Description:
//   Sets the value of the isStatic flags (which controls whether or not the light is
//   cached in Enlighten).
// Arguments:
//   isStatic - The new value of the isStatic flag
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::SetStatic( bool isStatic )
{
	if( isStatic != m_isStatic )
		m_staticFlagChanged = true;
	m_isStatic = isStatic;
}

// --------------------------------------------------------------------------------------
// Description:
//   Tests the light's bounding volume against the bounding box of a cell.  If the light
//   intersects the cell, a ROI is created and the light is added to the cell.  If the
//   light does not intersect, the ROI (if it exists is destroyed) and the light is
//   removed from the cell.  The exact behavior of this function depends on the light
//   intersection type.  If the light has a non-empty explicit cell list, then the light
//   is not tested against the bounding box.
// Arguments:
//   cell - The cell to test against
// --------------------------------------------------------------------------------------
bool Tr2InteriorCylinderLight::TestCellIntersectionAndAdd( Tr2InteriorCell* cell )
{
	// Bail out if the cell is invalid
	if( cell == NULL )
	{
		// No cell, return no intersection
		return false;
	}

	// Get the cell's bounding box
	Vector3 minBounds, maxBounds;
	if( !cell->IsUnbounded() && !cell->GetBoundingBox( minBounds, maxBounds ) )
	{
		// Cell's bounding box not up-to-date, return false (no intersection)
		return false;
	}

	Matrix transformInv;
	D3DXMatrixInverse( &transformInv, NULL, &cell->GetWorldTransform() );

	Vector3 scale, translation;
	Quaternion rotation;
	D3DXMatrixDecompose( &scale, &rotation, &translation, &cell->GetWorldTransform() );
	Vector3 center = ( minBounds + maxBounds ) / 2 + translation;
	Vector3 extents = maxBounds - center;
	bool intersects = cell->IsUnbounded() || IntersectOrientedBoxOrientedBox( 
											m_position, 
											Vector3( m_radius, m_radius, m_length / 2.f + m_radius ), 
											m_rotation, 
											center, 
											extents,
											rotation );

	// If we got an intersection, setup the Umbra Region-of-influence
	if( intersects )
	{
		// Compute current transform matrix
		Matrix transMat;
		transMat = m_transform * transformInv;

		// Only do the ROI setup if the cell doesn't already contain this light
		if( cell->AddLight( this ) )
		{
			// See if we already have an ROI for this cell
			Umbra::RegionOfInfluence* roi = Umbra::RegionOfInfluence::create( m_umbraModel );
			roi->setCell( cell->GetUmbraCell() );
			roi->setObjectToCellMatrix( AS_UMBRA_MATRIX( transMat ) );

			// Set our own userdata to this ROI
			roi->setUserPointer( CONVERT_TO_UMBRA_USER_DATA( this ) );
			roi->set( Umbra::Object::ENABLED, m_affectTransparentObjects && m_enableROIs );

			// Store the roi in our list
			m_umbraRoiList.push_back( roi );

			Umbra::Object* obj = Umbra::Object::create( m_umbraModel );
			obj->setCell( cell->GetUmbraCell() );
			obj->setObjectToCellMatrix( AS_UMBRA_MATRIX( transMat ) );

			obj->setUserPointer( CONVERT_TO_UMBRA_USER_DATA( this ) );

			m_umbraObjectList.push_back( obj );
		}
		else
		{
			// Update the ROI matrix
			for( std::vector<Umbra::RegionOfInfluence*>::iterator it = m_umbraRoiList.begin(); 
				it != m_umbraRoiList.end(); ++it )
			{
				Umbra::RegionOfInfluence* roi = *it;
				if( roi->getCell() == cell->GetUmbraCell() )
				{
					roi->setObjectToCellMatrix( AS_UMBRA_MATRIX( transMat ) );
				}
			}

			// Update the Object matrix
			for( std::vector<Umbra::Object*>::iterator it = m_umbraObjectList.begin(); 
				it != m_umbraObjectList.end(); ++it )
			{
				Umbra::Object* obj = *it;
				if( obj->getCell() == cell->GetUmbraCell() )
				{
					obj->setObjectToCellMatrix( AS_UMBRA_MATRIX( transMat ) );
				}
			}
		}
	}
	else
	{
		// Remove the light from the cell
		cell->RemoveLight( this );

		// Remove the region of influence from the cell
		Umbra::Cell* umbraCell = cell->GetUmbraCell();
		for( std::vector<Umbra::RegionOfInfluence*>::iterator it = m_umbraRoiList.begin(); 
			 it != m_umbraRoiList.end(); )
		{
			Umbra::RegionOfInfluence* roi = *it;
			if( roi->getCell() == umbraCell )
			{
				roi->setCell( NULL );
				roi->setUserPointer( NULL );
				roi->release();
				roi = NULL;

				it = m_umbraRoiList.erase( it );
			}
			else
			{
				++it;
			}
		}

		// Remove the object from the cell
		for( std::vector<Umbra::Object*>::iterator it = m_umbraObjectList.begin(); 
			it != m_umbraObjectList.end(); )
		{
			Umbra::Object* obj = *it;
			if( obj->getCell() == umbraCell )
			{
				obj->setCell( NULL );
				obj->setUserPointer( NULL );
				obj->release();
				obj = NULL;

				it = m_umbraObjectList.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

	// Return the result of the intersection test
	return intersects;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the importance of the light, given the current view position.  The view
//   importance is simply the ratio of the light radius (half of bounding box diagonal) and the 
//   distance to the viewer.
// Arguments:
//   viewerPos - The position of the viewpoint
// Return Value:
//   The view importance
// --------------------------------------------------------------------------------------
float Tr2InteriorCylinderLight::GetCurrentViewImportance( const Vector3& viewerPos ) const
{
	// importance is based on:
	// 1. dist from camera to box center
	Vector3 position( 0.0f, 0.0f, 0.0f );
	D3DXVec3TransformCoord( &position, &position, &m_transform );

	Vector3 dist = viewerPos - position;
	float distToViewer = D3DXVec3LengthSq( &dist );

	Vector3 diagonal = Vector3( m_radius * 2.f, m_radius * 2.f, m_length + 2 * m_radius );

	float radius = D3DXVec3LengthSq( &diagonal ) / 4.f;
	float res = radius / distToViewer;

	return res;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the importance of the light's shadow, given the current view position.  The view
//   importance is simply the ratio of the light radius and the distance to the viewer
//   (scaled by shadowImportance).
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   viewerPos - The position of the viewpoint
// Return Value:
//   The view importance
// --------------------------------------------------------------------------------------
float Tr2InteriorCylinderLight::GetCurrentShadowImportance( unsigned int shadowMapIndex, const Vector3& viewerPos ) const
{
	return 0.f;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Called whenever light is added to interior scene.
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::AddToScene( void )
{
	// Set the dirty flag
	m_isDirty = true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Called whenever light is removed from interior scene.
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::RemoveFromScene( void )
{
	ClearUmbra();
}

// -------------------------------------------------------------
// Description:
//   Called whenever a cell is removed from the scene.
// Arguments:
//   cell - Cell that is removed from the scene
// -------------------------------------------------------------
void Tr2InteriorCylinderLight::CellRemoved( Tr2InteriorCell* cell )
{
}

// -------------------------------------------------------------
// Description:
//   Render light source debug information.
// -------------------------------------------------------------
void Tr2InteriorCylinderLight::RenderDebugInfo( Tr2RenderContext& renderContext ) const
{
	if( !m_renderDebugInfo )
	{
		return;
	}
	if( m_renderDebugType == DI_SHADOW_RESOLUTION || m_renderDebugType == DI_SHADOW_RELATIVE_RESOLUTION )
	{
		return;
	}
	if( m_boundingBox != NULL && m_vertexCount == 0 )
	{
		return;
	}

	Color color;

	switch( m_renderDebugType )
	{
	case DI_LIGHT_COLOR:
		if( m_useKelvinColor )
		{
			color = m_kelvinColor->AsRGB();
		}
		else
		{
			color = m_color;
		}
		break;
	default:
		color = 0x00ffffff;
	}

	if( m_boundingBox )
	{
		RenderDebugLightVolume(
			reinterpret_cast<const Vector3*>( m_vertexes ),
			m_vertexCount,
			sizeof( Vector4 ),
			nullptr,
			m_vertexCount / 3,
			nullptr,
			0,
			Tr2Renderer::GetIdentityTransform(),
			color,
			renderContext
			);
	}
	else
	{
		RenderDebugLightVolume(
			reinterpret_cast<const Vector3*>( unitCylinderVertices ),
			unitCylinderVertexCount,
			sizeof( Vector4 ),
			unitCylinderIndices,
			unitCylinderTriangleCount,
			unitCylinderWireIndices,
			unitCylinderWireLineCount,
			m_transform,
			color,
			renderContext
			);
	}
}

// -------------------------------------------------------------
// Description:
//   Per-frame update method. Updates curve sets.
// Arguments:
//   time - Current system time.
// -------------------------------------------------------------
void Tr2InteriorCylinderLight::Update( Be::Time time )
{
	for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		( *it )->Update( TimeAsDouble( time ) );
	}
}

// -------------------------------------------------------------
// Description:
//   Enables/disables Umbra regions of influeces for this light
//   source.
// Arguments:
//   enable - If true enable Umbra ROI; 
//            if false disable Umbra ROI
// -------------------------------------------------------------
void Tr2InteriorCylinderLight::EnableROI( bool enable )
{
	if( m_enableROIs != enable )
	{
		m_enableROIs = enable;
		for( std::vector<Umbra::RegionOfInfluence*>::iterator it = m_umbraRoiList.begin(); it != m_umbraRoiList.end(); ++it )
		{
			( *it )->set( Umbra::Object::ENABLED, m_affectTransparentObjects && m_enableROIs );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Per-object data for light rendering during light accumulation pass in pre-pass scenes.
//   Sets a shadow map texture and projection texture.
// --------------------------------------------------------------------------------------
class Tr2InteriorCylinderLight::PerLightData : public Tr2ProjectedPerLightData
{
};

// --------------------------------------------------------------------------------------
// Description:
//   Adds light's geometry to a render batch accumulator. Used during light accumulation 
//   pass.
// Arguments:
//   batches - Render batch accumulator to add batch to
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::GetBatches( ITriRenderBatchAccumulator* batches )
{
	GetInstancedBatches( batches, Tr2Renderer::GetIdentityTransform() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds light's geometry to a render batch accumulator. Used during light accumulation 
//   pass.
// Arguments:
//   batches - Render batch accumulator to add batch to
//   mirrorToWorldMatrix - Mirror to world space transform.
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::GetInstancedBatches( ITriRenderBatchAccumulator* batches,
												  const Matrix& mirrorToWorldMatrix )
{
	if( m_boundingBox != NULL && m_vertexCount == 0 )
	{
		return;
	}

	PerLightData* data = batches->Allocate<PerLightData>(); 

	if( !data )
	{
		return;
	}

	data->SetProjectedTexture( m_projectedTextureRes );

	Tr2InteriorPerLightPSData perObjectPSBuffer;

	if( m_boundingBox )
	{
		D3DXMatrixInverse( &perObjectPSBuffer.boundingBox, NULL, &m_boundingBox->GetTransform() );
		D3DXMatrixTranspose( &perObjectPSBuffer.boundingBox, &perObjectPSBuffer.boundingBox );
	}

	PopulateLightData( &perObjectPSBuffer.lightData, mirrorToWorldMatrix );
	perObjectPSBuffer.additionalParameters.x = m_specularIntensity;

	Matrix mirrorMatrix;
	D3DXMatrixTranspose( &mirrorMatrix, &mirrorToWorldMatrix );
	D3DXMatrixInverse( &mirrorMatrix, NULL, &mirrorMatrix );

	D3DXMatrixRotationQuaternion( &perObjectPSBuffer.mirrorToWorldMatrix, &m_rotation );
	D3DXMatrixInverse( &perObjectPSBuffer.mirrorToWorldMatrix, NULL, &perObjectPSBuffer.mirrorToWorldMatrix );
	perObjectPSBuffer.mirrorToWorldMatrix = mirrorMatrix * perObjectPSBuffer.mirrorToWorldMatrix;
	D3DXMatrixTranspose( &perObjectPSBuffer.mirrorToWorldMatrix, &perObjectPSBuffer.mirrorToWorldMatrix );

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	const Vector4* vertexes;
	unsigned vertexCount;
	Matrix worldView, worldViewTranspose, worldViewProj;
	Tr2VertexBufferAL* vb;
	Tr2IndexBufferAL* ib;

	if( m_boundingBox != NULL )
	{
		vertexes = m_vertexes;
		vertexCount = m_vertexCount;
		worldView = mirrorMatrix * Tr2Renderer::GetViewTransform();
		vb = &m_VB;
		ib = nullptr;
	}
	else
	{
		vertexes = unitCylinderVertices;
		vertexCount = unitCylinderVertexCount;
		worldView = m_transform * mirrorMatrix * Tr2Renderer::GetViewTransform();
		vb = m_sharedVB;
		ib = m_sharedIB;
	}

	worldViewProj = worldView * Tr2Renderer::GetProjectionTransform();
	D3DXMatrixTranspose( &worldViewProj, &worldViewProj );
	data->CopyToVSFloatBuffer( worldViewProj );

	bool isInside = false;
	float det = D3DXMatrixDeterminant( &mirrorMatrix );
	if( det < 0 )
	{
		isInside = true;
	}
	else
	{
		XMVECTOR nearPlane = Vector4( 0, 0, -1, -Tr2Renderer::GetFrontClip() );
		D3DXMatrixTranspose(&worldViewTranspose, &worldView);
		nearPlane = XMPlaneTransform( nearPlane, worldViewTranspose );

		for( unsigned i = 0; i < vertexCount; ++i )
		{
			if( XMVectorGetX( XMPlaneDot( nearPlane, vertexes[i] ) ) <= 0 )
			{
				isInside = true;
				break;
			}
		}
	}

	Tr2InteriorLightGeometryRenderBatch* batch = batches->Allocate<Tr2InteriorLightGeometryRenderBatch>();
	if( batch )
	{
		batch->Initialize( vb, ib, m_lightVertexDecl, isInside );
		batch->SetShaderMaterial( m_material );
		batch->SetPerObjectData( data );
		batches->Commit( batch );
	}
}

// -------------------------------------------------------------
// Description:
//   Returns the number of shadow map textures in shadow atlas
//   required for this light source. Cylindrical lights do not 
//   currently support shadows.
// Return Value:
//   0 always
// -------------------------------------------------------------
unsigned int Tr2InteriorCylinderLight::GetRequiredShadowMapCount() const
{
	return 0;
}

// -------------------------------------------------------------
// Description:
//   Returns dimensions of shadow map required for this light.
//   Does nothing since cylindrical lights do not currently 
//   support shadows.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   maximumResolution - If true always return light maximum 
//						 shadow resolution, otherwise can apply 
//						 LOD scheme.
//   width (out) - Width of shadow map
//   height (out) - Height of shadow map
// -------------------------------------------------------------
void Tr2InteriorCylinderLight::GetRequiredShadowMapResolution( unsigned int index, bool maximumResolution, unsigned int& width, unsigned int& height ) const
{
}

// -------------------------------------------------------------
// Description:
//   Caches shadow map view-dependent resolution values for a 
//   light source. Is called every frame before any calls to 
//   GetRequiredShadowMapResolution.
//   Does nothing since cylindrical lights do not currently 
//   support shadows.
// -------------------------------------------------------------
void Tr2InteriorCylinderLight::CacheShadowMapResolution()
{
}

// -------------------------------------------------------------
// Description:
//   Returns atlas texture containing the given shadow map. Can
//   return NULL if the texture is not required of was not yet
//   assigned.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   NULL always
// -------------------------------------------------------------
Tr2AtlasTexture* Tr2InteriorCylinderLight::GetShadowAtlasTexture( unsigned int index ) const
{
	return NULL;
}

// -------------------------------------------------------------
// Description:
//   Assigns atlas texture for the given shadow map. 
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   texture - Atlas texture containing the shadow map
// -------------------------------------------------------------
void Tr2InteriorCylinderLight::SetShadowAtlasTexture( unsigned int index, Tr2AtlasTexture* texture )
{
}

// -------------------------------------------------------------
// Description:
//   Set shadow empty flag. The interior scene checks if the 
//   shadow map frustum has any shadow geometry to render and
//   if it doesn't then the scene removes this shadow map from
//   atlas and marks it as empty. 
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   emptyShadow Flag indicating that the shadow map is empty.
// -------------------------------------------------------------
void Tr2InteriorCylinderLight::SetEmptyShadow( unsigned int shadowMapIndex, bool emptyShadow )
{
}

// -------------------------------------------------------------
// Description:
//   Returns a flag indicating if the shadow map is empty 
//   (shadow frustum does not contain any shadow geometry to 
//   render).
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   true If the shadow map is empty
//   false Otherwise
// -------------------------------------------------------------
bool Tr2InteriorCylinderLight::IsShadowEmpty( unsigned int shadowMapIndex ) const
{
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets up shadow map render target and fills per-frame data provided. Called before
//	 rendering into spotlight shadow map. 
//   Does nothing since cylindrical lights do not currently 
//   support shadows.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   vsData - per-frame vertex shader data
//   psData - per-frame pixel shader data
// Return Value:
//   false always
// --------------------------------------------------------------------------------------
bool Tr2InteriorCylinderLight::BeginShadowUpdate( unsigned int shadowMapIndex, Tr2PerFrameVSData* vsData, Tr2PerFrameShadowPSData* psData, Tr2RenderContext& renderContext )
{
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Restores render target previously set in a call to BeginShadowUpdate. Does nothing.
//   Does nothing since cylindrical lights do not currently 
//   support shadows.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::EndShadowUpdate( unsigned int shadowMapIndex )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns shadow view matrix.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Identity matrix.
// --------------------------------------------------------------------------------------
const Matrix& Tr2InteriorCylinderLight::GetViewMatrix( unsigned int shadowMapIndex ) const
{
	return Tr2Renderer::GetIdentityTransform();
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns inverse of spotlight view matrix.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Identity matrix
// --------------------------------------------------------------------------------------
const Matrix& Tr2InteriorCylinderLight::GetInvViewMatrix( unsigned int shadowMapIndex ) const
{
	return Tr2Renderer::GetIdentityTransform();
}

// -------------------------------------------------------------
// Description:
//   Returns projection transform matrix for shadow map.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Identity matrix.
// -------------------------------------------------------------
const Matrix& Tr2InteriorCylinderLight::GetProjectionMatrix( unsigned int shadowMapIndex ) const
{
	return Tr2Renderer::GetIdentityTransform();
}

// -------------------------------------------------------------
// Description:
//   Marks light's shadow maps that are affected by given bounding
//   box as dirty. The bounding box usually represents moved dynamic
//   object.
//   Does nothing since cylindrical lights do not currently 
//   support shadows.
// Arguments:
//   minBounds - Min bounds of a box in world space
//   maxBounds - Max bounds of a box in world space
// -------------------------------------------------------------
void Tr2InteriorCylinderLight::MarkShadowsDirtyForBounds( const Vector3 &minBounds, const Vector3 &maxBounds )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets spotlight shadow dirty flag. Dirty spotlight shadows are to be updated during
//   subsequent scene render.
//   Does nothing since cylindrical lights do not currently 
//   support shadows.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   dirty - spotlight shadow dirty flag
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::MarkShadowDirty(  unsigned int shadowMapIndex, bool dirty )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a flag indicating if the spotlight shadow is dirty.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   false always
// --------------------------------------------------------------------------------------
bool Tr2InteriorCylinderLight::IsShadowDirty( unsigned int shadowMapIndex ) const
{
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns type of objects rendered into the shadow map.
// Return Value:
//   ST_NONE always
// --------------------------------------------------------------------------------------
Tr2InteriorCylinderLight::ShadowCasterTypes Tr2InteriorCylinderLight::GetShadowCasterTypes() const
{
	return ST_NONE;
}

// -------------------------------------------------------------
// Description:
//   Returns light's position for "light spider" tool.
// Return Value:
//   Position to render a ray to for "light spider" tool.
// -------------------------------------------------------------
Vector3 Tr2InteriorCylinderLight::GetPositionForLightSpider( const Matrix &objectToWorldMatrix ) const
{
	Vector3 position( 0.0f, 0.0f, 0.0f );
	D3DXVec3TransformCoord( &position, &position, &m_transform );
	D3DXVec3TransformCoord( &position, &position, &objectToWorldMatrix );
	return position;
}

// -------------------------------------------------------------
// Description:
//   Get batches for a light to use during SH lighting step.
// Arguments:
//   batches - Batch accumulator to add batches to.
// -------------------------------------------------------------
void Tr2InteriorCylinderLight::GetSHBatches( ITriRenderBatchAccumulator* batches ) const
{
	if( !m_primaryLighting )
	{
		return;
	}

	if( m_boundingBox != NULL && m_vertexCount == 0 )
	{
		return;
	}

	PerLightData* data = batches->Allocate<PerLightData>();

	if( !data )
	{
		return;
	}

	data->SetProjectedTexture( m_projectedTextureRes );

	Tr2InteriorPerLightPSData perObjectPSBuffer;

	if( m_boundingBox )
	{
		D3DXMatrixInverse( &perObjectPSBuffer.boundingBox, NULL, &m_boundingBox->GetTransform() );
		D3DXMatrixTranspose( &perObjectPSBuffer.boundingBox, &perObjectPSBuffer.boundingBox );
	}

	PopulateLightData( &perObjectPSBuffer.lightData, Tr2Renderer::GetIdentityTransform() );
	perObjectPSBuffer.additionalParameters.x = m_specularIntensity;

	D3DXMatrixRotationQuaternion( &perObjectPSBuffer.mirrorToWorldMatrix, &m_rotation );
	D3DXMatrixInverse( &perObjectPSBuffer.mirrorToWorldMatrix, NULL, &perObjectPSBuffer.mirrorToWorldMatrix );
	D3DXMatrixTranspose( &perObjectPSBuffer.mirrorToWorldMatrix, &perObjectPSBuffer.mirrorToWorldMatrix );

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	Tr2InteriorFullScreenLightBatch* batch = batches->Allocate<Tr2InteriorFullScreenLightBatch>();
	if( batch )
	{
		batch->SetVertexDeclaration( m_lightVertexDecl );
		batch->SetShaderMaterial( m_shMaterial );
		batch->SetPerObjectData( data );

		batches->Commit( batch );
	}
}

// -------------------------------------------------------------
// Description:
//   Get batches for a lightmap rendering.
// Arguments:
//   batches - Batch accumulator to add batches to.
// -------------------------------------------------------------
void Tr2InteriorCylinderLight::GetLightMapBatches( ITriRenderBatchAccumulator* batches ) const
{
	if( !m_primaryLighting )
	{
		return;
	}

	if( m_boundingBox != NULL && m_vertexCount == 0 )
	{
		return;
	}

	PerLightData* data = batches->Allocate<PerLightData>();

	if( !data )
	{
		return;
	}

	data->SetProjectedTexture( m_projectedTextureRes );

	Tr2InteriorPerLightPSData perObjectPSBuffer;

	if( m_boundingBox )
	{
		D3DXMatrixInverse( &perObjectPSBuffer.boundingBox, NULL, &m_boundingBox->GetTransform() );
		D3DXMatrixTranspose( &perObjectPSBuffer.boundingBox, &perObjectPSBuffer.boundingBox );
	}

	PopulateLightData( &perObjectPSBuffer.lightData, Tr2Renderer::GetIdentityTransform() );
	perObjectPSBuffer.additionalParameters.x = m_specularIntensity;

	D3DXMatrixRotationQuaternion( &perObjectPSBuffer.mirrorToWorldMatrix, &m_rotation );
	D3DXMatrixInverse( &perObjectPSBuffer.mirrorToWorldMatrix, NULL, &perObjectPSBuffer.mirrorToWorldMatrix );
	D3DXMatrixTranspose( &perObjectPSBuffer.mirrorToWorldMatrix, &perObjectPSBuffer.mirrorToWorldMatrix );

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	Tr2InteriorFullScreenLightBatch* batch = batches->Allocate<Tr2InteriorFullScreenLightBatch>();
	if( batch )
	{
		batch->SetVertexDeclaration( m_lightVertexDecl );
		batch->SetShaderMaterial( m_lightmapMaterial );
		batch->SetPerObjectData( data );

		batches->Commit( batch );
	}
}


// --------------------------------------------------------------------------------------
// Description:
//   Removes the cell from the Umbra scene by destroying all the ROI's and the Umbra 
//   model
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::ClearUmbra( void )
{
	// Clear the regions of influence
	for( std::vector<Umbra::RegionOfInfluence*>::iterator it = m_umbraRoiList.begin(); 
		 it != m_umbraRoiList.end(); ++it )
	{
		( *it )->setCell( NULL );
		( *it )->setUserPointer( NULL );
		( *it )->release();
		( *it ) = NULL;
	}

	m_umbraRoiList.clear();

	// Clear the objects
	for( std::vector<Umbra::Object*>::iterator it = m_umbraObjectList.begin();
		 it != m_umbraObjectList.end(); ++it )
	{
		( *it )->setCell( NULL );
		( *it )->setUserPointer( NULL );
		( *it )->release();
		( *it ) = NULL;
	}

	m_umbraObjectList.clear();

	// Clear the Umbra model
	m_umbraModel->release();
	m_umbraModel = NULL;
}

// --------------------------------------------------------------------------------------
// Description:
//   Rebuilds the Umbra model and sets it as the model for all the current ROIs
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::RebuildVolume( void )
{
	// Clear the Umbra model
	m_umbraModel->release();
	m_umbraModel = NULL;

	// Build a volume for point light
	// Build the Umbra model
	m_umbraModel = ( Umbra::Model* )Umbra::OBBModel::create( AS_UMBRA_MATRIX( Tr2Renderer::GetIdentityTransform() ) );

	// Set the new umbra test model for all the regions of influence
	for( std::vector<Umbra::RegionOfInfluence*>::iterator it = 
		 m_umbraRoiList.begin(); it != m_umbraRoiList.end(); ++it )
	{
		( *it )->setTestModel( m_umbraModel );
	}

	// Set the new umbra test model for all the umbra objects
	for( std::vector<Umbra::Object*>::iterator it = 
		m_umbraObjectList.begin(); it != m_umbraObjectList.end(); ++it )
	{
		( *it )->setTestModel( m_umbraModel );
	}

	// Set the dirty flag
	m_isDirty = true;
}

// -------------------------------------------------------------
// Description:
//   Functor to color dusters in Tr2InteriorDusterCache.
// -------------------------------------------------------------
__declspec( align( 16 ) ) struct Tr2InteriorCylinderLight::ColorDusters
{
	// Light's min world bounds
	XMVECTOR m_minBounds;
	// Light's max world bounds
	XMVECTOR m_maxBounds;

	// Light center
	XMVECTOR m_position;
	// Normalized cylinder axis
	XMVECTOR m_direction;
	// Negated half cylinder length: -length / 2
	XMVECTOR m_minHalfLength;
	// Half cylinder length: length / 2
	XMVECTOR m_maxHalfLength;
	// Inverse of cylinder radius
	XMVECTOR m_radiusInv;
	// Light falloff power
	XMVECTOR m_falloff;
	// Light's color
	XMVECTOR m_color;
	// Duster position array
	const XMVECTOR* m_positions;
	// Duster normal array
	const XMVECTOR* m_normals;
	// Duster albedo array
	XMVECTOR* m_albedoes;
	// Duster color array
	XMVECTOR* m_colors;

	// -------------------------------------------------------------
	// Description:
	//   Colors dusters in Tr2InteriorDusterCache octree node.
	// Arguments:
	//   indexes - Array of duster indexes
	//   count - Number of dusters
	// -------------------------------------------------------------
	void operator()( const unsigned int* indexes, unsigned int count )
	{
		static const XMVECTOR half = Vector4( 0.5f, 0.5f, 0.5f, 0.5f );
		static const XMVECTOR zero = Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
		static const XMVECTOR one = Vector4( 1.0f, 1.0f, 1.0f, 1.0f );
		static const XMVECTOR invalidAlbedo = Vector4( -1.0f, -1.0f, -1.0f, -1.0f );
		static const XMVECTOR pie = Vector4( D3DX_PI, D3DX_PI, D3DX_PI, D3DX_PI );

		for( unsigned int i = 0; i < count; ++i )
		{
			unsigned int index = indexes[i];
			if( XMVector3GreaterOrEqual( m_positions[index], m_minBounds ) && 
				XMVector3LessOrEqual( m_positions[index], m_maxBounds ) )
			{
				XMVECTOR direction = XMVectorSubtract( m_positions[index], m_position );
				XMVECTOR t = XMVectorClamp( XMVector3Dot( direction, m_direction ), m_minHalfLength, m_maxHalfLength );
				XMVECTOR position = XMVectorAdd( m_position, XMVectorMultiply( m_direction, t ) );

				direction = XMVectorSubtract( position, m_positions[index] );
				XMVECTOR length = XMVector3Length( direction );
				if( !XMVector3Equal( length, zero ) )
				{
					direction = XMVectorMultiply( direction, XMVectorReciprocal( length ) );
				}

				XMVECTOR falloffRadius = XMVectorPowEst( XMVectorMin( XMVectorMultiply( length, m_radiusInv ), one ), m_falloff );
				XMVECTOR falloff = XMVectorSubtract( one, falloffRadius );

				XMVECTOR dot = XMVectorMax( XMVectorMultiply( XMVector3Dot( direction, m_normals[index] ), falloff ), zero );

				m_colors[index] = XMVectorAdd( m_colors[index], XMVectorMultiply( dot, m_color ) );
			}
		}
	}
};

// --------------------------------------------------------------------------------------
// Description:
//   Adds light source to Enlighten system
// Arguments:
//   dusters - Enlighten lighting/duster cache
//	 systemTransformInv - World to Enlighten system transform
// Return value:
//   true If light source was added to Enlighten system
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2InteriorCylinderLight::AddToEnlightenSystem( Tr2InteriorDusterCache* dusters,
													 const Matrix &systemTransformInv ) const
{
	if( !m_secondaryLighting || m_radius <= 0.f )
	{
		return false;
	}

	ColorDusters colorDusters;

	colorDusters.m_position = m_position;
	Vector4 axis( 0.f, 0.f, 1.f, 0.f );
	D3DXVec4Transform( &axis, &axis, &m_transform );
	D3DXVec3Normalize( reinterpret_cast<Vector3*>( &axis ), reinterpret_cast<Vector3*>( &axis ) );
	colorDusters.m_direction = axis;
	colorDusters.m_minHalfLength = XMVectorReplicate( -m_length * 0.5f );
	colorDusters.m_maxHalfLength = XMVectorReplicate( m_length * 0.5f );
	colorDusters.m_radiusInv = XMVectorReplicate( 1.f / m_radius );
	colorDusters.m_falloff = XMVectorReplicate( m_falloff );

	Vector3 boundsMin, boundsMax;
	GetBoundingBox( boundsMin, boundsMax );

	colorDusters.m_minBounds = boundsMin;
	colorDusters.m_maxBounds = boundsMax;
	Color color = m_color;
	if( m_useKelvinColor )
	{
		color = m_kelvinColor->AsRGB();
	}
	colorDusters.m_color = TriGammaToLinear( Vector4( color.r, color.g, color.b, 0.0f ) ) * m_secondaryLightingMultiplier;

	colorDusters.m_positions = dusters->GetPositions();
	colorDusters.m_normals = dusters->GetNormals();
	colorDusters.m_colors = dusters->GetDusterMemory();

	dusters->ProcessDustersInBounds( boundsMin, boundsMax, colorDusters );

	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Chooses an appropriate effect for light rendering during light accumulation pass.
//   Accounts for if there is an assigned projected texture.
// --------------------------------------------------------------------------------------
void Tr2InteriorCylinderLight::ChooseLightEffect()
{
	Tr2ShaderSituation situation;
	if( m_projectedTextureRes && m_projectedTextureRes->IsGood() )
	{
		situation.AddSituationString( "ProjectedTexture" );
	}
	if( m_boundingBox )
	{
		situation.AddSituationString( "AdditionalBoundingBox" );
	}
	if( m_sectorAngleOuter < 180.0 || m_sectorAngleInner < 180.0 )
	{
		situation.AddSituationString( "Sector" );
	}

	m_material->BindLowLevelShader( situation );

	situation.AddSituationString( "GenerateSHCoefficients" );
	m_shMaterial->BindLowLevelShader( situation );
	situation.AddSituationString( "GenerateLightMap" );
	m_lightmapMaterial->BindLowLevelShader( situation );
}

// -------------------------------------------------------------
// Description:
//  Function to clip given geometry with a plane. The geometry
//	is given as an array of triangles. Function returns new 
//	geometry that lies in the negative halfspace formed by a plane.
// Arguments:
//  geometryBegin - Iterator to the beginning of geometry vertexes.
//  geometryEnd - Iterator to the end of geometry vertexes.
//	plane - Plane to clip geometry.
//	result - A vector with vertexes of the resulting geometry.
// -------------------------------------------------------------
template<typename GeometryIterator>
static void ClipGeometryByPlane(
	GeometryIterator geometryBegin,
	GeometryIterator geometryEnd,
	const D3DXPLANE& plane,
	std::vector<Vector4>& result
	)
{
	for( GeometryIterator jt = geometryBegin; jt != geometryEnd; )
	{
		Vector4 vertex[3];
		vertex[0] = *jt;
		++jt;
		vertex[1] = *jt;
		++jt;
		vertex[2] = *jt;
		++jt;

		Vector4 outPolygon[12];
		int outCount = 0;

		for( int edge = 0; edge < 3; ++edge )
		{
			const Vector4& vertex1 = vertex[edge];
			const Vector4& vertex2 = vertex[( edge + 1 ) % 3];
			float v0 = D3DXPlaneDot( &plane, &vertex1 );
			float v1 = D3DXPlaneDot( &plane, &vertex2 );
			if( v0 <= 0 )
			{
				outPolygon[outCount++] = vertex[edge];
				CCP_ASSERT( outCount < 12 );
			}
			if( v0 * v1 < 0 )
			{
				Vector3 result;
				D3DXPlaneIntersectLine( &result, &plane, (Vector3*)&vertex1, (Vector3*)&vertex2 );
				outPolygon[outCount++] = Vector4( result, 1.0f );
				CCP_ASSERT( outCount < 12 );
			}
		}

		for( int edge = 2; edge < outCount; ++edge )
		{
			result.push_back( outPolygon[0] );
			result.push_back( outPolygon[edge - 1] );
			result.push_back( outPolygon[edge] );
		}
	}
}

// -------------------------------------------------------------
// Description:
//  Function to clip given geometry with another geometry. Both
//	geometries are given as arrays of triangles. Function returns 
//	a subset of the given geometry that lies inside the clipping
//  geometry.
// Arguments:
//  geometryBegin - Iterator to the beginning of geometry vertexes.
//  geometryEnd - Iterator to the end of geometry vertexes.
//  clipGeometryBegin - Iterator to the beginning of clipping 
//		geometry vertexes.
//  clipGeometryEnd - Iterator to the end of clipping geometry 
//		vertexes.
//	result - A vector with vertexes of the resulting geometry.
// -------------------------------------------------------------
template<typename GeometryIterator, typename ClippingIterator>
static void ClipGeometry(
	GeometryIterator geometryBegin,
	GeometryIterator geometryEnd,
	ClippingIterator clipGeometryBegin,
	ClippingIterator clipGeometryEnd,
	std::vector<Vector4>& result
	)
{
	std::vector<Vector4> buffer;
	std::vector<Vector4>* input = &result;
	std::vector<Vector4>* output = &buffer;
	bool firstIteration = true;
	
	for( ClippingIterator it = clipGeometryBegin; it != clipGeometryEnd; )
	{
		const Vector3& clip0 = (const Vector3&)*it;
		++it;
		const Vector3& clip1 = (const Vector3&)*it;
		++it;
		const Vector3& clip2 = (const Vector3&)*it;
		++it;

		D3DXPLANE plane;
		D3DXPlaneFromPoints( &plane, &clip0, &clip1, &clip2 );

		output->clear();

		if( firstIteration )
		{
			ClipGeometryByPlane( geometryBegin, geometryEnd, plane, *output );
			firstIteration = false;
		}
		else
		{
			ClipGeometryByPlane( input->begin(), input->end(), plane, *output );
		}

		std::swap( input, output );
		if( input->empty() )
		{
			break;
		}
	}
	if( input != &result )
	{
		result.clear();
		result.assign( input->begin(), input->end() );
	}
}

// -------------------------------------------------------------
// Description:
//  Rebuilds the light geometry clipped by the bounding box.
// -------------------------------------------------------------
void Tr2InteriorCylinderLight::RebuildGeometry()
{
	CCP_DELETE []m_vertexes;
	m_vertexes = NULL;
	m_vertexCount = 0;

	if( m_boundingBox )
	{
		std::vector<Vector4> lightVertexes;
		for( unsigned i = 0; i < sizeof( unitCylinderIndices ) / sizeof( unsigned ); ++i )
		{
			lightVertexes.push_back( unitCylinderVertices[unitCylinderIndices[i]] );
		}
		XMVector4TransformStream( (XMFLOAT4*)&lightVertexes[0], 
								  sizeof( Vector4 ), 
								  (XMFLOAT4*)&lightVertexes[0], 
								  sizeof( Vector4 ), 
								  (unsigned int)lightVertexes.size(),
								  m_transform );

		Vector3 sides[6][4] = {
			{ 
				Vector3( -0.5f, -0.5f, -0.5f ), 
				Vector3( -0.5f, 0.5f, -0.5f ), 
				Vector3( -0.5f, 0.5f, 0.5f ), 
				Vector3( -0.5f, -0.5f, 0.5f ), 
			},
			{ 
				Vector3( 0.5f, -0.5f, -0.5f ), 
				Vector3( 0.5f, -0.5f, 0.5f ), 
				Vector3( 0.5f, 0.5f, 0.5f ), 
				Vector3( 0.5f, 0.5f, -0.5f ), 
			},
			{ 
				Vector3( -0.5f, -0.5f, -0.5f ), 
				Vector3( -0.5f, -0.5f, 0.5f ), 
				Vector3( 0.5f, -0.5f, 0.5f ), 
				Vector3( 0.5f, -0.5f, -0.5f ), 
			},
			{ 
				Vector3( -0.5f, 0.5f, -0.5f ), 
				Vector3( 0.5f, 0.5f, -0.5f ), 
				Vector3( 0.5f, 0.5f, 0.5f ), 
				Vector3( -0.5f, 0.5f, 0.5f ), 
			},
			{ 
				Vector3( -0.5f, -0.5f, -0.5f ), 
				Vector3( 0.5f, -0.5f, -0.5f ), 
				Vector3( 0.5f, 0.5f, -0.5f ), 
				Vector3( -0.5f, 0.5f, -0.5f ), 
			},
			{ 
				Vector3( -0.5f, -0.5f, 0.5f ), 
				Vector3( -0.5f, 0.5f, 0.5f ), 
				Vector3( 0.5f, 0.5f, 0.5f ), 
				Vector3( 0.5f, -0.5f, 0.5f ), 
			},
		};

		std::vector<Vector4> boxVertexes;
		for( unsigned i = 0; i < 6; ++i )
		{
			Vector4 vertex;
			D3DXVec3TransformCoord( (Vector3*)&vertex, &sides[i][0], &m_boundingBox->GetTransform() );
			vertex.w = 1;
			boxVertexes.push_back( vertex );
			D3DXVec3TransformCoord( (Vector3*)&vertex, &sides[i][2], &m_boundingBox->GetTransform() );
			vertex.w = 1;
			boxVertexes.push_back( vertex );
			D3DXVec3TransformCoord( (Vector3*)&vertex, &sides[i][1], &m_boundingBox->GetTransform() );
			vertex.w = 1;
			boxVertexes.push_back( vertex );
			D3DXVec3TransformCoord( (Vector3*)&vertex, &sides[i][3], &m_boundingBox->GetTransform() );
			vertex.w = 1;
			boxVertexes.push_back( vertex );
			boxVertexes.push_back( boxVertexes[boxVertexes.size() - 3] );
			boxVertexes.push_back( boxVertexes[boxVertexes.size() - 5] );
		}

		std::vector<Vector4> clippedLight;
		std::vector<Vector4> clippedBox;
		ClipGeometry( lightVertexes.begin(), lightVertexes.end(), boxVertexes.begin(), boxVertexes.end(), clippedLight );
		ClipGeometry( boxVertexes.begin(), boxVertexes.end(), lightVertexes.begin(), lightVertexes.end(), clippedBox );

		m_vertexCount = (unsigned int)clippedLight.size() + (unsigned int)clippedBox.size();
		if( m_vertexCount )
		{
			m_vertexes = CCP_NEW( "Tr2InteriorCylinderLight/m_vertexes" ) Vector4[m_vertexCount];
			if( !clippedLight.empty() )
			{
				memcpy( m_vertexes, &clippedLight[0], clippedLight.size() * sizeof( Vector4 ) );
			}
			if( !clippedBox.empty() )
			{
				memcpy( m_vertexes + clippedLight.size(), &clippedBox[0], clippedBox.size() * sizeof( Vector4 ) );
			}
		}

		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_VB.Create( m_vertexCount * sizeof( Vector4 ), USAGE_IMMUTABLE, m_vertexes, renderContext );
	}
}

// -------------------------------------------------------------
// Description:
//   Blue-exposed function that returns AABB for the light in its
//   local coordinate space.
// Return value:
//   AABB for the light in its local coordinate space
// -------------------------------------------------------------
AxisAlignedBoundingBox Tr2InteriorCylinderLight::GetBoundingBoxInLocalSpace() const
{
	return AxisAlignedBoundingBox( 
		Vector3( -GetRadius(), -GetRadius(), -GetLength() / 2.f - GetRadius() ), 
		Vector3( GetRadius(), GetRadius(), GetLength() / 2.f + GetRadius() ) );
}

#endif

