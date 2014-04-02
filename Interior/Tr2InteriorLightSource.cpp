#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorLightSource.h"
#include "Tr2InteriorConstantBufferFormats.h"
#include "TriDebugResourceHelper.h"

#include "umbraTypes.h"

#include "Utilities/BoundingSphere.h"
#include "Tr2AtlasTexture.h"
#include "Tr2InteriorCell.h"
#include "Tr2InteriorLightGeometryRenderBatch.h"
#include "Tr2KelvinColor.h"
#include "ITr2UmbraUserData.h"
#include "Tr2InteriorOrientedBoundingBox.h"
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
	PARENTLOCK( m_explicitAffectedCells ),
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
	m_secondaryLighting( true ),
	m_secondaryLightingMultiplier( 1.0f ),
	m_affectTransparentObjects( true ),
	m_enableROIs( true ),
	m_importanceScale( 1.0f ),
	m_importanceBias( 0.0f ),
	m_isDirty( true ),
	m_isStatic( true ),
	m_staticFlagChanged( true ),
	m_umbraModel( NULL ),
	m_umbraRoiList(),
	m_cellIntersectionType( IT_BOUNDING_BOX ),
	m_lightVertexDecl( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_shadowResolution( 256 ),
	m_enableShadowLOD( true ),
	m_shadowCasterTypes( ST_ALL ),
	m_renderDebugInfo( false ),
	m_renderDebugType( DI_LIGHT_COLOR ),
	PARENTLOCK( m_curveSets ),
	m_vertexes( NULL ),
	m_vertexCount( 0 ),
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

	m_explicitAffectedCells.SetNotify( this );

	D3DXMatrixIdentity( &m_unitToWorldTransform );

	m_material.CreateInstance();
	m_shMaterial.CreateInstance();
	m_lightmapMaterial.CreateInstance();

	m_kelvinColor.CreateInstance();
	m_useKelvinColor = false;

	m_material->SetHighLevelShaderName( "PointLight" );
	m_shMaterial->SetHighLevelShaderName( "PointLight" );
	m_lightmapMaterial->SetHighLevelShaderName( "PointLight" );
	PrepareResources();
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorLightSource destructor.
// --------------------------------------------------------------------------------------
Tr2InteriorLightSource::~Tr2InteriorLightSource()
{
	// Clear Umbra data
	ClearUmbra();

	if( m_projectedTextureRes )
	{
		m_projectedTextureRes->RemoveNotifyTarget( this );
		m_projectedTextureRes.Unlock();
	}

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
//   a new round of light-cell intersection tests on the next scene Update.  If the light 
//   radius, cone direction, or cone outer angle changes, then a new Umbra model is 
//   constructed and given to the regions of influence.  The light is flagged as dirty.
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
		// Compute a world-translation matrix
		Matrix translationMatrix;
		D3DXMatrixTranslation( &translationMatrix, m_position.x, m_position.y, m_position.z );

		// Update the matrices on the regions of influence
		for( std::vector<Umbra::RegionOfInfluence*>::iterator it = m_umbraRoiList.begin(); 
			 it != m_umbraRoiList.end(); ++it )
		{
			if( ( *it )->getCell() != NULL )
			{
				Matrix cellTransform;
				( *it )->getCell()->getCellToWorldMatrix( AS_UMBRA_MATRIX( cellTransform ) );
				D3DXMatrixInverse( &cellTransform, NULL, &cellTransform );
				cellTransform = translationMatrix * cellTransform;
				( *it )->setObjectToCellMatrix( AS_UMBRA_MATRIX( cellTransform ) );
			}
			else
			{
				( *it )->setObjectToCellMatrix( AS_UMBRA_MATRIX( translationMatrix ) );
			}
		}

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
	// If the radius, cone angle, or direction changes, we need to rebuild the Umbra model
	else if( IsMatch( value, m_radius )			||
			 IsMatch( value, m_coneAlphaOuter ) ||
			 IsMatch( value, m_coneDirection ) )
	{
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
	else if( IsMatch( value, m_cellIntersectionType ) )
	{
		// Mark the dirty flag
		m_isDirty = true;
		for( int i = 0; i < 6; ++i )
		{
			m_emptyShadow[i] = false;
			m_dirtySpotLightShadow[i] = true;
		}
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
	else if( IsMatch( value, m_affectTransparentObjects ) )
	{
		for( std::vector<Umbra::RegionOfInfluence*>::iterator it = m_umbraRoiList.begin(); it != m_umbraRoiList.end(); ++it )
		{
			( *it )->set( Umbra::Object::ENABLED, m_affectTransparentObjects && m_enableROIs );
		}
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
			m_lightmapMaterial = NULL;

			m_material.CreateInstance();
			m_shMaterial.CreateInstance();
			m_lightmapMaterial.CreateInstance();

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
		m_lightmapMaterial = NULL;

		BeClasses->CloneTo( m_customMaterial->GetRawRoot(), (IRoot**)&m_shMaterial );
		BeClasses->CloneTo( m_customMaterial->GetRawRoot(), (IRoot**)&m_lightmapMaterial );
		ChooseLightEffect();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the projected texture resource.
// Arguments:
//   texture - New texture resource
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::SetProjectedTexture( TriTextureRes* texture )
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
//   Sets the dirty flag when the explicitAffectCells list changes.
// Arguments:
//   event		- The list-changed event type (ignored)
//   key		- The primary list key (ignored)
//   key2		- The secondary list key (ignored)
//   value		- The list item that was modified (ignored)
//   theList	- The list that generated the event (ignored)
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::OnListModified( long event, ssize_t key, ssize_t key2, 
											 IRoot* value, const IList* theList )
{
	// Mark the dirty flag
	m_isDirty = true;
	for( int i = 0; i < 6; ++i )
	{
		m_emptyShadow[i] = false;
		m_dirtySpotLightShadow[i] = true;
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
		float halfAngle = D3DXToRadian( m_coneAlphaOuter );
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
		D3DXMatrixInverse( &m_invViewMatrix[0], NULL, &m_viewMatrix[0] );
		D3DXMatrixPerspectiveFovRH( &m_projectionMatrix, D3DXToRadian( m_coneAlphaOuter * 2 ), 1.f, 0.01f, m_radius );
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

		for( int i = 0; i < 6; ++i )
		{
			D3DXMatrixInverse( &m_invViewMatrix[i], NULL, &m_viewMatrix[i] );
		}

		D3DXMatrixPerspectiveFovRH( &m_projectionMatrix, D3DX_PI / 2.f, 1.f, 0.01f, m_radius );
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
	if( m_VB.GetMemoryClass() & s )
	{
		m_VB.Destroy();
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

	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( m_boundingBox && !m_VB.IsValid() )
	{
		m_VB.Create( m_vertexCount * sizeof( Vector4 ), USAGE_IMMUTABLE, m_vertexes, renderContext );
	}
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
// Arguments:
//   minBounds - The min bounds of the BB
//   maxBounds - The max bounds of the BB
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::GetBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
{
	minBounds = m_position - Vector3( m_radius, m_radius, m_radius );
	maxBounds = m_position + Vector3( m_radius, m_radius, m_radius );
}

// ---------------------------------------------------------------------------------------
// Description:
//   Sets the value of the isStatic flags (which controls whether or not the light is
//   cached in Enlighten).
// Arguments:
//   isStatic - The new value of the isStatic flag
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::SetStatic( bool isStatic )
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
bool Tr2InteriorLightSource::TestCellIntersectionAndAdd( Tr2InteriorCell* cell )
{
	// Bail out if the cell is invalid
	if( cell == NULL )
	{
		// No cell, return no intersection
		return false;
	}

	Matrix transformInv;
	D3DXMatrixInverse( &transformInv, NULL, &cell->GetWorldTransform() );

	bool intersects = cell->IsUnbounded();

	if( !intersects )
	{
		switch( m_cellIntersectionType )
		{
		case IT_BOUNDING_BOX:
			// Test point light
			if( !IsSpotLight() )
			{
				intersects = cell->IntersectsSphere( m_position, m_radius );
			}
			// Test spot light
			else
			{
				intersects = cell->IntersectsOBB( m_collisionCenter + m_position, m_collisionExtents, m_collisionOrientation );
			}
			break;
		case IT_POINT:
			{
				intersects = cell->ContainsPoint( m_position );
			}
			break;
		case IT_MANUAL:
		default:
			intersects = HasCellInManualList( cell );
			break;
		}
	}

	// If we got an intersection, setup the Umbra Region-of-influence
	if( intersects )
	{
		// Compute current transform matrix
		Matrix transMat;
		D3DXMatrixTranslation( &transMat, m_position.x, m_position.y, m_position.z );
		transMat = transMat * transformInv;

		// Only do the ROI setup if the cell doesn't already contain this light
		if( cell->AddLight( this ) )
		{
			// See if we already have an ROI for this cell
			Umbra::RegionOfInfluence* roi = Umbra::RegionOfInfluence::create( m_umbraModel );
			roi->setCell( cell->GetUmbraCell() );
			roi->setObjectToCellMatrix( AS_UMBRA_MATRIX( transMat ) );

			// Set our own userdata to this ROI
			void* userData = CONVERT_TO_UMBRA_USER_DATA( this );

			roi->setUserPointer( userData );
			roi->set( Umbra::Object::ENABLED, m_affectTransparentObjects && m_enableROIs );

			// Store the roi in our list
			m_umbraRoiList.push_back( roi );

			Umbra::Object* obj = Umbra::Object::create( m_umbraModel );
			obj->setCell( cell->GetUmbraCell() );
			obj->setObjectToCellMatrix( AS_UMBRA_MATRIX( transMat ) );

			obj->setUserPointer( userData );

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

// --------------------------------------------------------------------------------------
// Description:
//   Adds the light to the umbra scene
// --------------------------------------------------------------------------------------
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


// --------------------------------------------------------------------------------------
// Description:
//   Removes the light from the Umbra scene.
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::RemoveFromScene( void )
{
	ClearUmbra();
	for( int i = 0; i < 6; ++i )
	{
		m_shadowAtlasTexture[i] = NULL;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the cell intersection type
// Return Value:
//   The cell intersection type
// --------------------------------------------------------------------------------------
Tr2InteriorLightSource::CellIntersectionType 
	Tr2InteriorLightSource::GetCellIntersectionType() const
{
	return m_cellIntersectionType;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the cell intersection type
// Arguments:
//   cellIntersectionType - The new cell intersection type
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::SetCellIntersectionType( 
	CellIntersectionType cellIntersectionType )
{
	m_cellIntersectionType = cellIntersectionType;
}

// ---------------------------------------------------------------------------------------
// Description:
//   Checks to see whether the given cell is in the manual explicit cell list
// Arguments:
//   cell - The cell to test
// Return Value:
//   true, if the cell is in the explicitAffectedCells list
//   false, otherwise
// --------------------------------------------------------------------------------------
bool Tr2InteriorLightSource::HasCellInManualList( const Tr2InteriorCell* cell ) const
{
	for( PTr2InteriorCellVector::const_iterator it = m_explicitAffectedCells.begin(); 
		 it != m_explicitAffectedCells.end(); ++it )
	{
		if( cell == *it )
		{
			return true;
		}
	}
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds the cell to the explicitAffectedCell list
// Arguments:
//   cell - The cell to add
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::AddCellToManualList( Tr2InteriorCell* cell )
{
	if( HasCellInManualList( cell ) )
	{
		return;
	}
	m_explicitAffectedCells.Insert( -1, cell->GetRawRoot() );
	for( int i = 0; i < 6; ++i )
	{
		m_emptyShadow[i] = false;
		m_dirtySpotLightShadow[i] = true;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes the cell from the explicitAffectedCell list
// Arguments:
//   cell - The cell to remove
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::CellRemoved( Tr2InteriorCell* cell )
{
	ssize_t key = m_explicitAffectedCells.FindKey( cell->GetRawRoot() );
	if( key != -1 )
	{
		m_explicitAffectedCells.Remove( key );
		for( int i = 0; i < 6; ++i )
		{
			m_emptyShadow[i] = false;
			m_dirtySpotLightShadow[i] = true;
		}
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
void Tr2InteriorLightSource::GetBatches( ITriRenderBatchAccumulator* batches )
{
	GetInstancedBatches( batches, Tr2Renderer::GetIdentityTransform() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds light's geometry to a render batch accumulator. Used during light accumulation 
//   pass.
// Arguments:
//   batches - Render batch accumulator to add batch to
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::GetInstancedBatches( ITriRenderBatchAccumulator* batches,
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

	data->SetShadowTexture( m_shadowAtlasTexture, 6 );

	data->SetProjectedTexture( m_projectedTextureRes );

	Tr2InteriorPerLightPSData perObjectPSBuffer;

	PopulateLightData( &perObjectPSBuffer.lightData, mirrorToWorldMatrix );
	perObjectPSBuffer.lightData.shadow0Influence = 0.0f;
	perObjectPSBuffer.additionalParameters.x = m_specularIntensity;
	perObjectPSBuffer.mirrorToWorldMatrix = mirrorToWorldMatrix;
	if( m_boundingBox )
	{
		D3DXMatrixInverse( &perObjectPSBuffer.boundingBox, NULL, &m_boundingBox->GetTransform() );
		D3DXMatrixTranspose( &perObjectPSBuffer.boundingBox, &perObjectPSBuffer.boundingBox );
	}

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
	}

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
	if( UseWithPrimaryLighting() && m_shadowImportance > 0.0f && m_shadowCasterTypes != ST_NONE )
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

	if( size <= 0 || !_finite( size ) )
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

// --------------------------------------------------------------------------------------
// Description:
//   Returns inverse of spotlight view matrix.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Inverse of spotlight view matrix
// --------------------------------------------------------------------------------------
const Matrix& Tr2InteriorLightSource::GetInvViewMatrix( unsigned int shadowMapIndex ) const
{
	return m_invViewMatrix[shadowMapIndex];
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
//   Debug/testing method to check if a given AABB intersect shadow frustums (and thus
//   could trigger shadow updates).
// Arguments:
//   minBounds - Min AABB bounds
//   minBounds - Max AABB bounds
// Return Value:
//   true if AABB intersects one of light's shadow frustums
//   false otherwise
// --------------------------------------------------------------------------------------
bool Tr2InteriorLightSource::TestShadowFrustumBoxIntersection( const Vector3 &minBounds, const Vector3 &maxBounds )
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
					return true;
				}
			}
		}
	}
	else
	{
		if( m_frustum[0].IsBoxVisible( minBounds, maxBounds ) )
		{
			return true;
		}
	}
	return false;
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
//   Returns light's position for "light spider" tool.
// Return Value:
//   Position to render a ray to for "light spider" tool.
// -------------------------------------------------------------
Vector3 Tr2InteriorLightSource::GetPositionForLightSpider( const Matrix &objectToWorldMatrix ) const
{
	Vector3 position;
	D3DXVec3TransformCoord( &position, &m_position, &objectToWorldMatrix );
	return position;
}


// -------------------------------------------------------------
// Description:
//   Render light source debug information.
// -------------------------------------------------------------
void Tr2InteriorLightSource::RenderDebugInfo( Tr2RenderContext& renderContext ) const
{
	if( !m_renderDebugInfo )
	{
		return;
	}
	if( ( m_renderDebugType == DI_SHADOW_RESOLUTION || m_renderDebugType == DI_SHADOW_RELATIVE_RESOLUTION ) && GetRequiredShadowMapCount() == 0 )
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
	case DI_SHADOW_RESOLUTION:
		color = GetDebugLightShadowResultionColor( m_shadowAtlasTexture[0] ? m_shadowAtlasTexture[0]->GetWidth() : 0 );
		break;
	case DI_SHADOW_RELATIVE_RESOLUTION:
		color = GetDebugLightShadowRelativeResultionColor( m_shadowResolution, m_shadowAtlasTexture[0] ? m_shadowAtlasTexture[0]->GetWidth() : 0 );
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
		if( IsSpotLight() )
		{
			RenderDebugLightVolume(
				reinterpret_cast<const Vector3*>( unitConeVertices ),
				unitConeVertexCount,
				sizeof( Vector4 ),
				unitConeIndices,
				unitConeTriangleCount,
				unitConeWireIndices,
				unitConeWireLineCount,
				m_unitToWorldTransform,
				color,
				renderContext
				);
		}
		else
		{
			RenderDebugLightVolume(
				reinterpret_cast<const Vector3*>( unitIcosahedronVertices ),
				unitIcosahedronVertexCount,
				sizeof( Vector4 ),
				unitIcosahedronIndices,
				unitIcosahedronTriangleCount,
				unitIcosahedronIndices,
				unitIcosahedronTriangleCount,
				m_unitToWorldTransform,
				color,
				renderContext
				);
		}
	}
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

// -------------------------------------------------------------
// Description:
//   Enables/disables Umbra regions of influeces for this light
//   source.
// Arguments:
//   enable - If true enable Umbra ROI; 
//            if false disable Umbra ROI
// -------------------------------------------------------------
void Tr2InteriorLightSource::EnableROI( bool enable )
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
//   Removes the cell from the Umbra scene by destroying all the ROI's and the Umbra 
//   model
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::ClearUmbra( void )
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
void Tr2InteriorLightSource::RebuildVolume( void )
{
	// Clear the Umbra model
	m_umbraModel->release();
	m_umbraModel = NULL;

	// Build a volume for point light
	if( !IsSpotLight() )
	{
		// Build the Umbra model
		m_umbraModel = ( Umbra::Model* )Umbra::SphereModel::create( 
			AS_UMBRA_VECTOR3( Vector3( 0.f, 0.f, 0.f ) ), m_radius );
	}
	// Build a volume for spot light
	else
	{
		// direct:
		float size = sinf( D3DXToRadian( m_coneAlphaOuter ) ) * m_radius;

		// UMBRA needs a 4x4 matrix for constructing OBB
		Matrix obbRotMatrix, obbScaleMatrix, obbTransMatrix;
		TriMatrixArcFromForward( &obbRotMatrix, &m_coneDirection );
		D3DXMatrixTranslation( &obbTransMatrix, 0.f, 0.f, -0.5f * m_radius );
		D3DXMatrixScaling( &obbScaleMatrix, size, size, 0.5f * m_radius );
		D3DXMatrixMultiply( &m_obbMatrix, &obbScaleMatrix, &obbTransMatrix );
		D3DXMatrixMultiply( &m_obbMatrix, &m_obbMatrix, &obbRotMatrix );
		m_umbraModel = ( Umbra::Model* )Umbra::OBBModel::create( AS_UMBRA_MATRIX( m_obbMatrix ) );

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
	for( int i = 0; i < 6; ++i )
	{
		m_dirtySpotLightShadow[i] = true;
		m_emptyShadow[i] = false;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds light source to Enlighten system
// Arguments:
//   dusters - Enlighten lighting/duster cache
//   systemTransformInv - Transform from world CS to Enlighten system CS
// Return value:
//   true If light source was added to Enlighten system
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2InteriorLightSource::AddToEnlightenSystem( Tr2InteriorDusterCache* dusters,
												   const Matrix &systemTransformInv ) const
{
	if( !m_secondaryLighting )
	{
		return false;
	}

	Vector3 position;
	D3DXVec3TransformCoord( &position, &m_position, &systemTransformInv );
	const Geo::v128 position1 = Geo::VConstruct( position.x, position.y, position.z, 0.0f );

	const float scale = m_secondaryLightingMultiplier;
	Color color = m_color;
	if( m_useKelvinColor )
	{
		color = m_kelvinColor->AsRGB();
	}
	const Geo::v128 color1 = Geo::VConstruct(	TriGammaToLinear( color.r ) * scale, 
												TriGammaToLinear( color.g ) * scale,
												TriGammaToLinear( color.b ) * scale, 
												m_color.a );

	Enlighten::InputLight& light = dusters->AddEnlightenLightSource();
	light.m_Position = position1;
	light.m_Radius = 0.f;
	light.m_CutOff = m_radius;
	light.m_FalloffTable = dusters->GetFalloffTable( m_falloff );
	light.m_Intensity = color1;
	light.m_ProjectionMap = NULL;
	light.m_ShadowMap = NULL;

	if( IsSpotLight() )
	{
		Vector3 direction;
		D3DXVec3TransformNormal( &direction, &m_coneDirection, &systemTransformInv );
		Geo::v128 spotDirection = Geo::VConstruct( direction.x, direction.y, direction.z, 0.f );

		light.m_LightType = Enlighten::ENLIGHTEN_SPOT_LIGHT;
		light.m_Direction = spotDirection;
		light.m_InnerConeAngle = 2.f * D3DXToRadian( m_coneAlphaInner );
		light.m_ConeAngle = 2.f * D3DXToRadian( m_coneAlphaOuter );
	}
	else
	{
		light.m_LightType = Enlighten::ENLIGHTEN_POINT_LIGHT;
	}
	return true;
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
			m_lightmapMaterial->SetHighLevelShaderName( "SpotLight" );
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
			m_lightmapMaterial->SetHighLevelShaderName( "PointLight" );
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

	if( m_boundingBox )
	{
		situation.AddSituationString( "AdditionalBoundingBox" );
	}

	m_material->BindLowLevelShader( situation );
	
	situation.AddSituationString( "GenerateSHCoefficients" );
	m_shMaterial->BindLowLevelShader( situation );
	situation.AddSituationString( "GenerateLightMap" );
	m_lightmapMaterial->BindLowLevelShader( situation );
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
	if( m_boundingBox )
	{
		D3DXMatrixInverse( &perObjectPSBuffer.boundingBox, NULL, &m_boundingBox->GetTransform() );
		D3DXMatrixTranspose( &perObjectPSBuffer.boundingBox, &perObjectPSBuffer.boundingBox );
	}

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
//   Get batches for a lightmap rendering.
// Arguments:
//   batches - Batch accumulator to add batches to.
// -------------------------------------------------------------
void Tr2InteriorLightSource::GetLightMapBatches( ITriRenderBatchAccumulator* batches ) const
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
	if( m_boundingBox )
	{
		D3DXMatrixInverse( &perObjectPSBuffer.boundingBox, NULL, &m_boundingBox->GetTransform() );
		D3DXMatrixTranspose( &perObjectPSBuffer.boundingBox, &perObjectPSBuffer.boundingBox );
	}

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
		batch->SetShaderMaterial( m_lightmapMaterial );
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
	CCP_DELETE []m_vertexes;
	m_vertexes = NULL;
	m_vertexCount = 0;

	if( m_boundingBox )
	{
		std::vector<Vector4> lightVertexes;
		if( IsSpotLight() )
		{
			for( unsigned i = 0; i < sizeof( unitConeIndices ) / sizeof( unsigned ); ++i )
			{
				lightVertexes.push_back( unitConeVertices[unitConeIndices[i]] );
			}
		}
		else
		{
			for( unsigned i = 0; i < sizeof( unitIcosahedronIndices ) / sizeof( unsigned ); ++i )
			{
				lightVertexes.push_back( unitIcosahedronVertices[unitIcosahedronIndices[i]] );
			}
		}
		XMVector4TransformStream( (XMFLOAT4*)&lightVertexes[0], 
								  sizeof( Vector4 ), 
								  (XMFLOAT4*)&lightVertexes[0], 
								  sizeof( Vector4 ), 
								  (unsigned int)lightVertexes.size(),
								  m_unitToWorldTransform );

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
			m_vertexes = CCP_NEW( "Tr2InteriorLightSource/m_vertexes" ) Vector4[m_vertexCount];
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
	else
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
}

// --------------------------------------------------------------------------------------
// Description:
//   A utility function that returns a color for light volume visualization based on 
//   shadow resolution.
// Arguments:
//   currentResolution - Shadow map resolution (or 0 if no shadow)
// Return Value:
//   Color for light volume visualizer
// --------------------------------------------------------------------------------------
Color GetDebugLightShadowResultionColor( unsigned currentResolution )
{
	int power = 0;
	if( currentResolution > 0 )
	{
		power = 1 + int( log( float( currentResolution / 16 ) ) / log( 2.f ) + 0.5f );
	}
	static unsigned int colorCodes[] = {
		0x00ff0000,		// Droped shadow (0 resultion)
		0x00ff8000,		// Resolution 16
		0x00ffff00,		// Resolution 32
		0x00ff00ff,		// Resolution 64
		0x000000ff,		// Resolution 128
		0x0000ff00,		// Resolution 256
		0x0000ffff,		// Resolution 512
		0x00ffffff,		// Resolution 1024
	};
	if( power >= sizeof( colorCodes ) / sizeof( int ) )
	{
		power = sizeof( colorCodes ) / sizeof( int ) - 1;
	}
	return colorCodes[power];
}

// --------------------------------------------------------------------------------------
// Description:
//   A utility function that returns a color for light volume visualization based on 
//   shadow resolution relative to the maximum resolution.
// Arguments:
//   originalResultion - Original (maximum) shadow map resolution
//   currentResolution - Shadow map resolution (or 0 if no shadow)
// Return Value:
//   Color for light volume visualizer
// --------------------------------------------------------------------------------------
Color GetDebugLightShadowRelativeResultionColor( unsigned originalResultion, unsigned currentResolution )
{
	int power = 0;
	if( originalResultion == currentResolution )
	{
		power = 1;
	}
	else if( currentResolution > 0 )
	{
		power = 1 + int( log( float( originalResultion / currentResolution ) ) / log( 2.f ) + 0.5f );
	}
	static unsigned int colorCodes[] = {
		0x00ff0000,		// Droped shadow (0 resultion)
		0x00ffffff,		// Full resolution
		0x0000ffff,		// Differes by factor of 2
		0x0000ff00,		// Differes by factor of 4
		0x000000ff,		// Differes by factor of 8
		0x00ff00ff,		// Differes by factor of 16
		0x00ffff00,		// Differes by factor of 32
	};
	if( power >= sizeof( colorCodes ) / sizeof( int ) )
	{
		power = sizeof( colorCodes ) / sizeof( int ) - 1;
	}
	return colorCodes[power];
}

// --------------------------------------------------------------------------------------
// Description:
//   A helper function that renders light volume for debug visualization.
// Arguments:
//   vertices - Array of light volume geometry vertices
//   vertexCount - Number of elements in vertices array
//   vertexStride - Element size of vertices array
//   indices - Triangle indices of light volume geometry
//   triangleCount - Number of triangles in light volume geometry
//   wireIndices - Indices for wireframe light volume geometry
//   wireCount - Number of line segments for wireframe light volume geometry
//   worldTransform - Local-to-world transform for light geometry
//   baseColor - Color of light volume
//   renderContext - Render context
// --------------------------------------------------------------------------------------
void RenderDebugLightVolume(
	const Vector3* vertices,
	unsigned vertexCount,
	unsigned vertexStride,
	const unsigned* indices,
	unsigned triangleCount,
	const unsigned* wireIndices,
	unsigned wireCount,
	const Matrix& worldTransform,
	const Color& baseColor,
	Tr2RenderContext& renderContext
	)
{
	DebugRenderEffectCallback callback, wireCallback;

	callback.polygon = new TriDebugResourceHelper::VertexPosColor[vertexCount];
	callback.vertexCount = vertexCount;
	callback.indices = indices;
	callback.triangleCount = triangleCount;
	XMVector4TransformStream( reinterpret_cast<XMFLOAT4*>( callback.polygon ), 
								sizeof( TriDebugResourceHelper::VertexPosColor ), 
								(const XMFLOAT4*)vertices, 
								vertexStride, 
								vertexCount, 
								worldTransform );
	if( wireIndices )
	{
		wireCallback.polygon = new TriDebugResourceHelper::VertexPosColor[vertexCount];
		wireCallback.vertexCount = callback.vertexCount;
		wireCallback.indices = wireIndices;
		wireCallback.triangleCount = wireCount;
		std::copy( callback.polygon, callback.polygon + callback.vertexCount, wireCallback.polygon );
	}
	else
	{
		wireCallback.polygon = new TriDebugResourceHelper::VertexPosColor[vertexCount * 2];
		wireCallback.vertexCount = callback.vertexCount * 2;
		wireCallback.indices = wireIndices;
		wireCallback.triangleCount = vertexCount; 
		for( unsigned i = 0; i < vertexCount; i += 3 )
		{
			wireCallback.polygon[i * 2 + 0].m_pos = callback.polygon[i + 0].m_pos;
			wireCallback.polygon[i * 2 + 1].m_pos = callback.polygon[i + 1].m_pos;
			wireCallback.polygon[i * 2 + 2].m_pos = callback.polygon[i + 1].m_pos;
			wireCallback.polygon[i * 2 + 3].m_pos = callback.polygon[i + 2].m_pos;
			wireCallback.polygon[i * 2 + 4].m_pos = callback.polygon[i + 2].m_pos;
			wireCallback.polygon[i * 2 + 5].m_pos = callback.polygon[i + 0].m_pos;
		}
	}
	wireCallback.fillMode = Tr2RenderContextEnum::FM_SOLID;
	wireCallback.primitiveType = TOP_LINES;

	callback.fillMode = Tr2RenderContextEnum::FM_SOLID;
	callback.primitiveType = TOP_TRIANGLES;

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
	renderContext.m_esm.ApplyVertexDeclaration( g_debugResourceHelper.GetVertexPosColorDecl() );

	unsigned color;
	color = ( unsigned( std::min( std::max( baseColor.b, 0.0f ), 1.0f ) * 6.f ) << 16 ) | 
		( unsigned( std::min( std::max( baseColor.g, 0.0f ), 1.0f ) * 6.f ) << 8 ) | 
		unsigned( std::min( std::max( baseColor.r, 0.0f ), 1.0f ) * 6.f );
	for( unsigned int i = 0; i < callback.vertexCount; ++i )
	{
		callback.polygon[i].m_color = color;
	}
	callback.zFunc = CMP_GREATER;
	g_debugResourceHelper.GetEffect()->Render( &callback, renderContext );

	color = ( unsigned( std::min( std::max( baseColor.b, 0.0f ), 1.0f ) * 16.f ) << 16 ) | 
		( unsigned( std::min( std::max( baseColor.g, 0.0f ), 1.0f ) * 16.f ) << 8 ) | 
		unsigned( std::min( std::max( baseColor.r, 0.0f ), 1.0f ) * 16.f );
	for( unsigned int i = 0; i < callback.vertexCount; ++i )
	{
		callback.polygon[i].m_color = color;
	}
	callback.zFunc = CMP_LESSEQUAL;
	g_debugResourceHelper.GetEffect()->Render( &callback, renderContext );

	for( unsigned int i = 0; i < wireCallback.vertexCount; ++i )
	{
		wireCallback.polygon[i].m_color = 0x00101010;
	}
	wireCallback.zFunc = CMP_GREATER;
	g_debugResourceHelper.GetEffect()->Render( &wireCallback, renderContext );

	for( unsigned int i = 0; i < wireCallback.vertexCount; ++i )
	{
		wireCallback.polygon[i].m_color = 0x00444444;
	}
	wireCallback.zFunc = CMP_LESSEQUAL;
	g_debugResourceHelper.GetEffect()->Render( &wireCallback, renderContext );

	delete[] callback.polygon;
	delete[] wireCallback.polygon;
}

#endif
