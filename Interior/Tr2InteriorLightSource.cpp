#include "StdAfx.h"

#include "Tr2InteriorLightSource.h"
#include "Tr2InteriorConstantBufferFormats.h"
#include "TriDebugResourceHelper.h"

#include "Utilities/BoundingSphere.h"
#include "Tr2AtlasTexture.h"
#include "Tr2InteriorCell.h"
#include "Tr2InteriorLightGeometryRenderBatch.h"
#include "Tr2KelvinColor.h"
#include "Tr2ShaderMaterial.h"
#include "Tr2ConstGeometry.h"
#include "Curves/TriCurveSet.h"
#include "Include/TriMath.h"
#include "Tr2Effect.h"
#include "TriViewport.h"

CCP_STATS_DECLARE( wodIntLightsAlive, "Trinity/Tr2IntLightsAlive", false, CST_COUNTER_LOW, 
				  "Count of Tr2InteriorLightSources alive" );

BLUE_DEFINE_INTERFACE( ITr2InteriorLight );

using namespace Tr2RenderContextEnum;
	
namespace {
    unsigned int const unitIcosahedronIndices[] =
    {
        0,  8,  4,  
        0,  5,  10,  
        2,  4,  9,  
        2,  11, 5, 
        1,  6,  8,  
        1,  10, 7, 
        3,  9,  6,  
        3,  7,  11,  
        0,  10, 8, 
        1,  8,  10,  
        2,  9,  11,  
        3,  11, 9, 
        4,  2,  0,  
        5,  0,  2,  
        6,  1,  3,  
        7,  3,  1,  
        8,  6,  4,  
        9,  4,  6,  
        10, 5,  7,  
        11, 7,  5,  
    };
    Vector4 const unitIcosahedronVertices[] =
    {
        Vector4( 1.051483f,   0.552797f,  0.341647f, 1.0f ),
        Vector4( -1.051483f,  0.552797f,  0.341647f, 1.0f ),
        Vector4( 1.051483f,  -0.552797f, -0.341647f, 1.0f ),
        Vector4( -1.051483f, -0.552797f, -0.341647f, 1.0f ),
        Vector4( 0.649852f,  -0.552797f,  0.894445f, 1.0f ),
        Vector4( 0.649852f,   0.552797f, -0.894445f, 1.0f ),
        Vector4( -0.649852f, -0.552797f,  0.894445f, 1.0f ),
        Vector4( -0.649852f,  0.552797f, -0.894445f, 1.0f ),
        Vector4( 0.000000f,   0.552797f,  1.105594f, 1.0f ),
        Vector4( 0.000000f,  -1.236092f,  0.000000f, 1.0f ),
        Vector4( 0.000000f,   1.236092f, -0.000000f, 1.0f ),
        Vector4( 0.000000f,  -0.552797f, -1.105594f, 1.0f ),
    };

	static const unsigned int unitIcosahedronVertexCount = sizeof( unitIcosahedronVertices ) / sizeof( Vector4 );
	static const unsigned int unitIcosahedronTriangleCount = sizeof( unitIcosahedronIndices ) / sizeof( unsigned int ) / 3;

	Vector4 const unitConeVertices[] =
	{
		Vector4( 0.0f,	 0.0f,		 0.0f, 1.0f ),
		Vector4( 0.0f,	 1.1547006f, 1.0f, 1.0f ),
		Vector4( 1.0f,	 0.5773503f, 1.0f, 1.0f ),
		Vector4( 1.0f,	-0.5773503f, 1.0f, 1.0f ),
		Vector4( 0.0f,	-1.1547006f, 1.0f, 1.0f ),
		Vector4( -1.0f,	-0.5773503f, 1.0f, 1.0f ),
		Vector4( -1.0f,	 0.5773503f, 1.0f, 1.0f ),
	};
    unsigned int const unitConeIndices[] =
    {
		0,	1,	2,
		0,	2,	3,
		0,	3,	4,
		0,	4,	5,
		0,	5,	6,
		0,	6,	1,
			  
		1,	3,	2,
		1,	4,	3,
		1,	5,	4,
		1,	6,	5,
    };
    unsigned int const unitConeWireIndices[] =
    {
		0,	1,
		0,	2,
		0,	3,
		0,	4,
		0,	5,
		0,	6,

		1,	2,
		2,	3,
		3,	4,
		4,	5,
		5,	6,
		6,	1,
    };

	static const unsigned int unitConeVertexCount = sizeof( unitConeVertices ) / sizeof( Vector4 );
	static const unsigned int unitConeTriangleCount = sizeof( unitConeIndices ) / sizeof( unsigned int ) / 3;
	static const unsigned int unitConeWireLineCount = sizeof( unitConeWireIndices ) / sizeof( unsigned int ) / 2;

	// share this so the other light types don't have to copy paste this.
	struct DebugRenderEffectCallback : public IRenderCallback
	{
		TriDebugResourceHelper::VertexPosColor *polygon;
		unsigned int vertexCount;
		unsigned int triangleCount;
		const unsigned int *indices;
		uint32_t zFunc;
		uint32_t fillMode;
		Tr2RenderContextEnum::Topology	primitiveType;

