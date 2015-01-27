////////////////////////////////////////////////////////////
//
//    Created:   July 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"

#include "Tr2InteriorBoxLight.h"

#include "Tr2InteriorLightGeometryRenderBatch.h"
#include "Tr2InteriorCell.h"
#include "Tr2AtlasTexture.h"
#include "Tr2KelvinColor.h"
#include "TriFrustum.h"
#include "Tr2ShaderMaterial.h"
#include "Tr2ConstGeometry.h"
#include "Curves/TriCurveSet.h"

CCP_STATS_DECLARED_ELSEWHERE( wodIntLightsAlive );

using namespace Tr2RenderContextEnum;
	
namespace {
    unsigned int const unitCubeIndices[] =
    {
        6, 4, 5,
        5, 7, 6,
        2, 1, 0,
        1, 2, 3,
        7, 5, 1,
        7, 1, 3,
        2, 0, 4,
        2, 4, 6,
        5, 4, 0,
        5, 0, 1,
        2, 6, 7, 
        2, 7, 3,
    };
    unsigned int const unitCubeWireIndices[] =
    {
		0,	1,
		0,	2,
		0,	4,
		1,	3,
		1,	5,
		2,	3,
		2,	6,
		3,	7,
		4,	5,
		4,	6,
		5,	7,
		6,	7,
    };
    Vector4 const unitCubeVertices[] =
    {
        Vector4( -0.5f,  0.5f,  0.5f, 1.0f ),
        Vector4( 0.5f,  0.5f,  0.5f, 1.0f ),
        Vector4( -0.5f, -0.5f,  0.5f, 1.0f ),
        Vector4( 0.5f, -0.5f,  0.5f, 1.0f ),
        Vector4( -0.5f,  0.5f, -0.5f, 1.0f ),
        Vector4( 0.5f,  0.5f, -0.5f, 1.0f ),
        Vector4( -0.5f, -0.5f, -0.5f, 1.0f ),
        Vector4( 0.5f, -0.5f, -0.5f, 1.0f ),
    };

