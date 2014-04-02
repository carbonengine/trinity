////////////////////////////////////////////////////////////
//
//    Created:   November 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorDirectionalLight.h"

#include "umbraTypes.h"

#include "Tr2InteriorLightGeometryRenderBatch.h"
#include "Tr2InteriorCell.h"
#include "Tr2AtlasTexture.h"
#include "Tr2KelvinColor.h"
#include "ITr2UmbraUserData.h"
#include "Tr2ShaderMaterial.h"
#include "Tr2InteriorLightSource.h"
#include "Curves/TriCurveSet.h"
#include "Tr2InteriorConstantBufferFormats.h"
#include "IRenderCallback.h"
#include "TriDebugResourceHelper.h"

CCP_STATS_DECLARED_ELSEWHERE( wodIntLightsAlive );

using namespace Tr2RenderContextEnum;
	
// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorDirectionalLight default constructor
// --------------------------------------------------------------------------------------
Tr2InteriorDirectionalLight::Tr2InteriorDirectionalLight( IRoot* lockobj ) :
	m_name(),
	m_direction( 0.f, -1.f, 0.f ),
	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_specularIntensity( 1.f ),
	m_primaryLighting( true ),
	m_secondaryLighting( true ),
	m_secondaryLightingMultiplier( 1.0f ),
	m_affectTransparentObjects( true ),
	m_enableROIs( true ),
	m_shadowImportance( 1.f ),
	m_useExplicitBounds( false ),
	m_explicitBoundsMin( -1.f, -1.f, -1.f ),
	m_explicitBoundsMax( 1.f, 1.f, 1.f ),
	m_isDirty( true ),
	m_isStatic( true ),
	m_staticFlagChanged( true ),
	m_umbraModel( NULL ),
	m_umbraRoiList(),
	m_lightVertexDecl( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_shadowCasterTypes( ST_ALL ),
	m_shadowResolution( 256 ),
	m_shadowLODs( 1 ),
	m_LODBlendRegion( 0.2f ),
	m_debugShadowLODs( false ),
	m_LODDistribution( 0.f ),
	m_renderDebugInfo( false ),
	m_renderDebugType( DI_LIGHT_COLOR ),
	PARENTLOCK( m_curveSets )
{
	CCP_STATS_INC( wodIntLightsAlive );

	for( int i = 0; i < 6; ++i )
	{
		m_dirtyShadow[i] = true;
		m_emptyShadow[i] = false;
		m_framesSinceShadowUpdate[i] = 0;
	}
	m_shadowRect[0] = m_newShadowRect[0] = Vector4( 0.f, 0.f, 1.f, 1.f );

	m_material.CreateInstance();
	m_shMaterial.CreateInstance();
	m_lightmapMaterial.CreateInstance();
	
	m_material->SetHighLevelShaderName( "DirectionalLight" );
	m_shMaterial->SetHighLevelShaderName( "DirectionalLight" );
	m_lightmapMaterial->SetHighLevelShaderName( "DirectionalLight" );

	m_kelvinColor.CreateInstance();
	m_useKelvinColor = false;

	PrepareResources();

	RebuildVolume();
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorDirectionalLight destructor.
// --------------------------------------------------------------------------------------
Tr2InteriorDirectionalLight::~Tr2InteriorDirectionalLight()
{
	// Clear Umbra data
	ClearUmbra();

	CCP_STATS_DEC( wodIntLightsAlive );
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from IInitialize interface.  Builds the Umbra culling volume after object 
//   initialization.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InteriorDirectionalLight::Initialize()
{
	RebuildVolume();

	TransformModified();
	ChooseLightEffect();

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
bool Tr2InteriorDirectionalLight::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_direction ) )
	{
		TransformModified();
	}
	else if( IsMatch( value, m_shadowImportance ) )
	{
		// If shadowImportance goes to 0 - destroy the shadow map
		UpdateShadowMap();
	}
	else if( IsMatch( value, m_primaryLighting ) )
	{
		UpdateShadowMap();
	}
	else if( IsMatch( value, m_shadowLODs ) )
	{
		for( int i = 1; i < 6; ++i )
		{
			m_dirtyShadow[i] = true;
			m_emptyShadow[i] = false;
		}
		ChooseLightEffect();
	}
	else if( IsMatch( value, m_useExplicitBounds ) )
	{
		RebuildBounds();
	}
	else if( IsMatch( value, m_explicitBoundsMin ) )
	{
		if( m_useExplicitBounds )
		{
			RebuildBounds();
		}
	}
	else if( IsMatch( value, m_explicitBoundsMax ) )
	{
		if( m_useExplicitBounds )
		{
			RebuildBounds();
		}
	}
	else if( IsMatch( value, m_LODDistribution ) )
	{
		for( int i = 1; i < 6; ++i )
		{
			m_dirtyShadow[i] = true;
			m_emptyShadow[i] = false;
		}
	}
	else if( IsMatch( value, m_shadowResolution ) )
	{
		for( int i = 0; i < 6; ++i )
		{
			m_dirtyShadow[i] = true;
			m_emptyShadow[i] = false;
		}
	}
	else if( IsMatch( value, m_shadowCasterTypes ) )
	{
		for( int i = 0; i < 6; ++i )
		{
			m_dirtyShadow[i] = true;
			m_emptyShadow[i] = false;
		}
		UpdateShadowMap();
		ChooseLightEffect();
	}
	else if( IsMatch( value, m_affectTransparentObjects ) )
	{
		for( std::vector<Umbra::RegionOfInfluence*>::iterator it = m_umbraRoiList.begin(); it != m_umbraRoiList.end(); ++it )
		{
			( *it )->set( Umbra::Object::ENABLED, m_affectTransparentObjects && m_enableROIs );
		}
	}
	else if( IsMatch( value, m_LODBlendRegion ) )
	{
		ChooseLightEffect();
	}
	else if( IsMatch( value, m_debugShadowLODs ) )
	{
		ChooseLightEffect();
	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Invalidates light's geometry vertex 
//   declaration and shadow maps.
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::ReleaseResources( TriStorage s )
{
	m_lightVertexDecl = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	for( int i = 0; i < 6; ++i )
	{
		m_shadowTextures[i] = NULL;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Re-creates light's geometry vertex 
//   declaration and shadow map.
// --------------------------------------------------------------------------------------
bool Tr2InteriorDirectionalLight::OnPrepareResources()
{
	Tr2VertexDefinition vd;
	vd.Add( vd.FLOAT32_4, vd.POSITION );

	UpdateShadowMap();

	m_lightVertexDecl = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Copies the lighting parameters into the per-object data.
// Arguments:
//   lightData - The per-object light data to populate
//   mirrorToWorldMatrix - Mirror to world space transform
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::PopulateLightData( Tr2InteriorPerObjectLightData* lightData, const Matrix &mirrorToWorldMatrix ) const
{
	Matrix mirrorMatrix;
	D3DXMatrixTranspose( &mirrorMatrix, &mirrorToWorldMatrix );
	D3DXMatrixInverse( &mirrorMatrix, NULL, &mirrorMatrix );

	Vector4 direction( -m_direction.x, -m_direction.y, -m_direction.z, 0.f );
	D3DXVec4Transform( reinterpret_cast<Vector4*>( &lightData->spotDirection ), &direction, &mirrorMatrix );

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

	static const float shadowDepthCorrection = 1.f;
	lightData->radius = 1.f / std::max( 1.f, m_maxBounds.z - m_minBounds.z ) * shadowDepthCorrection;

	lightData->pointLightFalloff = m_LODBlendRegion * 0.5f;

	// These are used to distinguish directional light in forward rendering
	lightData->coneCosAlphaOuter = 100.f;
	lightData->coneCosAlphaInner = -100.f;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the world-space axis aligned bounding box for the light
// Arguments:
//   minBounds - The min bounds of the BB
//   maxBounds - The max bounds of the BB
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::GetBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
{
	minBounds = Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	maxBounds = Vector3( FLT_MAX, FLT_MAX, FLT_MAX );
}

void Tr2InteriorDirectionalLight::GetLocalBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
{
	minBounds = m_minBounds;
	maxBounds = m_maxBounds;
}

// ---------------------------------------------------------------------------------------
// Description:
//   Sets the value of the isStatic flags (which controls whether or not the light is
//   cached in Enlighten).
// Arguments:
//   isStatic - The new value of the isStatic flag
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::SetStatic( bool isStatic )
{
	if( isStatic != m_isStatic )
		m_staticFlagChanged = true;
	m_isStatic = isStatic;
}

// --------------------------------------------------------------------------------------
// Description:
//   Always tries to add a light to the cell. Also resizes shadow frustum to fit the
//   cell.
// Arguments:
//   cell - The cell to test against
// --------------------------------------------------------------------------------------
bool Tr2InteriorDirectionalLight::TestCellIntersectionAndAdd( Tr2InteriorCell* cell )
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

	m_cellBounds[cell] = std::make_pair( minBounds, maxBounds );

	// Only do the ROI setup if the cell doesn't already contain this light
	if( cell->AddLight( this ) )
	{
		// See if we already have an ROI for this cell
		Umbra::RegionOfInfluence* roi = Umbra::RegionOfInfluence::create( m_umbraModel );
		roi->setCell( cell->GetUmbraCell() );

		// Set our own userdata to this ROI
		roi->setUserPointer( CONVERT_TO_UMBRA_USER_DATA( this ) );
		roi->set( Umbra::Object::ENABLED, m_affectTransparentObjects && m_enableROIs );
		roi->set( Umbra::Object::UNBOUNDED, true );

		// Store the roi in our list
		m_umbraRoiList.push_back( roi );

		Umbra::Object* obj = Umbra::Object::create( m_umbraModel );
		obj->setCell( cell->GetUmbraCell() );
		obj->set( Umbra::Object::UNBOUNDED, true );

		obj->setUserPointer( CONVERT_TO_UMBRA_USER_DATA( this ) );

		m_umbraObjectList.push_back( obj );
	}

	RebuildBounds();

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Rebuilds bounding box for shadow LOD0 frustum.
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::RebuildBounds()
{
	if( m_useExplicitBounds )
	{
		m_minBounds = m_explicitBoundsMin;
		m_maxBounds = m_explicitBoundsMax;
	}
	else
	{
		m_minBounds = Vector3( FLT_MAX, FLT_MAX, FLT_MAX );
		m_maxBounds = Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX );
		for( std::map<Tr2InteriorCell*, std::pair<Vector3, Vector3>>::iterator it = m_cellBounds.begin(); 
			it != m_cellBounds.end(); ++it )
		{
			Vector3 minBounds = it->second.first;
			Vector3 maxBounds = it->second.second;
			BoundingBoxTransform( minBounds, maxBounds, m_transformInv );
			BoundingBoxUpdate( m_minBounds, m_maxBounds, minBounds, maxBounds );
		}
		Vector2 size( m_maxBounds.x - m_minBounds.x, m_maxBounds.y - m_minBounds.y );
		if( size.x > size.y )
		{
			float dy = ( size.x - size.y ) * 0.5f;
			m_minBounds.y -= dy;
			m_maxBounds.y += dy;
			size.y = size.x;
		}
		else
		{
			float dx = ( size.y - size.x ) * 0.5f;
			m_minBounds.x -= dx;
			m_maxBounds.x += dx;
			size.x = size.y;
		}
	}
	Vector2 size( m_maxBounds.x - m_minBounds.x, m_maxBounds.y - m_minBounds.y );

	const float nearPlane = 1.f;
 
	Vector3 eye( ( m_minBounds.x + m_maxBounds.x ) * 0.5f, ( m_minBounds.y + m_maxBounds.y ) * 0.5f, m_maxBounds.z + nearPlane );

	D3DXMatrixTranslation( &m_viewMatrix[0], -eye.x, -eye.y, -eye.z );
	m_viewMatrix[0] = m_transformInv * m_viewMatrix[0];
	D3DXMatrixInverse( &m_invViewMatrix[0], NULL, &m_viewMatrix[0] );
	D3DXMatrixOrthoRH( m_projectionMatrix, size.x, size.y, nearPlane, m_maxBounds.z - m_minBounds.z + nearPlane );

	for( unsigned int i = 0; i < GetRequiredShadowMapCount(); ++i )
	{
		MarkShadowDirty( i, true );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the importance of the light, given the current view position. For directional
//   light there is no sencible view-dependant importance, so we always return 1.
// Arguments:
//   viewerPos - The position of the viewpoint
// Return Value:
//   Always 1.
// --------------------------------------------------------------------------------------
float Tr2InteriorDirectionalLight::GetCurrentViewImportance( const Vector3& viewerPos ) const
{
	return 1.f;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the importance of the light's shadow, given the current view position. 
//   The importance is given by shadowImportance attribute.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   viewerPos - The position of the viewpoint
// Return Value:
//   The view importance
// --------------------------------------------------------------------------------------
float Tr2InteriorDirectionalLight::GetCurrentShadowImportance( unsigned int shadowMapIndex, const Vector3& viewerPos ) const
{
	return GetCurrentViewImportance( viewerPos ) * m_shadowImportance * m_shadowImportance;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Called whenever light is added to interior scene.
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::AddToScene( void )
{
	// Set the dirty flag
	m_isDirty = true;
	for( int i = 0; i < 6; ++i )
	{
		m_dirtyShadow[i] = true;
		m_emptyShadow[i] = false;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Called whenever light is removed from interior scene.
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::RemoveFromScene( void )
{
	ClearUmbra();
	for( int i = 0; i < 6; ++i )
	{
		m_shadowTextures[i] = NULL;
	}
	m_cellBounds.clear();
}

// -------------------------------------------------------------
// Description:
//   Called whenever a cell is removed from the scene. Resizes
//   shadow frustum to fit new scene bounds.
// Arguments:
//   cell - Cell that is removed from the scene
// -------------------------------------------------------------
void Tr2InteriorDirectionalLight::CellRemoved( Tr2InteriorCell* cell )
{
	m_cellBounds.erase( cell );
	if( cell->IsUnbounded() )
	{
		for( int i = 0; i < 6; ++i )
		{
			m_dirtyShadow[i] = true;
		}
	}
	else
	{
		Vector3 minBounds, maxBounds;
		if( !cell->GetBoundingBox( minBounds, maxBounds ) )
		{
			return;
		}
		MarkShadowsDirtyForBounds( minBounds, maxBounds );
	}
}

// -------------------------------------------------------------
// Description:
//   Render light source debug information.
// -------------------------------------------------------------
void Tr2InteriorDirectionalLight::RenderDebugInfo( Tr2RenderContext& renderContext ) const
{
	if( !m_renderDebugInfo )
	{
		return;
	}
	if( m_renderDebugType == DI_SHADOW_RESOLUTION || m_renderDebugType == DI_SHADOW_RELATIVE_RESOLUTION )
	{
		return;
	}



	struct EffectCallback: public IRenderCallback
	{
		TriDebugResourceHelper::VertexPosColor polygon[8];
		unsigned int vertexCount;
		unsigned int triangleCount;
		const unsigned int *indices;
		uint32_t zFunc;
		uint32_t fillMode;
		Tr2RenderContextEnum::Topology primitiveType;

		void SubmitGeometry( Tr2RenderContext& renderContext )
		{
			renderContext.SetRenderState( RS_ZENABLE, TRUE );
			renderContext.SetRenderState( RS_ZWRITEENABLE, FALSE );
			renderContext.SetRenderState( RS_ZFUNC, zFunc );
			renderContext.SetRenderState( RS_FILLMODE, fillMode );
			uint32_t stride = sizeof( TriDebugResourceHelper::VertexPosColor );
			renderContext.SetTopology( primitiveType );
			renderContext.DrawIndexedPrimitiveUP( vertexCount, triangleCount, indices, polygon, stride );
		}
	};
	EffectCallback callback, wireCallback;

	static unsigned int const unitCubeIndices[] =
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
	static unsigned int const unitCubeWireIndices[] =
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
        Vector4( m_minBounds.x,  m_maxBounds.y,  m_maxBounds.z, 1.0f ),
        Vector4( m_maxBounds.x,  m_maxBounds.y,  m_maxBounds.z, 1.0f ),
        Vector4( m_minBounds.x,  m_minBounds.y,  m_maxBounds.z, 1.0f ),
        Vector4( m_maxBounds.x,  m_minBounds.y,  m_maxBounds.z, 1.0f ),
        Vector4( m_minBounds.x,  m_maxBounds.y,  m_minBounds.z, 1.0f ),
        Vector4( m_maxBounds.x,  m_maxBounds.y,  m_minBounds.z, 1.0f ),
        Vector4( m_minBounds.x,  m_minBounds.y,  m_minBounds.z, 1.0f ),
        Vector4( m_maxBounds.x,  m_minBounds.y,  m_minBounds.z, 1.0f ),
    };

	
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

	RenderDebugLightVolume(
		reinterpret_cast<const Vector3*>( unitCubeVertices ),
		8,
		sizeof( Vector4 ),
		unitCubeIndices,
		12,
		unitCubeWireIndices,
		12,
		m_transform,
		color,
		renderContext
		);
}

// -------------------------------------------------------------
// Description:
//   Per-frame update method. Updates curve sets.
// Arguments:
//   time - Current system time.
// -------------------------------------------------------------
void Tr2InteriorDirectionalLight::Update( Be::Time time )
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
void Tr2InteriorDirectionalLight::EnableROI( bool enable )
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
//   Sets a shadow map texture.
// --------------------------------------------------------------------------------------
class Tr2InteriorDirectionalLight::PerLightData : public Tr2ShadowPerLightData<Tr2PerObjectDataStandard>
{
};

// --------------------------------------------------------------------------------------
// Description:
//   Adds light's geometry to a render batch accumulator. Used during light accumulation 
//   pass.
// Arguments:
//   batches - Render batch accumulator to add batch to
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::GetBatches( ITriRenderBatchAccumulator* batches )
{
	PerLightData* data = batches->Allocate<PerLightData>();

	if( !data )
	{
		return;
	}

	data->SetShadowTexture( m_shadowTextures, 6 );
	
	Tr2InteriorPerLightPSData perObjectPSBuffer;

	PopulateLightData( &perObjectPSBuffer.lightData, Tr2Renderer::GetIdentityTransform() );
	perObjectPSBuffer.additionalParameters.x = m_specularIntensity;

	for( int  i = 0; i < 6; ++i )
	{
		int nextShadow = 0;
		if( m_shadowTextures[i] )
		{
			perObjectPSBuffer.shadowRect[i].x = float( m_shadowTextures[i]->GetX() + 0.5f ) / m_shadowTextures[i]->GetTextureWidth();
			perObjectPSBuffer.shadowRect[i].y = float( m_shadowTextures[i]->GetY() + 0.5f ) / m_shadowTextures[i]->GetTextureHeight();
			perObjectPSBuffer.shadowRect[i].z = float( m_shadowTextures[i]->GetWidth() - 1.0f ) / m_shadowTextures[i]->GetTextureWidth();
			perObjectPSBuffer.shadowRect[i].w = float( m_shadowTextures[i]->GetHeight() - 1.0f ) / m_shadowTextures[i]->GetTextureHeight();
			for( int j = i - 1; j >= 0; --j )
			{
				if( m_shadowTextures[j] )
				{
					nextShadow = j;
					break;
				}
			}
		}
		else
		{
			perObjectPSBuffer.shadowRect[i] = Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
		}
		perObjectPSBuffer.shadowMatrix[i] = Matrix(
			m_shadowRect[i].x, m_shadowRect[i].x, 0.f, 0.f, 
			m_shadowRect[i].y, m_shadowRect[i].y, 0.f, 0.f,  
			m_shadowRect[i].z, 1.f / ( m_shadowRect[i].z - m_shadowRect[i].x ), 0.f, 0.f, 
			m_shadowRect[i].w, 1.f / ( m_shadowRect[i].w - m_shadowRect[i].y ), 0.f, float( nextShadow ) );
	}

	{
		Matrix texAdjust( 0.5f, 0.0f, 0.0f, 0.0f,
						  0.0f, -0.5f, 0.0f, 0.0f,
						  0.0f, 0.0f, 1.0f, 0.0f,
						  0.5f, 0.5f, 0.0f, 1.0f );
		perObjectPSBuffer.mirrorToWorldMatrix = GetViewMatrix( 0 ) * m_projectionMatrix[0] * texAdjust;
		D3DXMatrixTranspose( &perObjectPSBuffer.mirrorToWorldMatrix, &perObjectPSBuffer.mirrorToWorldMatrix );
	}

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	Tr2InteriorFullScreenLightBatch* batch = batches->Allocate<Tr2InteriorFullScreenLightBatch>();
	if( batch )
	{
		batch->SetVertexDeclaration( m_lightVertexDecl );
		batch->SetShaderMaterial( m_material );
		batch->SetPerObjectData( data );

		batches->Commit( batch );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds light's geometry to a render batch accumulator. Used during light accumulation 
//   pass.
// Arguments:
//   batches - Render batch accumulator to add batch to
//   mirrorToWorldMatrix - Mirror to world space transform.
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::GetInstancedBatches( ITriRenderBatchAccumulator* batches,
												  const Matrix& mirrorToWorldMatrix )
{
	PerLightData* data = batches->Allocate<PerLightData>();

	if( !data )
	{
		return;
	}

	data->SetShadowTexture( m_shadowTextures, 6 );

	Tr2InteriorPerLightPSData perObjectPSBuffer;

	PopulateLightData( &perObjectPSBuffer.lightData, mirrorToWorldMatrix );
	perObjectPSBuffer.additionalParameters.x = m_specularIntensity;

	for( int  i = 0; i < 6; ++i )
	{
		int nextShadow = 0;
		if( m_shadowTextures[i] )
		{
			perObjectPSBuffer.shadowRect[i].x = float( m_shadowTextures[i]->GetX() + 0.5f ) / m_shadowTextures[i]->GetTextureWidth();
			perObjectPSBuffer.shadowRect[i].y = float( m_shadowTextures[i]->GetY() + 0.5f ) / m_shadowTextures[i]->GetTextureHeight();
			perObjectPSBuffer.shadowRect[i].z = float( m_shadowTextures[i]->GetWidth() - 1.0f ) / m_shadowTextures[i]->GetTextureWidth();
			perObjectPSBuffer.shadowRect[i].w = float( m_shadowTextures[i]->GetHeight() - 1.0f ) / m_shadowTextures[i]->GetTextureHeight();
			for( int j = i - 1; j >= 0; --j )
			{
				if( m_shadowTextures[j] )
				{
					nextShadow = j;
					break;
				}
			}
		}
		else
		{
			perObjectPSBuffer.shadowRect[i] = Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
		}
		perObjectPSBuffer.shadowMatrix[i] = Matrix(
			m_shadowRect[i].x, m_shadowRect[i].x, 0.f, 0.f, 
			m_shadowRect[i].y, m_shadowRect[i].y, 0.f, 0.f,  
			m_shadowRect[i].z, 1.f / ( m_shadowRect[i].z - m_shadowRect[i].x ), 0.f, 0.f, 
			m_shadowRect[i].w, 1.f / ( m_shadowRect[i].w - m_shadowRect[i].y ), 0.f, float( nextShadow ) );
	}

	{
		Matrix texAdjust( 0.5f, 0.0f, 0.0f, 0.0f,
						  0.0f, -0.5f, 0.0f, 0.0f,
						  0.0f, 0.0f, 1.0f, 0.0f,
						  0.5f, 0.5f, 0.0f, 1.0f );
		D3DXMatrixTranspose( &perObjectPSBuffer.mirrorToWorldMatrix, &mirrorToWorldMatrix );
		perObjectPSBuffer.mirrorToWorldMatrix = perObjectPSBuffer.mirrorToWorldMatrix * GetViewMatrix( 0 ) * m_projectionMatrix[0] * texAdjust;
		D3DXMatrixTranspose( &perObjectPSBuffer.mirrorToWorldMatrix, &perObjectPSBuffer.mirrorToWorldMatrix );
	}

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	Tr2InteriorFullScreenLightBatch* batch = batches->Allocate<Tr2InteriorFullScreenLightBatch>();
	if( batch )
	{
		batch->SetVertexDeclaration( m_lightVertexDecl );
		batch->SetShaderMaterial( m_material );
		batch->SetPerObjectData( data );

		batches->Commit( batch );
	}
}

// -------------------------------------------------------------
// Description:
//   Returns the number of shadow map textures in shadow atlas
//   required for this light source. This is the number of
//   shadow LODs or 0 if the light does not cast shadows.
// Return Value:
//   number of shadow maps required for this light
// -------------------------------------------------------------
unsigned int Tr2InteriorDirectionalLight::GetRequiredShadowMapCount() const
{
	if( UseWithPrimaryLighting() && m_shadowImportance > 0.0f && m_shadowCasterTypes != ST_NONE )
	{
		return  m_shadowLODs;
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
void Tr2InteriorDirectionalLight::GetRequiredShadowMapResolution( unsigned int index, bool maximumResolution, unsigned int& width, unsigned int& height ) const
{
	width = height = m_shadowResolution;
}

// -------------------------------------------------------------
// Description:
//   Returns the size of the shadow LOD.
// Arguments:
//   shadowIndex - Shadow LOD index
// Return Value:
//   size of the shadow LOD
// -------------------------------------------------------------
float Tr2InteriorDirectionalLight::GetShadowLODSize( unsigned shadowIndex ) const
{
	float fraction = float( m_shadowLODs - shadowIndex ) / m_shadowLODs;
	float size = m_maxBounds.x - m_minBounds.x;
	
	return( pow( size, fraction ) * m_LODDistribution + size * fraction * ( 1.f - m_LODDistribution ) )  * 0.5f;
}

// -------------------------------------------------------------
// Description:
//   Recalculates shadow LOD positions based on viewer position 
//   and marks changed LODs as dirty.
// -------------------------------------------------------------
void Tr2InteriorDirectionalLight::CacheShadowMapResolution()
{
	Vector3 eye = Tr2Renderer::GetViewPosition();
	D3DXVec3TransformCoord( &eye, &eye, &m_transformInv );
	Vector2 size( m_maxBounds.x - m_minBounds.x, m_maxBounds.y - m_minBounds.y );
	for( unsigned int i = 1; i < m_shadowLODs; ++i )
	{
		float currentSize = GetShadowLODSize( i );

		float pixelSize = currentSize / m_shadowResolution * 2.f;
		Vector3 localEye = eye;
		localEye.x = floor( localEye.x / pixelSize ) * pixelSize;
		localEye.y = floor( localEye.y / pixelSize ) * pixelSize;

		Vector2 minBounds, maxBounds;
		if( localEye.x - currentSize < m_minBounds.x )
		{
			minBounds.x = m_minBounds.x;
		}
		else
		{
			minBounds.x = localEye.x - currentSize;
		}
		if( localEye.y - currentSize < m_minBounds.y )
		{
			minBounds.y = m_minBounds.y;
		}
		else
		{
			minBounds.y = localEye.y - currentSize;
		}
		if( localEye.x + currentSize > m_maxBounds.x )
		{
			maxBounds.x = m_maxBounds.x;
			minBounds.x = maxBounds.x - 2.f * currentSize;
		}
		else
		{
			maxBounds.x = minBounds.x + 2.f * currentSize;
		}
		if( localEye.y + currentSize > m_maxBounds.y )
		{
			maxBounds.y = m_maxBounds.y;
			minBounds.y = maxBounds.y - 2.f * currentSize;
		}
		else
		{
			maxBounds.y = minBounds.y + 2.f * currentSize;
		}

		const float nearPlane = 0.1f;
 
		Vector3 eye( ( minBounds.x + maxBounds.x ) * 0.5f, ( minBounds.y + maxBounds.y ) * 0.5f, m_maxBounds.z + nearPlane );
		D3DXMatrixTranslation( &m_viewMatrix[i], -eye.x, -eye.y, -eye.z );
		m_viewMatrix[i] = m_transformInv * m_viewMatrix[i];
		D3DXMatrixInverse( &m_invViewMatrix[i], NULL, &m_viewMatrix[i] );
		D3DXMatrixOrthoRH( &m_projectionMatrix[i], maxBounds.x - minBounds.x, maxBounds.y - minBounds.y, nearPlane, m_maxBounds.z - m_minBounds.z + nearPlane );
	
		float prevX = m_shadowRect[i].x * size.x + m_minBounds.x;
		float prevY = ( 1 - m_shadowRect[i].w ) * size.y + m_minBounds.y;
		if( abs( prevX - minBounds.x ) > pixelSize || abs( prevY - minBounds.y ) > pixelSize )
		{
			m_newShadowRect[i].x = ( minBounds.x - m_minBounds.x ) / size.x;
			m_newShadowRect[i].y = 1-( maxBounds.y - m_minBounds.y ) / size.y;
			m_newShadowRect[i].z = ( maxBounds.x - m_minBounds.x ) / size.x;
			m_newShadowRect[i].w = 1-( minBounds.y - m_minBounds.y ) / size.y;

			MarkShadowDirty( i, true );
		}
		else
		{
			m_newShadowRect[i] = m_shadowRect[i];
		}
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
//   Atlas texture containing the given shadow map
// -------------------------------------------------------------
Tr2AtlasTexture* Tr2InteriorDirectionalLight::GetShadowAtlasTexture( unsigned int index ) const
{
	return m_shadowTextures[index];
}

// -------------------------------------------------------------
// Description:
//   Assigns atlas texture for the given shadow map. 
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
//   texture - Atlas texture containing the shadow map
// -------------------------------------------------------------
void Tr2InteriorDirectionalLight::SetShadowAtlasTexture( unsigned int index, Tr2AtlasTexture* texture )
{
	bool rebind = ( m_shadowTextures[index] == NULL ) != ( texture == NULL );
	m_shadowTextures[index] = texture;
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
//   emptyShadow - Flag indicating that the shadow map is empty.
// -------------------------------------------------------------
void Tr2InteriorDirectionalLight::SetEmptyShadow( unsigned int shadowMapIndex, bool emptyShadow )
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
bool Tr2InteriorDirectionalLight::IsShadowEmpty( unsigned int shadowMapIndex ) const
{
	return m_emptyShadow[shadowMapIndex];
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
//   false always
// --------------------------------------------------------------------------------------
bool Tr2InteriorDirectionalLight::BeginShadowUpdate( unsigned int shadowMapIndex, Tr2PerFrameVSData* vsData, Tr2PerFrameShadowPSData* psData, Tr2RenderContext& renderContext )
{
	D3DPERF_EVENT( L"Tr2InteriorDirectionalLight::BeginShadowRendering" );

	Tr2Renderer::SetViewTransform( m_viewMatrix[shadowMapIndex] );
	Tr2Renderer::SetProjectionTransform( m_projectionMatrix[shadowMapIndex] );

	// pre-mults
	Matrix viewProjMat = m_viewMatrix[shadowMapIndex] * m_projectionMatrix[shadowMapIndex];

	// column_major for shaders
	D3DXMatrixTranspose( &vsData->ViewMat, &m_viewMatrix[shadowMapIndex] );
	D3DXMatrixTranspose( &vsData->ProjectionMat, &m_projectionMatrix[shadowMapIndex] );
	D3DXMatrixTranspose( &vsData->ViewProjectionMat, &viewProjMat );
	D3DXMatrixInverse( &vsData->ViewInverseTransposeMat, NULL, &m_viewMatrix[shadowMapIndex] );

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
void Tr2InteriorDirectionalLight::EndShadowUpdate( unsigned int shadowMapIndex )
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
const Matrix& Tr2InteriorDirectionalLight::GetViewMatrix( unsigned int shadowMapIndex ) const
{
	return m_viewMatrix[shadowMapIndex];
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns inverse of spotlight view matrix.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Identity matrix
// --------------------------------------------------------------------------------------
const Matrix& Tr2InteriorDirectionalLight::GetInvViewMatrix( unsigned int shadowMapIndex ) const
{
	return m_invViewMatrix[shadowMapIndex];
}

// -------------------------------------------------------------
// Description:
//   Returns projection transform matrix for shadow map.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Identity matrix.
// -------------------------------------------------------------
const Matrix& Tr2InteriorDirectionalLight::GetProjectionMatrix( unsigned int shadowMapIndex ) const
{
	return m_projectionMatrix[shadowMapIndex];
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
void Tr2InteriorDirectionalLight::MarkShadowsDirtyForBounds( const Vector3 &minBounds, const Vector3 &maxBounds )
{
	MarkShadowDirty( 0, true );
	Vector3 min = minBounds;
	Vector3 max = maxBounds;
	BoundingBoxTransform( min, max, m_viewMatrix[0] );
	BoundingBoxTransform( min, max, m_projectionMatrix[0] );
	min.x = min.x * 0.5f + 0.5f;
	min.y = -min.y * 0.5f + 0.5f;
	max.x = max.x * 0.5f + 0.5f; 
	max.y = -max.y * 0.5f + 0.5f;
	for( unsigned int i = 0; i < m_shadowLODs; ++i )
	{
		if( min.x > m_shadowRect[i].z || min.y > m_shadowRect[i].w ||
			max.x < m_shadowRect[i].x || max.y < m_shadowRect[i].y )
		{
			break;
		}
		MarkShadowDirty( i, true );
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
void Tr2InteriorDirectionalLight::MarkShadowDirty( unsigned int shadowMapIndex, bool dirty )
{
	m_dirtyShadow[shadowMapIndex] = dirty;
	if( dirty )
	{
		m_emptyShadow[shadowMapIndex] = false;
	}
	else
	{
		m_shadowRect[shadowMapIndex] = m_newShadowRect[shadowMapIndex];
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a flag indicating if the spotlight shadow is dirty.
// Arguments:
//   shadowMapIndex - Index of shadow map (for multi-shadow lights)
// Return Value:
//   Shadow dirty flag
// --------------------------------------------------------------------------------------
bool Tr2InteriorDirectionalLight::IsShadowDirty( unsigned int shadowMapIndex ) const
{
	return m_dirtyShadow[shadowMapIndex];
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns type of objects rendered into the shadow map.
// Return Value:
//   Type of objects rendered into the shadow map
// --------------------------------------------------------------------------------------
Tr2InteriorDirectionalLight::ShadowCasterTypes Tr2InteriorDirectionalLight::GetShadowCasterTypes() const
{
	return m_shadowCasterTypes;
}

// -------------------------------------------------------------
// Description:
//   Get batches for a light to use during SH lighting step.
// Arguments:
//   batches - Batch accumulator to add batches to.
// -------------------------------------------------------------
void Tr2InteriorDirectionalLight::GetSHBatches( ITriRenderBatchAccumulator* batches ) const
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

	data->SetShadowTexture( m_shadowTextures, 6 );

	Tr2InteriorPerLightPSData perObjectPSBuffer;

	PopulateLightData( &perObjectPSBuffer.lightData, Tr2Renderer::GetIdentityTransform() );
	perObjectPSBuffer.additionalParameters.x = m_specularIntensity;

	for( int  i = 0; i < 6; ++i )
	{
		int nextShadow = 0;
		if( m_shadowTextures[i] )
		{
			perObjectPSBuffer.shadowRect[i].x = float( m_shadowTextures[i]->GetX() + 0.5f ) / m_shadowTextures[i]->GetTextureWidth();
			perObjectPSBuffer.shadowRect[i].y = float( m_shadowTextures[i]->GetY() + 0.5f ) / m_shadowTextures[i]->GetTextureHeight();
			perObjectPSBuffer.shadowRect[i].z = float( m_shadowTextures[i]->GetWidth() - 1.0f ) / m_shadowTextures[i]->GetTextureWidth();
			perObjectPSBuffer.shadowRect[i].w = float( m_shadowTextures[i]->GetHeight() - 1.0f ) / m_shadowTextures[i]->GetTextureHeight();
			for( int j = i - 1; j >= 0; --j )
			{
				if( m_shadowTextures[j] )
				{
					nextShadow = j;
					break;
				}
			}
		}
		else
		{
			perObjectPSBuffer.shadowRect[i] = Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
		}
		perObjectPSBuffer.shadowMatrix[i] = Matrix(
			m_shadowRect[i].x, m_shadowRect[i].x, 0.f, 0.f, 
			m_shadowRect[i].y, m_shadowRect[i].y, 0.f, 0.f,  
			m_shadowRect[i].z, 1.f / ( m_shadowRect[i].z - m_shadowRect[i].x ), 0.f, 0.f, 
			m_shadowRect[i].w, 1.f / ( m_shadowRect[i].w - m_shadowRect[i].y ), 0.f, float( nextShadow ) );
	}

	{
		Matrix texAdjust( 0.5f, 0.0f, 0.0f, 0.0f,
						  0.0f, -0.5f, 0.0f, 0.0f,
						  0.0f, 0.0f, 1.0f, 0.0f,
						  0.5f, 0.5f, 0.0f, 1.0f );
		perObjectPSBuffer.mirrorToWorldMatrix = GetViewMatrix( 0 ) * m_projectionMatrix[0] * texAdjust;
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
//   Get batches for a lightmap rendering.
// Arguments:
//   batches - Batch accumulator to add batches to.
// -------------------------------------------------------------
void Tr2InteriorDirectionalLight::GetLightMapBatches( ITriRenderBatchAccumulator* batches ) const
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

	data->SetShadowTexture( m_shadowTextures, 6 );

	Tr2InteriorPerLightPSData perObjectPSBuffer;

	PopulateLightData( &perObjectPSBuffer.lightData, Tr2Renderer::GetIdentityTransform() );
	perObjectPSBuffer.additionalParameters.x = m_specularIntensity;

	for( int  i = 0; i < 6; ++i )
	{
		int nextShadow = 0;
		if( m_shadowTextures[i] )
		{
			perObjectPSBuffer.shadowRect[i].x = float( m_shadowTextures[i]->GetX() + 0.5f ) / m_shadowTextures[i]->GetTextureWidth();
			perObjectPSBuffer.shadowRect[i].y = float( m_shadowTextures[i]->GetY() + 0.5f ) / m_shadowTextures[i]->GetTextureHeight();
			perObjectPSBuffer.shadowRect[i].z = float( m_shadowTextures[i]->GetWidth() - 1.0f ) / m_shadowTextures[i]->GetTextureWidth();
			perObjectPSBuffer.shadowRect[i].w = float( m_shadowTextures[i]->GetHeight() - 1.0f ) / m_shadowTextures[i]->GetTextureHeight();
			for( int j = i - 1; j >= 0; --j )
			{
				if( m_shadowTextures[j] )
				{
					nextShadow = j;
					break;
				}
			}
		}
		else
		{
			perObjectPSBuffer.shadowRect[i] = Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
		}
		perObjectPSBuffer.shadowMatrix[i] = Matrix(
			m_shadowRect[i].x, m_shadowRect[i].x, 0.f, 0.f, 
			m_shadowRect[i].y, m_shadowRect[i].y, 0.f, 0.f,  
			m_shadowRect[i].z, 1.f / ( m_shadowRect[i].z - m_shadowRect[i].x ), 0.f, 0.f, 
			m_shadowRect[i].w, 1.f / ( m_shadowRect[i].w - m_shadowRect[i].y ), 0.f, float( nextShadow ) );
	}

	{
		Matrix texAdjust( 0.5f, 0.0f, 0.0f, 0.0f,
						  0.0f, -0.5f, 0.0f, 0.0f,
						  0.0f, 0.0f, 1.0f, 0.0f,
						  0.5f, 0.5f, 0.0f, 1.0f );
		perObjectPSBuffer.mirrorToWorldMatrix = GetViewMatrix( 0 ) * m_projectionMatrix[0] * texAdjust;
		D3DXMatrixTranspose( &perObjectPSBuffer.mirrorToWorldMatrix, &perObjectPSBuffer.mirrorToWorldMatrix );
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
//   Returns light's position for "light spider" tool. There is
//   no sencible position for directional light so we return
//   Vector3(0, 0, 0).
// Return Value:
//   Position to render a ray to for "light spider" tool.
// -------------------------------------------------------------
Vector3 Tr2InteriorDirectionalLight::GetPositionForLightSpider( const Matrix &objectToWorldMatrix ) const
{
	Vector3 position( 0.0f, 0.0f, 0.0f );
	return position;
}

// --------------------------------------------------------------------------------------
// Description:
//   Called whenever light's transform changes. Rebuilds transform matrix and updates
//   shadow frustum.
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::TransformModified()
{
	Vector3 side( 1.f, 0.f, 0.f );
	if( abs( m_direction.x ) > 0.99f )
	{
		side = Vector3( 0.f, 0.f, 1.f );
	}
	Vector3 up;
	D3DXVec3Cross( &up, &side, &m_direction );
	D3DXVec3Normalize( &up, &up );

	Vector3 zero( 0.f, 0.f, 0.f );
	D3DXMatrixLookAtRH( &m_transformInv, &zero, &m_direction, &up );
	D3DXMatrixInverse( &m_transform, NULL, &m_transformInv );

	RebuildBounds();
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes the cell from the Umbra scene by destroying all the ROI's and the Umbra 
//   model
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::ClearUmbra( void )
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
void Tr2InteriorDirectionalLight::RebuildVolume( void )
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
bool Tr2InteriorDirectionalLight::AddToEnlightenSystem( Tr2InteriorDusterCache* dusters,
														const Matrix &systemTransformInv ) const
{
	if( !m_secondaryLighting )
	{
		return false;
	}

	Color color = m_color;
	if( m_useKelvinColor )
	{
		color = m_kelvinColor->AsRGB();
	}
	const float scale = m_secondaryLightingMultiplier;
	const Geo::v128 color1 = Geo::VConstruct(	TriGammaToLinear( color.r ) * scale, 
												TriGammaToLinear( color.g ) * scale,
												TriGammaToLinear( color.b ) * scale, 
												color.a );

	Vector3 localDirection;
	D3DXVec3TransformNormal( &localDirection, &m_direction, &systemTransformInv );
	Geo::v128 direction = Geo::VConstruct( localDirection.x, localDirection.y, localDirection.z, 0.f );

	Enlighten::InputLight& light = dusters->AddEnlightenLightSource();
	light.m_LightType = Enlighten::ENLIGHTEN_DIRECTIONAL_LIGHT;
	light.m_Direction = direction;
	light.m_Intensity = color1;
	light.m_ProjectionMap = NULL;
	light.m_ShadowMap = NULL;

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Chooses an appropriate effect for light rendering during light accumulation pass.
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::ChooseLightEffect()
{
	Tr2ShaderSituation situation;

	for( unsigned int i = 0; i < m_shadowLODs; ++i )
	{
		if( m_shadowTextures[i] )
		{
			situation.AddSituationString( "Shadow" );
			break;
		}
	}
	if( m_shadowLODs & 1 )
	{
		situation.AddSituationString( "ShadowCount1" );
	}
	if( m_shadowLODs & 2 )
	{
		situation.AddSituationString( "ShadowCount2" );
	}
	if( m_shadowLODs & 4 )
	{
		situation.AddSituationString( "ShadowCount3" );
	}
	if( m_LODBlendRegion > 0.f )
	{
		situation.AddSituationString( "SmoothShadowLOD" );
	}
	if( m_debugShadowLODs )
	{
		situation.AddSituationString( "DebugLOD" );
	}
	m_material->BindLowLevelShader( situation );

	situation.AddSituationString( "GenerateSHCoefficients" );
	m_shMaterial->BindLowLevelShader( situation );
	situation.AddSituationString( "GenerateLightMap" );
	m_lightmapMaterial->BindLowLevelShader( situation );
}

// --------------------------------------------------------------------------------------
// Description:
//   Releases shadow texture or marks shadow as dirty.
// --------------------------------------------------------------------------------------
void Tr2InteriorDirectionalLight::UpdateShadowMap()
{
	if( m_primaryLighting && m_shadowImportance > 0.0f && m_shadowCasterTypes != ST_NONE )
	{
		for( int i = 0; i < 6; ++i )
		{
			if( m_shadowTextures[i] == NULL )
			{
				m_dirtyShadow[i] = true;
				m_emptyShadow[i] = false;
			}
		}
	}
	else
	{
		for( int i = 0; i < 6; ++i )
		{
			m_shadowTextures[i] = NULL;
		}
	}
	ChooseLightEffect();
}

#endif