		void SubmitGeometry( Tr2RenderContext& renderContext )
		{
			renderContext.SetRenderState( RS_ZENABLE, TRUE );
			renderContext.SetRenderState( RS_ZWRITEENABLE, FALSE );
			renderContext.SetRenderState( RS_ZFUNC, zFunc );
			renderContext.SetRenderState( RS_FILLMODE, fillMode );
			uint32_t stride = sizeof( TriDebugResourceHelper::VertexPosColor );
			renderContext.SetTopology( primitiveType );
			if( indices )
			{
				renderContext.DrawIndexedPrimitiveUP( vertexCount, triangleCount, indices, polygon, stride );
			}
			else
			{
				renderContext.DrawPrimitiveUP( triangleCount, polygon, stride );
			}
		}
	};

} // anonymous

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorLightSource default constructor
// --------------------------------------------------------------------------------------
Tr2InteriorLightSource::Tr2InteriorLightSource( IRoot* lockobj ) :
	m_name(),
	m_position( 0.f, 0.f, 0.f ),
	m_radius( 1.f ),
	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_falloff( 1.f ),
	m_specularIntensity( 1.f ),
	m_shadowImportance( 1.f ),
	m_coneAlphaOuter( 180.f ),
	m_coneAlphaInner( 180.f ),
	m_coneDirection( 0.f, -1.f, 0.f ),
	m_primaryLighting( true ),
	m_affectTransparentObjects( true ),
	m_importanceScale( 1.0f ),
	m_importanceBias( 0.0f ),
	m_isDirty( true ),
	m_lightVertexDecl( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_shadowResolution( 256 ),
	m_enableShadowLOD( true ),
	m_shadowCasterTypes( ST_ALL ),
	PARENTLOCK( m_curveSets ),
	m_sharedVB( nullptr ),
	m_sharedIB( nullptr )
{
	CCP_STATS_INC( wodIntLightsAlive );
	RebuildVolume();

	for( int i = 0; i < 6; ++i )
	{
		m_emptyShadow[i] = false;
		m_dirtySpotLightShadow[i] = true;
		m_framesSinceShadowUpdate[0] = 0;
	}

	D3DXMatrixIdentity( &m_unitToWorldTransform );
	m_worldBoundingBox = AxisAlignedBoundingBox( Vector3( -1.f, -1.f, -1.f ), Vector3( 1.f, 1.f, 1.f ) );

	m_material.CreateInstance();
	m_shMaterial.CreateInstance();

	m_kelvinColor.CreateInstance();
	m_useKelvinColor = false;

	m_material->SetHighLevelShaderName( "PointLight" );
	m_shMaterial->SetHighLevelShaderName( "PointLight" );
	PrepareResources();
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorLightSource destructor.
// --------------------------------------------------------------------------------------
Tr2InteriorLightSource::~Tr2InteriorLightSource()
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
bool Tr2InteriorLightSource::Initialize()
{
	RebuildVolume();
	
	UpdateInternalMaterials();
	ChooseLightEffect();
	PrepareResources();

	RecalculateUnitToWorldMatrix();

	BeResMan->GetResource( m_projectedTexturePath.c_str(), "", m_projectedTextureRes );
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
//   changes generated in Python.  If the light position changes, the regions of influence 
//   are updated with the new transform matrix & the light is flagged as 'dirty', forcing 
//   a new round of light-cell intersection tests on the next scene Update. The light is 
//   flagged as dirty.
// Arguments:
//   value - The Blue-exposed parameter that changed
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InteriorLightSource::OnModified( Be::Var* value )
{
	// Update the regions of influce if the position changes
	if( IsMatch( value, m_position ) )
	{
		m_worldBoundingBox = AxisAlignedBoundingBox( m_position - Vector3( m_radius, m_radius, m_radius ), m_position + Vector3( m_radius, m_radius, m_radius ) );

		RecalculateUnitToWorldMatrix();
		RebuildGeometry();

		// Mark the dirty flag
		m_isDirty = true;
		for( int i = 0; i < 6; ++i )
		{
			m_emptyShadow[i] = false;
			m_dirtySpotLightShadow[i] = true;
		}
	}
	else if( IsMatch( value, m_radius )			||
			 IsMatch( value, m_coneAlphaOuter ) ||
			 IsMatch( value, m_coneDirection ) )
	{
		m_worldBoundingBox = AxisAlignedBoundingBox( m_position - Vector3( m_radius, m_radius, m_radius ), m_position + Vector3( m_radius, m_radius, m_radius ) );

		// Rebuild the bounding volume
		RebuildVolume();

		RecalculateUnitToWorldMatrix();

		// Mark the dirty flag
		m_isDirty = true;
		for( int i = 0; i < 6; ++i )
		{
			m_emptyShadow[i] = false;
			m_dirtySpotLightShadow[i] = true;
		}

		RebuildGeometry();
		ChooseLightEffect();
		PrepareResources();
	}
	else if( IsMatch( value, m_shadowImportance ) )
	{
		// If shadowImportance goes to 0 - destroy the shadow map
		PrepareResources();
	}
	else if( IsMatch( value, m_shadowResolution ) )
	{
		// Have to destroy shadow map here so that the scene will
		// allocate a new one
		for( int i = 0; i < 6; ++i )
		{
			m_shadowAtlasTexture[i] = NULL;
		}
		PrepareResources();
		UpdateSpotlightFrustum();
	}
	else if( IsMatch( value, m_primaryLighting ) )
	{
		PrepareResources();
	}
	else if( IsMatch( value, m_projectedTexturePath ) )
	{
		if( m_projectedTextureRes )
		{
			m_projectedTextureRes->RemoveNotifyTarget( this );
			m_projectedTextureRes.Unlock();
		}
		BeResMan->GetResource( m_projectedTexturePath.c_str(), "", m_projectedTextureRes );
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
		for( int i = 0; i < 6; ++i )
		{
			m_emptyShadow[i] = false;
			m_dirtySpotLightShadow[i] = true;
		}
		PrepareResources();
	}
	else if( IsMatch( value, m_customMaterial ) )
	{
		if( m_customMaterial )
		{
			m_material = m_customMaterial;
			UpdateInternalMaterials();
		}
		else
		{
			m_material = NULL;
			m_shMaterial = NULL;

			m_material.CreateInstance();
			m_shMaterial.CreateInstance();

			ChooseLightEffect();
		}
		RebuildGeometry();
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
void Tr2InteriorLightSource::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_projectedTextureRes )
	{
		ChooseLightEffect();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates internal shader materials (for SH lighting and lightmap generation) after
//   custom material was changed.
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::UpdateInternalMaterials()
{
	if( m_customMaterial )
	{
		m_shMaterial = NULL;

		BeClasses->CloneTo( m_customMaterial->GetRawRoot(), (IRoot**)&m_shMaterial );
		ChooseLightEffect();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Recalculates cached unit to world light geometry transform for light accumulation 
//   pass
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::RecalculateUnitToWorldMatrix()
{
	if( IsSpotLight() )
	{
		// Orient unit cone so that its Z axis is aligned with light's cone direction
		Matrix orientation;
		D3DXMatrixIdentity( &orientation );
		D3DXVec3Normalize( &orientation.GetZ(), &m_coneDirection );
		Vector3 y( 0.0f, 1.0f, 0.0f );
		if( abs( D3DXVec3Dot( &orientation.GetZ(), &y ) ) > 0.9f )
		{
			y = Vector3( 1.0f, 0.0f, 0.0f );
		}
		D3DXVec3Normalize( &orientation.GetX(), D3DXVec3Cross( &orientation.GetX(), &orientation.GetZ(), &y ) );
		D3DXVec3Cross( &orientation.GetY(), &orientation.GetZ(), &orientation.GetX() );

		// Determine the radius of the cone
		float halfAngle = m_coneAlphaOuter / 180.f * XM_PI;
		float sideLength = 1.0f / cos( halfAngle );
		float baseRadius = sideLength * sin( halfAngle );

		Matrix scale, translation, worldViewProj;
		D3DXMatrixScaling( &scale, baseRadius * m_radius, baseRadius * m_radius, m_radius );
		D3DXMatrixTranslation( &translation, m_position.x, m_position.y, m_position.z );
		m_unitToWorldTransform = scale * orientation * translation;

		// Recalculate shadow view/projection matrices
		Vector3 eye = m_position;
		Vector3 at = m_position + m_coneDirection;
		Vector3 up;
		if( abs( m_coneDirection.y ) < 0.99f )
		{
			Vector3 y( 0, 1, 0 );
			D3DXVec3Cross( &up, &m_coneDirection, &y );
			D3DXVec3Cross( &up, &up, &m_coneDirection );
		}
		else
		{
			Vector3 x( 0, 0, 1 );
			D3DXVec3Cross( &up, &m_coneDirection, &x );
		}
		D3DXMatrixLookAtRH( &m_viewMatrix[0], &eye, &at, &up );
		D3DXMatrixPerspectiveFovRH( &m_projectionMatrix, m_coneAlphaOuter * 2 / 180.f * XM_PI, 1.f, 0.01f, m_radius );
		UpdateSpotlightFrustum();

	}
	else
	{
		Matrix scale, translation, worldViewProj;
		D3DXMatrixScaling( &scale, m_radius, m_radius, m_radius );
		D3DXMatrixTranslation( &translation, m_position.x, m_position.y, m_position.z );
		m_unitToWorldTransform = scale * translation;

		Matrix projectionMatrix;
		Vector3 at, up, eye = m_position;
		// neg x
		at = eye + Vector3( -1.f, 0.f, 0.f );
		up = Vector3( 0.f, 1.f, 0.f );
		D3DXMatrixLookAtRH( &m_viewMatrix[0], &eye, &at, &up );
		// pos x
		at = eye + Vector3( 1.f, 0.f, 0.f );
		up = Vector3( 0.f, 1.f, 0.f );
		D3DXMatrixLookAtRH( &m_viewMatrix[1], &eye, &at, &up );
		// pos y
		at = eye + Vector3( 0.f, 1.f, 0.f );
		up = Vector3( 0.f, 0.f, -1.f );
		D3DXMatrixLookAtRH( &m_viewMatrix[2], &eye, &at, &up );
		// neg y
		at = eye + Vector3( 0.f, -1.f, 0.f );
		up = Vector3( 0.f, 0.f, 1.f );
		D3DXMatrixLookAtRH( &m_viewMatrix[3], &eye, &at, &up );
		// pos z
		at = eye + Vector3( 0.f, 0.f, 1.f );
		up = Vector3( 0.f, 1.f, 0.f );
		D3DXMatrixLookAtRH( &m_viewMatrix[4], &eye, &at, &up );
		// neg z
		at = eye + Vector3( 0.f, 0.f, -1.f );
		up = Vector3( 0.f, 1.f, 0.f );
		D3DXMatrixLookAtRH( &m_viewMatrix[5], &eye, &at, &up );

		D3DXMatrixPerspectiveFovRH( &m_projectionMatrix, XM_PI / 2.f, 1.f, 0.01f, m_radius );
		UpdateSpotlightFrustum();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Invalidates light's geometry vertex 
//   declaration.
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::ReleaseResources( TriStorage s )
{
	m_lightVertexDecl = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	for( int i = 0; i < 6; ++i )
	{
		m_shadowAtlasTexture[i] = NULL;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Re-creates light's geometry vertex 
//   declaration.
// --------------------------------------------------------------------------------------
bool Tr2InteriorLightSource::OnPrepareResources()
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
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::PopulateLightData( Tr2InteriorPerObjectLightData* lightData, 
											   const Matrix& mirrorToWorldMatrix ) const
{
	Vector3 direction( XMVector3TransformNormal( m_coneDirection, mirrorToWorldMatrix ) );

	XMVECTOR det;
	XMMATRIX invTransMirrorMat = XMMatrixInverse( &det, XMMatrixTranspose( mirrorToWorldMatrix ) );
	Vector3 position( XMVector3TransformCoord( m_position, invTransMirrorMat ) );

	// just put this in struct
	lightData->position = position;
	lightData->radius = std::max( m_radius, 0.0f ); // when radius<0 the light is treated as box light in forward rendering
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
	lightData->shadow0Influence = 0.f;
	lightData->shadow1Influence = 0.f;

	// Spot light values (always populate them, they are basicly pointlights with 
	// some non-default values...).  First apply some limits
	float innerAngle = m_coneAlphaInner;
	float outerAngle = m_coneAlphaOuter;
	if( innerAngle + 1.f > outerAngle )
	{
		innerAngle = outerAngle - 1.f;
	}
	// if this is not a spotlight, force full 360degrees sphere
	if( !IsSpotLight() )
	{
		outerAngle = innerAngle = 360.f;
	}

	lightData->coneCosAlphaOuter = cosf( XMConvertToRadians( outerAngle ) );
	lightData->coneCosAlphaInner = cosf( XMConvertToRadians( innerAngle ) );
	lightData->spotDirection = XMVector3Normalize( direction );
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the world-space axis aligned bounding box for the light
// --------------------------------------------------------------------------------------
const AxisAlignedBoundingBox& Tr2InteriorLightSource::GetBoundingBox() const
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
bool Tr2InteriorLightSource::TestCellIntersectionAndAdd( Tr2InteriorCell* cell )
{
	// Bail out if the cell is invalid
	if( cell == NULL )
	{
		// No cell, return no intersection
		return false;
	}

	bool intersects = cell->IsUnbounded();

	if( !intersects )
	{
		if( !IsSpotLight() )
		{
			intersects = cell->IntersectsSphere( m_position, m_radius );
		}
		else
		{
			intersects = cell->IntersectsOBB( m_collisionCenter + m_position, m_collisionExtents, m_collisionOrientation );
		}
	}

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
//   importance is simply the ratio of the light radius and the distance to the viewer.
// Arguments:
//   viewerPos - The position of the viewpoint
// Return Value:
//   The view importance
// --------------------------------------------------------------------------------------
float Tr2InteriorLightSource::GetCurrentViewImportance( const Vector3& viewerPos ) const
{
	// importance is based on:
	// 1. dist from camera
	Vector3 dist = viewerPos - m_position;
	float distToViewer = D3DXVec3LengthSq( &dist );

	// put together the result
	float res = m_radius * m_radius / distToViewer;

	return res * m_importanceScale + m_importanceBias;
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
float Tr2InteriorLightSource::GetCurrentShadowImportance( unsigned int shadowMapIndex, const Vector3& viewerPos ) const
{
	return GetCurrentViewImportance( viewerPos ) * m_shadowImportance * m_shadowImportance;
}

void Tr2InteriorLightSource::AddToScene( void )
{
	// Set the dirty flag
	m_isDirty = true;
	for( int i = 0; i < 6; ++i )
	{
		m_emptyShadow[i] = false;
		m_dirtySpotLightShadow[i] = true;
	}
}


void Tr2InteriorLightSource::RemoveFromScene( void )
{
	for( int i = 0; i < 6; ++i )
	{
		m_shadowAtlasTexture[i] = NULL;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Per-object data for light rendering during light accumulation pass in pre-pass scenes.
//   Sets a shadow map and projection texture.
// --------------------------------------------------------------------------------------
class Tr2InteriorLightSource::PerLightData : public Tr2ShadowPerLightData<Tr2ProjectedPerLightData>
{
};

// --------------------------------------------------------------------------------------
// Description:
//   Adds light's geometry to a render batch accumulator. Used during light accumulation 
//   pass.
// Arguments:
//   batches - Render batch accumulator to add batch to
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::GetBatches( ITriRenderBatchAccumulator* batches, const Matrix& mirrorToWorldMatrix )
{
	PerLightData* data = batches->Allocate<PerLightData>(); 

	if( !data )
	{
		return;
	}

	data->SetShadowTexture( m_shadowAtlasTexture, 6 );

	data->SetProjectedTexture( m_projectedTextureRes );

	Tr2InteriorPerLightPSData perObjectPSBuffer;

	PopulateLightData( &perObjectPSBuffer.lightData, mirrorToWorldMatrix );
	perObjectPSBuffer.lightData.shadow0Influence = 0.0f;
	perObjectPSBuffer.additionalParameters.x = m_specularIntensity;
	perObjectPSBuffer.mirrorToWorldMatrix = mirrorToWorldMatrix;

	const XMMATRIX texAdjust = XMMatrixSet(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f );
	const int nShadowMatrices = IsSpotLight() ? 1 : 6;
	for( int i = 0; i < nShadowMatrices; ++i )
	{
		perObjectPSBuffer.shadowMatrix[i] = XMMatrixTranspose( XMMatrixMultiply(
			XMMatrixMultiply( XMMatrixTranspose( mirrorToWorldMatrix ), GetViewMatrix( i ) ),
			XMMatrixMultiply( m_projectionMatrix, texAdjust ) ) );
	}

	for( int i = 0; i < 6; ++i )
	{
		if( m_shadowAtlasTexture[i] )
		{
			perObjectPSBuffer.shadowRect[i].x = float( m_shadowAtlasTexture[i]->GetX() + 0.5f ) / 
				m_shadowAtlasTexture[i]->GetTextureWidth();
			perObjectPSBuffer.shadowRect[i].y = float( m_shadowAtlasTexture[i]->GetY() + 0.5f ) / 
				m_shadowAtlasTexture[i]->GetTextureHeight();
			perObjectPSBuffer.shadowRect[i].z = float( m_shadowAtlasTexture[i]->GetWidth() - 1.0f ) / 
				m_shadowAtlasTexture[i]->GetTextureWidth();
			perObjectPSBuffer.shadowRect[i].w = float( m_shadowAtlasTexture[i]->GetHeight() - 1.0f ) / 
				m_shadowAtlasTexture[i]->GetTextureHeight();
			perObjectPSBuffer.shadowInfluence[i] = Vector4( 1.0f, 0.0f, 0.0f, 0.0f );
		}
		else
		{
			perObjectPSBuffer.shadowRect[i] = Vector4( 0.0f, 0.0f, 1.0f, 1.0f );
			perObjectPSBuffer.shadowInfluence[i] = Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
		}
	}

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	XMVECTOR det;
	Matrix mirrorMatrix( XMMatrixInverse( &det, XMMatrixTranspose( mirrorToWorldMatrix ) ) );

	const Vector4* vertexes;
	unsigned vertexCount;
	Matrix worldView, worldViewTranspose, worldViewProj;
	Tr2VertexBufferAL* vb;
	Tr2IndexBufferAL* ib;

	if( IsSpotLight() )
	{
		vertexes = unitConeVertices;
		vertexCount = unitConeVertexCount;
	}
	else
	{
		vertexes = unitIcosahedronVertices;
		vertexCount = unitIcosahedronVertexCount;
	}
	vb = m_sharedVB;
	ib = m_sharedIB;
	worldView = m_unitToWorldTransform * mirrorMatrix * Tr2Renderer::GetViewTransform();

	worldViewProj = worldView * Tr2Renderer::GetProjectionTransform();
	D3DXMatrixTranspose( &worldViewProj, &worldViewProj );
	data->CopyToVSFloatBuffer( worldViewProj );

	bool isInside = false;
	if( XMVectorGetX( det ) < 0 )
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
//   Set shadow empty flag. The interior scene checks if the 
//   shadow map frustum has any shadow geometry to render and
//   if it doesn't then the scene removes this shadow map from
//   atlas and marks it as empty. 
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   emptyShadow Flag indicating that the shadow map is empty.
// -------------------------------------------------------------
void Tr2InteriorLightSource::SetEmptyShadow( unsigned int shadowMapIndex, bool emptyShadow )
{
	m_emptyShadow[shadowMapIndex] = emptyShadow;
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
bool Tr2InteriorLightSource::IsShadowEmpty( unsigned int shadowMapIndex ) const
{
	return m_emptyShadow[shadowMapIndex];
}

// -------------------------------------------------------------
// Description:
//   Returns the number of shadow map textures in shadow atlas
//   required for this light source.
// Return Value:
//   Number of shadow map textures in shadow atlas required
// -------------------------------------------------------------
unsigned int Tr2InteriorLightSource::GetRequiredShadowMapCount() const
{
	if( m_primaryLighting && m_shadowImportance > 0.0f && m_shadowCasterTypes != ST_NONE )
	{
		if( IsSpotLight() )
		{
			return 1;
		}
		else
		{
			return 6;
		}
	}
	return 0;
}

// -------------------------------------------------------------
// Description:
//   Returns the number of frames since the shadow was updated 
//   for lights. This method is only called for light sources
//   that manage their own shadow maps.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Number of frames since the shadow was updated
// -------------------------------------------------------------
unsigned int Tr2InteriorLightSource::GetFramesSinceShadowUpdate( unsigned int shadowMapIndex ) const 
{ 
	return m_framesSinceShadowUpdate[shadowMapIndex]; 
}

// -------------------------------------------------------------
// Description:
//   Sets the number of frames since the shadow was updated for 
//   lights. This method is only called for light sources
//   that manage their own shadow maps.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   framesSinceShadowUpdate - Number of frames since the shadow was updated
// -------------------------------------------------------------
void Tr2InteriorLightSource::SetFramesSinceShadowUpdate( unsigned int shadowMapIndex, unsigned int framesSinceShadowUpdate ) 
{ 
	m_framesSinceShadowUpdate[shadowMapIndex] = framesSinceShadowUpdate; 
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
void Tr2InteriorLightSource::CacheShadowMapResolution()
{
	if( !m_enableShadowLOD )
	{
		m_currentShadowResolution = m_shadowResolution;
		return;
	}

	float size;
	if( IsSpotLight() )
	{
		size = ::GetScreenSize<unitConeVertexCount - 1>( unitConeVertices + 1, m_unitToWorldTransform );
	}
	else
	{
		size = ::GetScreenSize<unitIcosahedronVertexCount>( unitIcosahedronVertices, m_unitToWorldTransform );
	}

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
//   Returns dimensions of shadow map required for this light.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   maximumResolution - If true always return light maximum 
//						 shadow resolution, otherwise can apply 
//						 LOD scheme.
//   width (out) - Width of shadow map
//   height (out) - Height of shadow map
// -------------------------------------------------------------
void Tr2InteriorLightSource::GetRequiredShadowMapResolution( unsigned int index, bool maximumResolution, unsigned int& width, unsigned int& height ) const
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
//   Returns atlas texture containing the given shadow map. Can
//   return NULL if the texture is not required of was not yet
//   assigned.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Atlas texture containing the shadow map
// -------------------------------------------------------------
Tr2AtlasTexture* Tr2InteriorLightSource::GetShadowAtlasTexture( unsigned int index ) const
{
	return m_shadowAtlasTexture[index];
}

// -------------------------------------------------------------
// Description:
//   Assigns atlas texture for the given shadow map. 
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   texture - Atlas texture containing the shadow map
// -------------------------------------------------------------
void Tr2InteriorLightSource::SetShadowAtlasTexture( unsigned int index, Tr2AtlasTexture* texture )
{
	bool rebind = ( m_shadowAtlasTexture[index] == NULL ) != ( texture == NULL );

	m_shadowAtlasTexture[index] = texture;

	if( rebind )
	{
		ChooseLightEffect();
	}
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
bool Tr2InteriorLightSource::BeginShadowUpdate( unsigned int shadowMapIndex, Tr2PerFrameVSData* vsData, 
											    Tr2PerFrameShadowPSData* psData, Tr2RenderContext& renderContext )
{
	D3DPERF_EVENT( L"Tr2InteriorLightSource::BeginShadowUpdate" );

	Tr2Renderer::SetViewTransform( m_viewMatrix[shadowMapIndex] );
	Tr2Renderer::SetProjectionTransform( m_projectionMatrix );

	// pre-mults
	Matrix viewProjMat = m_viewMatrix[shadowMapIndex] * m_projectionMatrix;

	// column_major for shaders
	D3DXMatrixTranspose( &vsData->ViewMat, &m_viewMatrix[shadowMapIndex] );
	D3DXMatrixTranspose( &vsData->ProjectionMat, &m_projectionMatrix );
	D3DXMatrixTranspose( &vsData->ViewProjectionMat, &viewProjMat );
	D3DXMatrixInverse( &vsData->ViewInverseTransposeMat, NULL, &m_viewMatrix[shadowMapIndex] );

	vsData->sunDirWorld.x = 0.0f;		// light type: projected
	psData->lightRadius = std::max( m_radius, 0.0f );

	// clear
	CR( renderContext.Clear( CLEARFLAGS_TARGET | CLEARFLAGS_ZBUFFER | CLEARFLAGS_STENCIL, 0x00ffff00, 1, 0 ) );

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Restores render target previously set in a call to BeginShadowUpdate. Does nothing.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::EndShadowUpdate( unsigned int shadowMapIndex )
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
const Matrix& Tr2InteriorLightSource::GetViewMatrix( unsigned int shadowMapIndex ) const
{
	return m_viewMatrix[shadowMapIndex];
}

// -------------------------------------------------------------
// Description:
//   Returns projection transform matrix for shadow map.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Shadow map view to projection space transform.
// -------------------------------------------------------------
const Matrix& Tr2InteriorLightSource::GetProjectionMatrix( unsigned int shadowMapIndex ) const
{
	return m_projectionMatrix;
}

// --------------------------------------------------------------------------------------
// Description:
//   Recalculates spotlight view frustum. Is called when any of parameters affecting 
//   frusum change.
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::UpdateSpotlightFrustum()
{
	CTriViewport viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = m_shadowResolution;
	viewport.height = m_shadowResolution;
	viewport.minZ = 0.0f;
	viewport.maxZ = 1.0f;

	for( int i = 0; i < 6; ++i )
	{
		m_frustum[i].DeriveFrustum( &m_viewMatrix[i], &m_position, &m_projectionMatrix, viewport );

		if( IsSpotLight() )
		{
			break;
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Check intersection betweeen light and a dynamic object.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   dynamic - dynamic object to check intersection with
// Return Value:
//   true if light and a dynamic object might intersect
//   false if light and a dynamic object don't intersect
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::MarkShadowsDirtyForBounds( const Vector3 &minBounds, const Vector3 &maxBounds )
{
	if( !IsSpotLight() )
	{
		Vector4 sphere( m_position.x, m_position.y, m_position.z, m_radius );
		if( IntersectSphereAxisAlignedBox( sphere, minBounds, maxBounds ) )
		{
			for( int i = 0; i < 6; ++i )
			{
				if( m_frustum[i].IsBoxVisible( minBounds, maxBounds ) )
				{
					MarkShadowDirty( i, true );
				}
			}
		}
	}
	else
	{
		if( m_frustum[0].IsBoxVisible( minBounds, maxBounds ) )
		{
			MarkShadowDirty( 0, true );
		}
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
void Tr2InteriorLightSource::MarkShadowDirty( unsigned int shadowMapIndex, bool dirty )
{
	m_dirtySpotLightShadow[shadowMapIndex] = dirty;
	if( dirty )
	{
		m_emptyShadow[shadowMapIndex] = false;
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
bool Tr2InteriorLightSource::IsShadowDirty( unsigned int shadowMapIndex ) const
{
	return m_dirtySpotLightShadow[shadowMapIndex];
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns type of objects rendered into the shadow map.
// --------------------------------------------------------------------------------------
Tr2InteriorLightSource::ShadowCasterTypes Tr2InteriorLightSource::GetShadowCasterTypes() const
{
	return m_shadowCasterTypes;
}

// -------------------------------------------------------------
// Description:
//   Per-frame update method. Updates curve sets.
// Arguments:
//   time - Current system time.
// -------------------------------------------------------------
void Tr2InteriorLightSource::Update( Be::Time time )
{
	for( auto it = m_curveSets.cbegin(); it != m_curveSets.cend(); ++it )
	{
		( *it )->Update( TimeAsDouble( time ) );
	}
}

void Tr2InteriorLightSource::RebuildVolume( void )
{
	if( IsSpotLight() )
	{
		// direct:
		float size = sinf( m_coneAlphaOuter / 180.f * XM_PI ) * m_radius;

		Matrix obbRotMatrix, obbScaleMatrix, obbTransMatrix;
		TriMatrixArcFromForward( &obbRotMatrix, &m_coneDirection );
		D3DXMatrixTranslation( &obbTransMatrix, 0.f, 0.f, -0.5f * m_radius );
		D3DXMatrixScaling( &obbScaleMatrix, size, size, 0.5f * m_radius );

		// Now build some vectors & quats for our own collision detection routines
		// Since the center is in world coordinates, we must pre-multiply the direction of the light
		Vector3 centerOffset;
		D3DXVec3Normalize( &centerOffset, &m_coneDirection );
		centerOffset *= 0.5f * m_radius;
		m_collisionCenter = centerOffset;
		m_collisionExtents = Vector3( size, size, 0.5f * m_radius );

		D3DXQuaternionRotationMatrix( &m_collisionOrientation, &obbRotMatrix );
		D3DXQuaternionNormalize( &m_collisionOrientation, &m_collisionOrientation );
	}

	// Set the dirty flag
	m_isDirty = true;
	for( int i = 0; i < 6; ++i )
	{
		m_dirtySpotLightShadow[i] = true;
		m_emptyShadow[i] = false;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Chooses an appropriate effect for light rendering during light accumulation pass.
//   Accounts for if the light is a spot light or not and if there is an assigned shadow
//   map.
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::ChooseLightEffect()
{
	Tr2ShaderSituation situation;


	if( IsSpotLight() )
	{
		if( m_customMaterial )
		{
			situation.AddSituationString( "SpotLight" );
		}
		else
		{
			m_material->SetHighLevelShaderName( "SpotLight" );
			m_shMaterial->SetHighLevelShaderName( "SpotLight" );
		}
		if( m_shadowAtlasTexture[0] )
		{
			situation.AddSituationString( "Shadow" );
		}
	}
	else
	{
		if( !m_customMaterial )
		{
			m_material->SetHighLevelShaderName( "PointLight" );
			m_shMaterial->SetHighLevelShaderName( "PointLight" );
		}
		for( int i = 0; i < 6; ++i )
		{
			if( m_shadowAtlasTexture[i] )
			{
				situation.AddSituationString( "Shadow" );
				break;
			}
		}
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
//   Releases shadow texture or marks shadow as dirty/invalid.
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::CreateShadowMap()
{
	if( m_primaryLighting && m_shadowImportance > 0.0f && m_shadowCasterTypes != ST_NONE )
	{
		if( m_shadowAtlasTexture[0] == NULL )
		{
			for( int i = 0; i < 6; ++i )
			{
				m_emptyShadow[i] = false; 
				m_dirtySpotLightShadow[i] = true;
			}
		}
	}
	else
	{
		for( int i = 0; i < 6; ++i )
		{
			m_shadowAtlasTexture[i] = NULL;
		}
	}
	ChooseLightEffect();
}

// -------------------------------------------------------------
// Description:
//   Get batches for a light to use during SH lighting step.
// Arguments:
//   batches - Batch accumulator to add batches to.
// -------------------------------------------------------------
void Tr2InteriorLightSource::GetSHBatches( ITriRenderBatchAccumulator* batches ) const
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

	data->SetShadowTexture( m_shadowAtlasTexture, 6 );
	data->SetProjectedTexture( m_projectedTextureRes );

	Tr2InteriorPerLightPSData perObjectPSBuffer;

	PopulateLightData( &perObjectPSBuffer.lightData, Tr2Renderer::GetIdentityTransform() );

	perObjectPSBuffer.lightData.shadow0Influence = 0.0f;
	perObjectPSBuffer.additionalParameters.x = m_specularIntensity;
	D3DXMatrixIdentity( &perObjectPSBuffer.mirrorToWorldMatrix );

	for( int i = 0; i < 6; ++i )
	{
		Matrix texAdjust( 0.5f, 0.0f, 0.0f, 0.0f,
						  0.0f, -0.5f, 0.0f, 0.0f,
						  0.0f, 0.0f, 1.0f, 0.0f,
						  0.5f, 0.5f, 0.0f, 1.0f );
		perObjectPSBuffer.shadowMatrix[i] = GetViewMatrix( i ) * m_projectionMatrix * texAdjust;
		D3DXMatrixTranspose( &perObjectPSBuffer.shadowMatrix[i], &perObjectPSBuffer.shadowMatrix[i] );
		if( IsSpotLight() )
		{
			break;
		}
	}

	for( int i = 0; i < 6; ++i )
	{
		if( m_shadowAtlasTexture[i] )
		{
			perObjectPSBuffer.shadowRect[i].x = float( m_shadowAtlasTexture[i]->GetX() + 0.5f ) / 
				m_shadowAtlasTexture[i]->GetTextureWidth();
			perObjectPSBuffer.shadowRect[i].y = float( m_shadowAtlasTexture[i]->GetY() + 0.5f ) / 
				m_shadowAtlasTexture[i]->GetTextureHeight();
			perObjectPSBuffer.shadowRect[i].z = float( m_shadowAtlasTexture[i]->GetWidth() - 1.0f ) / 
				m_shadowAtlasTexture[i]->GetTextureWidth();
			perObjectPSBuffer.shadowRect[i].w = float( m_shadowAtlasTexture[i]->GetHeight() - 1.0f ) / 
				m_shadowAtlasTexture[i]->GetTextureHeight();
			perObjectPSBuffer.shadowInfluence[i] = Vector4( 1, 0, 0, 0 ); 
		}
		else
		{
			perObjectPSBuffer.shadowRect[i] = Vector4( 0.0f, 0.0f, 1.0f, 1.0f );
			perObjectPSBuffer.shadowInfluence[i] = Vector4( 0, 0, 0, 0 ); 
		}
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
//  Rebuilds the light geometry clipped by the bounding box.
// -------------------------------------------------------------
void Tr2InteriorLightSource::RebuildGeometry()
{
	if( IsSpotLight() )
	{
		m_sharedVB = Tr2ConstGeometry::GetVB( unitConeVertices, unitConeVertexCount * sizeof( Vector4 ) );
		m_sharedIB = Tr2ConstGeometry::GetIB( IB_32BIT, unitConeIndices, sizeof( unitConeIndices ) / sizeof( unitConeIndices[0] ) );
	}
	else
	{
		m_sharedVB = Tr2ConstGeometry::GetVB( unitIcosahedronVertices, unitIcosahedronVertexCount * sizeof( Vector4 ) );
		m_sharedIB = Tr2ConstGeometry::GetIB( IB_32BIT, unitIcosahedronIndices, sizeof( unitIcosahedronIndices ) / sizeof( unitIcosahedronIndices[0] ) );
	}
}

bool Tr2InteriorLightSource::IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const
{
	if( !m_primaryLighting )
	{
		return false;
	}
	D3DXMatrixTranslation( &objectToWorld, m_position.x, m_position.y, m_position.z );
	return frustum.IsBoxVisible( m_worldBoundingBox.m_min, m_worldBoundingBox.m_max );
}