	static const unsigned int unitCubeVertexCount = sizeof( unitCubeVertices ) / sizeof( Vector4 );
	static const unsigned int unitCubeTriangleCount = sizeof( unitCubeIndices ) / sizeof( unsigned int ) / 3;
	static const unsigned int unitCubeWireLineCount = sizeof( unitCubeWireIndices ) / sizeof( unsigned int ) / 2;
} // anonymous

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorBoxLight default constructor
// --------------------------------------------------------------------------------------
Tr2InteriorBoxLight::Tr2InteriorBoxLight( IRoot* lockobj ) :
	m_name(),
	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_falloff( 1.f ),
	m_specularIntensity( 1.f ),
	m_shadowImportance( 1.f ),
	m_primaryLighting( true ),
	m_affectTransparentObjects( true ),
	m_isDirty( true ),
	m_lightVertexDecl( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_shadowResolution( 256 ),
	m_enableShadowLOD( true ),
	m_framesSinceShadowUpdate( 0 ),
	m_emptyShadow( false ),
	m_dirtyShadow( true ),
	m_shadowCasterTypes( ST_ALL ),
	PARENTLOCK( m_curveSets )
{
	CCP_STATS_INC( wodIntLightsAlive );

	D3DXMatrixIdentity( &m_transform );
	m_worldBoundingBox = AxisAlignedBoundingBox( Vector3( -0.5f, -0.5f, -0.5f ), Vector3( 0.5f, 0.5f, 0.5f ) );

	m_material.CreateInstance();
	m_shMaterial.CreateInstance();

	m_kelvinColor.CreateInstance();
	m_useKelvinColor = false;

	m_material->SetHighLevelShaderName( "BoxLight" );
	m_shMaterial->SetHighLevelShaderName( "BoxLight" );

	PrepareResources();

	RebuildVolume();

	m_sharedVB = Tr2ConstGeometry::GetVB( unitCubeVertices, unitCubeVertexCount * sizeof( Vector4 ) );
	m_sharedIB = Tr2ConstGeometry::GetIB( IB_32BIT, unitCubeIndices, sizeof( unitCubeIndices ) / sizeof( unitCubeIndices[0] ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorBoxLight destructor.
// --------------------------------------------------------------------------------------
Tr2InteriorBoxLight::~Tr2InteriorBoxLight()
{
	if( m_projectedTextureRes )
	{
		m_projectedTextureRes->RemoveNotifyTarget( this );
		m_projectedTextureRes.Unlock();
	}

	CCP_STATS_DEC( wodIntLightsAlive );
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from IInitialize interface.  
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InteriorBoxLight::Initialize()
{
	RebuildVolume();

	TransformModified();

	ChooseLightEffect();

	BeResMan->GetResource( m_projectedTexturePath.c_str(), "", BlueInterfaceIID<TriTextureRes>(), (void**)&m_projectedTextureRes );
	if( m_projectedTextureRes )
	{
		m_projectedTextureRes->AddNotifyTarget( this );
	}

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
bool Tr2InteriorBoxLight::OnModified( Be::Var* value )
{
	// Update the regions of influce if the position changes
	if( IsMatch( value, m_transform ) )
	{
		TransformModified();
	}
	else if( IsMatch( value, m_shadowImportance ) )
	{
		// If shadowImportance goes to 0 - destroy the shadow map
		CreateShadowMap();
	}
	else if( IsMatch( value, m_primaryLighting ) )
	{
		CreateShadowMap();
	}
	else if( IsMatch( value, m_shadowResolution ) )
	{
		m_dirtyShadow = true;
		m_emptyShadow = false;
	}
	else if( IsMatch( value, m_projectedTexturePath ) )
	{
		if( m_projectedTextureRes )
		{
			m_projectedTextureRes->RemoveNotifyTarget( this );
			m_projectedTextureRes.Unlock();
		}
		BeResMan->GetResource( m_projectedTexturePath.c_str(), "", BlueInterfaceIID<TriTextureRes>(), (void**)&m_projectedTextureRes );
		if( m_projectedTextureRes )
		{
			m_projectedTextureRes->AddNotifyTarget( this );
		}
		else
		{
			ChooseLightEffect();
		}
	}
	else if( IsMatch( value, m_shadowCasterTypes ) )
	{
		m_dirtyShadow = true;
		m_emptyShadow = false;
		CreateShadowMap();
		ChooseLightEffect();
	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IBlueAsyncResNotifyTarget interface. Re-binds shaders whenever projected
//   texture changes.
// Arguments:
//   p - Resource that changed (projected texture)
// --------------------------------------------------------------------------------------
void Tr2InteriorBoxLight::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_projectedTextureRes )
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
void Tr2InteriorBoxLight::TransformModified()
{
	// Mark the dirty flag
	m_isDirty = true;
	m_dirtyShadow = true;
	m_emptyShadow = false;
	
	Vector3 eye( 0.0f, 0.5f, 0.0f ), at( 0.0f, 0.0f, 0.0f ), up( 0.0f, 0.0f, 1.0f );
	D3DXMatrixLookAtRH( &m_viewMatrix, &eye, &at, &up );
	Matrix transformInv;
	D3DXMatrixInverse( &transformInv, NULL, &m_transform );
	m_viewMatrix = transformInv * m_viewMatrix;
	D3DXMatrixOrthoRH( &m_projectionMatrix, 1.0f, 1.0f, 0.0001f, 1.0f );

	m_collisionCenter = GetPosition();
	m_collisionExtents = GetScaling() * 0.5f;

	D3DXQuaternionRotationMatrix( &m_collisionOrientation, &m_transform );
	D3DXQuaternionNormalize( &m_collisionOrientation, &m_collisionOrientation );

	m_worldBoundingBox = GetBoundingBoxInLocalSpace();
	m_worldBoundingBox.Transform( m_transform );
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Invalidates light's geometry vertex 
//   declaration and shadow maps.
// --------------------------------------------------------------------------------------
void Tr2InteriorBoxLight::ReleaseResources( TriStorage s )
{
	m_lightVertexDecl = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_shadowTexture = NULL;
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Re-creates light's geometry vertex 
//   declaration and shadow map.
// --------------------------------------------------------------------------------------
bool Tr2InteriorBoxLight::OnPrepareResources()
{
	Tr2VertexDefinition vd;
	vd.Add( vd.FLOAT32_4, vd.POSITION );
	
	m_lightVertexDecl = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );

	CreateShadowMap();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Copies the lighting parameters into the per-object data.
// Arguments:
//   lightData - The per-object light data to populate
//   mirrorToWorldMatrix - Mirror to world space transform
// --------------------------------------------------------------------------------------
void Tr2InteriorBoxLight::PopulateLightData( Tr2InteriorPerObjectLightData* lightData, const Matrix &mirrorToWorldMatrix ) const
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

	Vector3 direction( 0.0f, 1.0f, 0.0f );
	D3DXVec3TransformNormal( &lightData->position, &direction, &worldToLight );
	D3DXVec3Normalize( &lightData->position, &lightData->position );
	lightData->radius = -1.0f;

	D3DXMatrixInverse( &worldToLight, NULL, &worldToLight );

	*reinterpret_cast<Vector4*>( &lightData->shadow0Influence ) = *reinterpret_cast<Vector4*>( &worldToLight._11 );
	*reinterpret_cast<Vector4*>( &lightData->spotDirection ) = *reinterpret_cast<Vector4*>( &worldToLight._21 );
	lightData->boxTransformRow3 = *reinterpret_cast<Vector4*>( &worldToLight._31 );
	lightData->boxTransformRow4 = *reinterpret_cast<Vector4*>( &worldToLight._41 );
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the world-space axis aligned bounding box for the light
// --------------------------------------------------------------------------------------
const AxisAlignedBoundingBox& Tr2InteriorBoxLight::GetBoundingBox() const
{
	return m_worldBoundingBox;
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
bool Tr2InteriorBoxLight::TestCellIntersectionAndAdd( Tr2InteriorCell* cell )
{
	// Bail out if the cell is invalid
	if( cell == NULL )
	{
		// No cell, return no intersection
		return false;
	}

	bool intersects = cell->IntersectsOBB( m_collisionCenter, m_collisionExtents, m_collisionOrientation );

	if( intersects )
	{
		cell->AddLight( this );
	}
	else
	{
		// Remove the light from the cell
		cell->RemoveLight( this );
	}

	// Return the result of the intersection test
	return intersects;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the importance of the light, given the current view position.  The view
//   importance is simply the ratio of the light radius (half of box diagonal) and the 
//   distance to the viewer.
// Arguments:
//   viewerPos - The position of the viewpoint
// Return Value:
//   The view importance
// --------------------------------------------------------------------------------------
float Tr2InteriorBoxLight::GetCurrentViewImportance( const Vector3& viewerPos ) const
{
	// importance is based on:
	// 1. dist from camera to box center
	Vector3 position( 0.0f, 0.0f, 0.0f );
	D3DXVec3TransformCoord( &position, &position, &m_transform );

	Vector3 dist = viewerPos - position;
	float distToViewer = D3DXVec3LengthSq( &dist );

	Vector3 minBounds( -0.5f, -0.5f, -0.5f ), maxBounds( 0.5f, 0.5f, 0.5f );
	D3DXVec3TransformCoord( &minBounds, &minBounds, &m_transform );
	D3DXVec3TransformCoord( &maxBounds, &maxBounds, &m_transform );
	Vector3 diagonal = minBounds - maxBounds;

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
float Tr2InteriorBoxLight::GetCurrentShadowImportance( unsigned int shadowMapIndex, const Vector3& viewerPos ) const
{
	return GetCurrentViewImportance( viewerPos ) * m_shadowImportance * m_shadowImportance;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Called whenever light is added to interior scene.
// --------------------------------------------------------------------------------------
void Tr2InteriorBoxLight::AddToScene( void )
{
	// Set the dirty flag
	m_isDirty = true;
	m_dirtyShadow = true;
	m_emptyShadow = false;
}


// --------------------------------------------------------------------------------------
// Description:
//   Called whenever light is removed from interior scene.
// --------------------------------------------------------------------------------------
void Tr2InteriorBoxLight::RemoveFromScene( void )
{
	m_shadowTexture = NULL;
}

// -------------------------------------------------------------
// Description:
//   Returns rotation component of light's transform.
// Return Value:
//   Rotation component of light's transform
// -------------------------------------------------------------
Quaternion Tr2InteriorBoxLight::GetRotation( void ) const
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	

	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );

	return tmpRotation;
}

// -------------------------------------------------------------
// Description:
//   Sets rotation component of light's transform.
// Arguments:
//   rotQuat - Rotation component of light's transform
// -------------------------------------------------------------
void Tr2InteriorBoxLight::SetRotation( const Quaternion& rotQuat )
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	

	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );
	D3DXMatrixTransformation( &m_transform, NULL, NULL, &tmpScale, NULL, &rotQuat, &tmpTranslation );

	TransformModified();
}

// -------------------------------------------------------------
// Description:
//   Returns scaling component of light's transform.
// Return Value:
//   Scaling component of light's transform
// -------------------------------------------------------------
Vector3 Tr2InteriorBoxLight::GetScaling( void ) const
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	
	
	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );

	return tmpScale;
}

// -------------------------------------------------------------
// Description:
//   Sets scaling component of light's transform.
// Arguments:
//   scaleVec - Scaling component of light's transform
// -------------------------------------------------------------
void Tr2InteriorBoxLight::SetScaling( const Vector3& scaleVec )
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	

	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );
	D3DXMatrixTransformation( &m_transform, NULL, NULL, &scaleVec, NULL, &tmpRotation, &tmpTranslation );

	TransformModified();
}

// -------------------------------------------------------------
// Description:
//   Per-frame update method. Updates curve sets.
// Arguments:
//   time - Current system time.
// -------------------------------------------------------------
void Tr2InteriorBoxLight::Update( Be::Time time )
{
	for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		( *it )->Update( TimeAsDouble( time ) );
	}
}

// -------------------------------------------------------------
// Description:
//   Sets translation component of light's transform.
// Arguments:
//   pos - Translation component of light's transform
// -------------------------------------------------------------
void Tr2InteriorBoxLight::SetPosition( const Vector3& pos )
{
	m_transform._41 = pos.x;
	m_transform._42 = pos.y;
	m_transform._43 = pos.z;

	TransformModified();
}

// --------------------------------------------------------------------------------------
// Description:
//   Per-object data for light rendering during light accumulation pass in pre-pass scenes.
//   Sets a shadow map texture and projection texture.
// --------------------------------------------------------------------------------------
class Tr2InteriorBoxLight::PerLightData : public Tr2ShadowPerLightData<Tr2ProjectedPerLightData>
{
};

// --------------------------------------------------------------------------------------
// Description:
//   Adds light's geometry to a render batch accumulator. Used during light accumulation 
//   pass.
// Arguments:
//   batches - Render batch accumulator to add batch to
//   mirrorToWorldMatrix - Mirror to world space transform.
// --------------------------------------------------------------------------------------
void Tr2InteriorBoxLight::GetBatches( ITriRenderBatchAccumulator* batches, const Matrix& mirrorToWorldMatrix )
{
	PerLightData* data = batches->Allocate<PerLightData>(); 

	if( !data )
	{
		return;
	}

	data->SetShadowTexture( m_shadowTexture );
	data->SetProjectedTexture( m_projectedTextureRes );

	Matrix mirrorMatrix;
	D3DXMatrixTranspose( &mirrorMatrix, &mirrorToWorldMatrix );
	D3DXMatrixInverse( &mirrorMatrix, NULL, &mirrorMatrix );

	Tr2InteriorPerLightPSData perObjectPSBuffer;

	PopulateLightData( &perObjectPSBuffer.lightData, mirrorToWorldMatrix );
	perObjectPSBuffer.additionalParameters.x = m_specularIntensity;

	if( m_shadowTexture )
	{
		perObjectPSBuffer.shadowRect[0].x = float( m_shadowTexture->GetX() + 0.5f ) / m_shadowTexture->GetTextureWidth();
		perObjectPSBuffer.shadowRect[0].y = float( m_shadowTexture->GetY() + 0.5f ) / m_shadowTexture->GetTextureHeight();
		perObjectPSBuffer.shadowRect[0].z = float( m_shadowTexture->GetWidth() - 1.0f ) / m_shadowTexture->GetTextureWidth();
		perObjectPSBuffer.shadowRect[0].w = float( m_shadowTexture->GetHeight() - 1.0f ) / m_shadowTexture->GetTextureHeight();
	}
	else
	{
		perObjectPSBuffer.shadowRect[0] = Vector4( 0.0f, 0.0f, 1.0f, 1.0f );
	}

	perObjectPSBuffer.mirrorToWorldMatrix = mirrorToWorldMatrix;
	
	{
		Matrix texAdjust( 0.5f, 0.0f, 0.0f, 0.0f,
						  0.0f, -0.5f, 0.0f, 0.0f,
						  0.0f, 0.0f, 1.0f, 0.0f,
						  0.5f, 0.5f, 0.0f, 1.0f );
		D3DXMatrixTranspose( &perObjectPSBuffer.mirrorToWorldMatrix, &perObjectPSBuffer.mirrorToWorldMatrix );
		perObjectPSBuffer.mirrorToWorldMatrix = perObjectPSBuffer.mirrorToWorldMatrix * GetViewMatrix( 0 ) * m_projectionMatrix * texAdjust;
		D3DXMatrixTranspose( &perObjectPSBuffer.mirrorToWorldMatrix, &perObjectPSBuffer.mirrorToWorldMatrix );
	}

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	const Vector4* vertexes;
	unsigned vertexCount;
	Matrix worldView, worldViewTranspose, worldViewProj;
	Tr2VertexBufferAL* vb;
	Tr2IndexBufferAL* ib;

	vertexes = unitCubeVertices;
	vertexCount = unitCubeVertexCount;
	worldView = m_transform * mirrorMatrix * Tr2Renderer::GetViewTransform();
	vb = m_sharedVB;
	ib = m_sharedIB;

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
//   required for this light source.
// Return Value:
//   1 if boxlight requires shadows and 0 otherwise
// -------------------------------------------------------------
unsigned int Tr2InteriorBoxLight::GetRequiredShadowMapCount() const
{
	if( m_primaryLighting && m_shadowImportance > 0.0f && m_shadowCasterTypes != ST_NONE )
	{
		return  1;
	}
	else
	{
		return 0;
	}
}

// -------------------------------------------------------------
// Description:
//   Returns dimensions of shadow map required for this light.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   maximumResolution - If true always return light maximum 
//						 shadow resolution, otherwise can apply 
//						 LOD scheme.
//   width (out) - Width of shadow map
//   height (out) - Height of shadow map
// -------------------------------------------------------------
void Tr2InteriorBoxLight::GetRequiredShadowMapResolution( unsigned int index, bool maximumResolution, unsigned int& width, unsigned int& height ) const
{
	if( maximumResolution )
	{
		width = height = m_shadowResolution;
	}
	else
	{
		width = height = m_currentShadowResolution;
	}
}

// -------------------------------------------------------------
// Description:
//   Calculates the fraction of camera field of view occupied by
//   a set of points.
// Arguments:
//   vertices - Array of points
//   world - Local to world space transformation
// Return Value:
//   Fraction of camera field of view occupied by points
// -------------------------------------------------------------
template<unsigned int count>
float GetScreenSize( const Vector4* vertices, const Matrix& world )
{
	if( Tr2Renderer::GetFieldOfView() <= 0.0f )
	{
		return 0.0f;
	}

	Vector3 directions[count];
	D3DXVec3TransformCoordArray( directions, sizeof( Vector3 ), reinterpret_cast<const Vector3*>( vertices ), sizeof( Vector4 ), &world, count );
	for( unsigned int i = 0; i < count; ++i )
	{
		directions[i] -= Tr2Renderer::GetViewPosition();
		D3DXVec3Normalize( directions + i, directions + i );
	}
	float cosMaxAngle = 1.f;
	for( unsigned int i = 0; i < count; ++i )
	{
		for( unsigned int j = i + 1; j < count; ++j )
		{
			float cosAngle = D3DXVec3Dot( directions + i, directions + j );
			if( cosAngle < cosMaxAngle )
			{
				cosMaxAngle = cosAngle;
			}
		}
	}
	cosMaxAngle = min( max( cosMaxAngle, -1.f ), 1.f );
	return acos( cosMaxAngle ) / Tr2Renderer::GetFieldOfView();
}

// -------------------------------------------------------------
// Description:
//   Caches shadow map view-dependent resolution values for a 
//   light source. Is called every frame before any calls to 
//   GetRequiredShadowMapResolution.
// -------------------------------------------------------------
void Tr2InteriorBoxLight::CacheShadowMapResolution()
{
	if( !m_enableShadowLOD )
	{
		m_currentShadowResolution = m_shadowResolution;
		return;
	}

	float size = ::GetScreenSize<unitCubeVertexCount>( unitCubeVertices, m_transform );

	// Assume the requested (maximum) shadow map size is for 3/4 of screen
	// width/height occupied by light
	const float idealSize = 0.85f;
	// Minimal shadow map size: we drop shadow maps that are less than that
	const unsigned int minSize = 16;

	m_currentShadowResolution = m_shadowResolution;

	size /= idealSize;

#ifdef WIN32
    if( !_finite( size ) )
    {
        size = -1;
    }
#else
    if( !finite( size ) )
    {
        size = -1;
    }
#endif
	if( size <= 0 )
	{
		m_currentShadowResolution = 0;
		return;
	}
	if( size > 1.f )
	{
		return;
	}

	int factor = int( pow( 2.f, ceil( -log( size ) / log( 2.f ) ) ) + 0.5f );

	m_currentShadowResolution /= factor;

	if( m_currentShadowResolution < minSize )
	{
		m_currentShadowResolution = 0;
	}
}

// -------------------------------------------------------------
// Description:
//   Returns atlas texture containing the given shadow map. Can
//   return NULL if the texture is not required of was not yet
//   assigned.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Atlas texture containing the shadow map
// -------------------------------------------------------------
Tr2AtlasTexture* Tr2InteriorBoxLight::GetShadowAtlasTexture( unsigned int index ) const
{
	return m_shadowTexture;
}

// -------------------------------------------------------------
// Description:
//   Assigns atlas texture for the given shadow map. 
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   texture - Atlas texture containing the shadow map
// -------------------------------------------------------------
void Tr2InteriorBoxLight::SetShadowAtlasTexture( unsigned int index, Tr2AtlasTexture* texture )
{
	bool rebind = ( m_shadowTexture == NULL ) != ( texture == NULL );
	m_shadowTexture = texture;
	if( rebind )
	{
		ChooseLightEffect();
	}
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
void Tr2InteriorBoxLight::SetEmptyShadow( unsigned int shadowMapIndex, bool emptyShadow )
{
	m_emptyShadow = emptyShadow;
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
bool Tr2InteriorBoxLight::IsShadowEmpty( unsigned int shadowMapIndex ) const
{
	return m_emptyShadow;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets up shadow map render target and fills per-frame data provided. Called before
//	 rendering into spotlight shadow map. 
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   vsData - per-frame vertex shader data
//   psData - per-frame pixel shader data
// Return Value:
//   true, if shadow map was successfully set and the scene can render into shadow map
//   false, otherwise
// --------------------------------------------------------------------------------------
bool Tr2InteriorBoxLight::BeginShadowUpdate( unsigned int shadowMapIndex, Tr2PerFrameVSData* vsData, Tr2PerFrameShadowPSData* psData, Tr2RenderContext& renderContext )
{
	D3DPERF_EVENT( L"Tr2InteriorBoxLight::BeginShadowRendering" );

	Tr2Renderer::SetViewTransform( m_viewMatrix );
	Tr2Renderer::SetProjectionTransform( m_projectionMatrix );

	// pre-mults
	Matrix viewProjMat = m_viewMatrix * m_projectionMatrix;

	// column_major for shaders
	D3DXMatrixTranspose( &vsData->ViewMat, &m_viewMatrix );
	D3DXMatrixTranspose( &vsData->ProjectionMat, &m_projectionMatrix );
	D3DXMatrixTranspose( &vsData->ViewProjectionMat, &viewProjMat );
	D3DXMatrixInverse( &vsData->ViewInverseTransposeMat, NULL, &m_viewMatrix );

	vsData->sunDirWorld.x = 1.0f;		// light type: orthographic
	psData->lightRadius = -1.f;

	CR( renderContext.Clear( CLEARFLAGS_TARGET | CLEARFLAGS_ZBUFFER | CLEARFLAGS_STENCIL, 0x00ffff00, 1, 0 ) );

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Restores render target previously set in a call to BeginShadowUpdate. Does nothing.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// --------------------------------------------------------------------------------------
void Tr2InteriorBoxLight::EndShadowUpdate( unsigned int shadowMapIndex )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns spotlight view matrix.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Spotlight view matrix
// --------------------------------------------------------------------------------------
const Matrix& Tr2InteriorBoxLight::GetViewMatrix( unsigned int shadowMapIndex ) const
{
	return m_viewMatrix;
}

// -------------------------------------------------------------
// Description:
//   Returns projection transform matrix for shadow map.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Shadow map view to projection space transform.
// -------------------------------------------------------------
const Matrix& Tr2InteriorBoxLight::GetProjectionMatrix( unsigned int shadowMapIndex ) const
{
	return m_projectionMatrix;
}

// -------------------------------------------------------------
// Description:
//   Marks light's shadow maps that are affected by given bounding
//   box as dirty. The bounding box usually represents moved dynamic
//   object.
// Arguments:
//   minBounds - Min bounds of a box in world space
//   maxBounds - Max bounds of a box in world space
// -------------------------------------------------------------
void Tr2InteriorBoxLight::MarkShadowsDirtyForBounds( const Vector3 &minBounds, const Vector3 &maxBounds )
{
	if( IntersectOrientedBoxAxisAlignedBox(
											m_collisionCenter, 
											m_collisionExtents, 
											m_collisionOrientation, 
											minBounds,
											maxBounds ) )
	{
		MarkShadowDirty( 0, true );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets spotlight shadow dirty flag. Dirty spotlight shadows are to be updated during
//   subsequent scene render.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   dirty - spotlight shadow dirty flag
// --------------------------------------------------------------------------------------
void Tr2InteriorBoxLight::MarkShadowDirty(  unsigned int shadowMapIndex, bool dirty )
{
	m_dirtyShadow = dirty;
	if( dirty )
	{
		m_emptyShadow = false;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a flag indicating if the spotlight shadow is dirty.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   true if the spotlight shadow is dirty and needs updating
//   false if the spotlight shadow is up to date
// --------------------------------------------------------------------------------------
bool Tr2InteriorBoxLight::IsShadowDirty( unsigned int shadowMapIndex ) const
{
	return m_dirtyShadow;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns type of objects rendered into the shadow map.
// --------------------------------------------------------------------------------------
Tr2InteriorBoxLight::ShadowCasterTypes Tr2InteriorBoxLight::GetShadowCasterTypes() const
{
	return m_shadowCasterTypes;
}

void Tr2InteriorBoxLight::RebuildVolume( void )
{
	m_collisionCenter = GetPosition();
	m_collisionExtents = GetScaling() * 0.5f;

	D3DXQuaternionRotationMatrix( &m_collisionOrientation, &m_transform );
	D3DXQuaternionNormalize( &m_collisionOrientation, &m_collisionOrientation );

	// Set the dirty flag
	m_isDirty = true;
	m_dirtyShadow = true;
	m_emptyShadow = false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Chooses an appropriate effect for light rendering during light accumulation pass.
//   Accounts for if there is an assigned shadow map and projected texture.
// --------------------------------------------------------------------------------------
void Tr2InteriorBoxLight::ChooseLightEffect()
{
	Tr2ShaderSituation situation;
	if( m_shadowTexture && m_shadowCasterTypes != ST_NONE )
	{
		situation.AddSituationString( "Shadow" );
	}
	if( m_projectedTextureRes && m_projectedTextureRes->IsGood() )
	{
		situation.AddSituationString( "ProjectedTexture" );
	}

	m_material->BindLowLevelShader( situation );

	situation.AddSituationString( "GenerateSHCoefficients" );
	m_shMaterial->BindLowLevelShader( situation );
}

// --------------------------------------------------------------------------------------
// Description:
//   Releases shadow texture or marks shadow as dirty.
// --------------------------------------------------------------------------------------
void Tr2InteriorBoxLight::CreateShadowMap()
{
	if( m_primaryLighting && m_shadowImportance > 0.0f && m_shadowCasterTypes != ST_NONE )
	{
		if( m_shadowTexture == NULL )
		{
			m_dirtyShadow = true;
			m_emptyShadow = false;
		}
	}
	else
	{
		m_shadowTexture = NULL;
	}
	ChooseLightEffect();
}

// -------------------------------------------------------------
// Description:
//   Get batches for a light to use during SH lighting step.
// Arguments:
//   batches - Batch accumulator to add batches to.
// -------------------------------------------------------------
void Tr2InteriorBoxLight::GetSHBatches( ITriRenderBatchAccumulator* batches ) const
{
	if( !m_primaryLighting )
	{
		return;
	}

	PerLightData* data = batches->Allocate<PerLightData>();

	if( !data )
	{
		return;
	}

	data->SetShadowTexture( m_shadowTexture );
	data->SetProjectedTexture( m_projectedTextureRes );

	Tr2InteriorPerLightPSData perObjectPSBuffer;

	PopulateLightData( &perObjectPSBuffer.lightData, Tr2Renderer::GetIdentityTransform() );
	perObjectPSBuffer.additionalParameters.x = m_specularIntensity;

	if( m_shadowTexture )
	{
		perObjectPSBuffer.shadowRect[0].x = float( m_shadowTexture->GetX() + 0.5f ) / m_shadowTexture->GetTextureWidth();
		perObjectPSBuffer.shadowRect[0].y = float( m_shadowTexture->GetY() + 0.5f ) / m_shadowTexture->GetTextureHeight();
		perObjectPSBuffer.shadowRect[0].z = float( m_shadowTexture->GetWidth() - 1.0f ) / m_shadowTexture->GetTextureWidth();
		perObjectPSBuffer.shadowRect[0].w = float( m_shadowTexture->GetHeight() - 1.0f ) / m_shadowTexture->GetTextureHeight();
	}
	else
	{
		perObjectPSBuffer.shadowRect[0] = Vector4( 0.0f, 0.0f, 1.0f, 1.0f );
	}

	{
		Matrix texAdjust( 0.5f, 0.0f, 0.0f, 0.0f,
						  0.0f, -0.5f, 0.0f, 0.0f,
						  0.0f, 0.0f, 1.0f, 0.0f,
						  0.5f, 0.5f, 0.0f, 1.0f );
		perObjectPSBuffer.mirrorToWorldMatrix = GetViewMatrix( 0 ) * m_projectionMatrix * texAdjust;
		D3DXMatrixTranspose( &perObjectPSBuffer.mirrorToWorldMatrix, &perObjectPSBuffer.mirrorToWorldMatrix );
	}

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
//   Blue-exposed function that returns AABB for box light in its
//   local coordinate space.
// Return value:
//   AABB for box light in its local coordinate space
// -------------------------------------------------------------
AxisAlignedBoundingBox Tr2InteriorBoxLight::GetBoundingBoxInLocalSpace() const
{
	return AxisAlignedBoundingBox( 
		Vector3( -0.5f, -0.5f, -0.5f ), 
		Vector3( 0.5f, 0.5f, 0.5f ) );
}

bool Tr2InteriorBoxLight::IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const
{
	if( !m_primaryLighting )
	{
		return false;
	}
	objectToWorld = m_transform;
	return frustum.IsBoxVisible( m_worldBoundingBox.m_min, m_worldBoundingBox.m_max );
}

