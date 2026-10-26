// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "Tr2LightManager.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Effect.h"
#include "Tr2GpuBuffer.h"
#include "Tr2GpuStructuredBuffer.h"
#include "Tr2TextureArray.h"
#include "Tr2DepthStencil.h"

#include "include/TriMath.h"

#include "Shader/Tr2Shader.h"
#include "Resources/TriTextureRes.h"
#include "ITr2TextureProvider.h"
#include "Tr2RenderTarget.h"

#include "TriSettingsRegistrar.h"

bool g_useDynamicLightsShadows = false;
TRI_REGISTER_SETTING( "useDynamicLightsShadows", g_useDynamicLightsShadows );

CCP_STATS_DECLARE( lightsGathered, "Trinity/Tr2LightManager/lightsGathered", true, CST_COUNTER_LOW, "How many lights were pushed to GPU" );

namespace
{

// Initial size of the light buffer (number of elements)
const uint32_t LIGHT_BUFFER_SIZE = 1024;
// Size of the light index buffer (number of indices)
const uint32_t INDEX_BUFFER_SIZE = 8 * 1024 * 1024;
// Size of the light index buffer of a reflection probe face (number of indices)
const uint32_t INDEX_BUFFER_SIZE_IN_REFLECTIONS = 1024 * 1024;

// Minimal size of a point light sphere in pixels before it is culled out
const float CUTOFF_PIXEL_SIZE = 7.f;
// Size (in pixels) for the light to stat dimming out before disappeating
const float FADE_SIZE = 5.f;

// Tile sizes in pixels, should match thread count in a compute shader
const uint32_t TILE_WIDTH = 16;
const uint32_t TILE_HEIGHT = 16;

// Size (in pixels) for width/height of shadow map atlas used by shadowcasting pointlights and spotlights
const uint32_t HIGH_QUALITY_ATLAS_SIZE_LOG2 = 14;
const uint32_t HIGH_QUALITY_ATLAS_ENTRY_MAX_SIZE = 1 << 13;
const uint32_t MAX_NUM_SHADOWCASTING_LIGHTS = 16;
const uint32_t MAX_NUM_SHADOWCASTING_LIGHTS_IN_REFLECTIONS = 4;
const uint32_t MAX_NUM_VOLUMETRIC_LIGHTS = 16;
const uint32_t MAX_NUM_VOLUMETRIC_LIGHTS_IN_REFLECTIONS = 16;

const uint32_t RAYTRACING_DUMMY_RESOLUTION = 4;

struct PerFrameData
{
	Matrix viewInverse;
	Matrix projInverse;
	Vector3 cameraPos;
	// cppcheck-suppress unusedStructMember
	float _padding0;
	uint32_t width;
	uint32_t height;
	uint32_t tilesX;
	uint32_t tilesY;
	uint32_t lightCount;
	uint32_t indexBufferSize;
	// cppcheck-suppress unusedStructMember
	uint32_t _padding[2];
};

bool ExtractAnnotationValue( const Tr2EffectParameterAnnotation& annotation,
							 const char* name,
							 uint32_t& value )
{
	if( strcmp( annotation.name, name ) == 0 )
	{
		if( annotation.type == Tr2EffectParameterAnnotation::INT )
		{
			value = uint32_t( annotation.intValue );
			return true;
		}
		else
		{
			CCP_LOGERR( "Tr2LightManager: unexpected annotation \"%s\" type", name );
		}
	}
	return false;
}

Tr2LightManager*& Singleton()
{
	static Tr2LightManager* manager = nullptr;
	return manager;
}

Tr2LightManager::ShadowMapAtlasSettings CalculateShadowMapAtlasSettings( ShadowQuality shadowQuality )
{
	Tr2LightManager::ShadowMapAtlasSettings settings{};
	switch( shadowQuality )
	{
	case ShadowQuality::SHADOW_DISABLED:
	case ShadowQuality::SHADOW_RAYTRACED:
		settings.sizeLog2 = 0;
		settings.size = 0;
		settings.entryMinSizeLog2 = 0;
		settings.entryMinSize = 0;
		settings.entryInverseScaleFactorLog2 = 0;
		settings.entryMaxSize = 0;
		break;
	case ShadowQuality::SHADOW_LOW:
		settings.sizeLog2 = HIGH_QUALITY_ATLAS_SIZE_LOG2 - 2;
		break;
	case ShadowQuality::SHADOW_HIGH:
		settings.sizeLog2 = HIGH_QUALITY_ATLAS_SIZE_LOG2;
		break;
	}
	if( shadowQuality != ShadowQuality::SHADOW_DISABLED )
	{
		settings.size = 1 << settings.sizeLog2;
		settings.entryMinSizeLog2 = settings.sizeLog2 - 10;
		settings.entryMinSize = 1 << settings.entryMinSizeLog2;
		settings.entryInverseScaleFactorLog2 = HIGH_QUALITY_ATLAS_SIZE_LOG2 - settings.sizeLog2;
		settings.entryMaxSize = HIGH_QUALITY_ATLAS_ENTRY_MAX_SIZE >> settings.entryInverseScaleFactorLog2;
	}
	return settings;
}

// for shadow scene
struct RtShadowPerFrameData
{
	Matrix projectionInverse;
	Matrix viewInverse;

	CcpMath::Sphere planets[2];

	Vector2 resolution;
	uint32_t numDynamicLights;
	uint32_t padding;

	Tr2LightManager::PerLightData dynamicLights[16];
};

const BlueSharedString RtShadowTechniqueName = BlueSharedString( "RtShadow" );
const BlueSharedString RtShadowMapTechniqueName = BlueSharedString( "RtShadowDest" );
const BlueSharedString RtNormalBufferTechniqueName = BlueSharedString( "RtShadowNormalBuffer" );
const BlueSharedString RtSceneTechniqueName = BlueSharedString( "RtShadowScene" );

}

Tr2LightManager::View::View( uint32_t maxVolumetricLights, uint32_t maxShadowCastingLights, uint32_t indexBufferSize, bool reflection ) :
	reflection( reflection ),
	maxVolumetricLights( maxVolumetricLights ),
	maxShadowCastingLights( maxShadowCastingLights ),
	indexBufferSize( indexBufferSize )
{
	lightBuffer.CreateInstance();
	indexBuffer.CreateInstance();
	indexBufferCounter.CreateInstance();
}

Tr2LightManager::Tr2LightManager( const char* effectPath )
{
	m_effect.CreateInstance();
	m_effect->SetEffectPathName( effectPath );

	m_views.emplace_back( MAX_NUM_VOLUMETRIC_LIGHTS, MAX_NUM_SHADOWCASTING_LIGHTS, INDEX_BUFFER_SIZE, false );
	View& mainView = m_views[MAIN_VIEW];

	m_lightBufferVariable.Register( "LightBuffer", mainView.lightBuffer );
	m_indexBufferVariable.Register( "LightIndexBuffer", mainView.indexBuffer );
	GlobalStore().RegisterVariable( "LightProfileArray", &GetLightProfileArray() );

	m_currentSpaceSceneShadowQuality = ShadowQuality::SHADOW_DISABLED;

	m_ShadowMap.m_qualityUsedByAtlas = ShadowQuality::SHADOW_DISABLED;

	m_currentFrameCounter = -1;

	GlobalStore().RegisterVariable( "EveSpaceSceneDynamicShadowMap", Tr2TextureAL{} );

	m_Raytracing.m_effect.CreateInstance();
	m_Raytracing.m_effect->SetEffectPathName( "res:/graphics/effect/managed/space/system/raytracing/rtdynamicshadows.fx" );

	PrepareResources();
}

Tr2LightManager::~Tr2LightManager()
{
	ResetVariableStore();
}

void Tr2LightManager::ResetVariableStore()
{
	Tr2GpuStructuredBufferPtr empty;
	empty.CreateInstance();

	GlobalStore().RegisterVariable( "LightBuffer", empty );
	GlobalStore().RegisterVariable( "LightIndexBuffer", empty );
	GlobalStore().RegisterVariable( "LightProfileArray", &GetLightProfileArray() );

	Tr2DepthStencilPtr emptyTexture;
	GlobalStore().RegisterVariable( "ShadowMapAtlas", emptyTexture );
}

Tr2LightManager* Tr2LightManager::GetOrCreateInstance( const char* effectPath )
{
	auto& singleton = Singleton();
	if( !singleton )
	{
		singleton = CCP_NEW( "Tr2LightManager::Singleton" ) Tr2LightManager( effectPath );
	}
	return singleton;
}

Tr2LightManager* Tr2LightManager::GetInstance()
{
	return Singleton();
}

void Tr2LightManager::DeleteInstance()
{
	auto& singleton = Singleton();
	CCP_DELETE singleton;
	singleton = nullptr;
}

uint16_t Tr2LightManager::PackFlags( uint16_t flags, uint16_t profileIndex )
{
	return ( flags & ( ( 1 << FLAG_BITS ) - 1 ) ) | ( profileIndex << FLAG_BITS );
}

Tr2LightManager::View& Tr2LightManager::GetActiveView()
{
	return m_views[m_activeView];
}

const Tr2LightManager::View& Tr2LightManager::GetActiveView() const
{
	return m_views[m_activeView];
}

void Tr2LightManager::CreateViewBuffers( View& view, Tr2RenderContext& renderContext )
{
	view.lightBuffer->Create( LIGHT_BUFFER_SIZE, sizeof( PerLightData ), Tr2GpuBuffer::CPU_WRITABLE );
	view.lightBuffer->SetName( "Light buffer" );
	view.indexBuffer->Create( view.indexBufferSize, sizeof( uint32_t ), Tr2GpuStructuredBuffer::GPU_WRITABLE );
	view.indexBuffer->SetName( "Light indices" );
	view.indexBufferCounter->Create( 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32_SINT, Tr2GpuBuffer::GPU_WRITABLE );
	ClearLightIndices( view, renderContext );
}

void Tr2LightManager::ClearReflectionViews()
{
	while( m_views.size() > MAIN_VIEW + 1 )
	{
		m_views.pop_back();
	}

	if( m_activeView != MAIN_VIEW )
	{
		m_activeView = MAIN_VIEW;
		m_lightBufferVariable = m_views[MAIN_VIEW].lightBuffer;
		m_indexBufferVariable = m_views[MAIN_VIEW].indexBuffer;
	}
}

uint8_t Tr2LightManager::AddReflectionView()
{
	m_views.emplace_back( MAX_NUM_VOLUMETRIC_LIGHTS_IN_REFLECTIONS, 0u, INDEX_BUFFER_SIZE_IN_REFLECTIONS, true );

	if( Tr2Renderer::IsResourceCreationAllowed() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CreateViewBuffers( m_views.back(), renderContext );
	}

	return static_cast<uint8_t>( m_views.size() - 1 );
}

uint32_t Tr2LightManager::GetReflectionViewCount() const
{
	return static_cast<uint32_t>( m_views.size() - ( MAIN_VIEW + 1 ) );
}

void Tr2LightManager::SetViewFrustum( uint8_t viewIndex, const TriFrustum& frustum )
{
	if( viewIndex >= m_views.size() )
	{
		CCP_LOGERR( "Tr2LightManager: invalid view index %u", viewIndex );
		return;
	}

	m_views[viewIndex].frustum = frustum;
}

void Tr2LightManager::SetActiveView( uint8_t viewIndex, const TriFrustum& frustum )
{
	if( viewIndex >= m_views.size() )
	{
		CCP_LOGERR( "Tr2LightManager: invalid view index %u", viewIndex );
		return;
	}

	m_activeView = viewIndex;
	View& view = m_views[viewIndex];
	view.frustum = frustum;
	m_lightBufferVariable = view.lightBuffer;
	m_indexBufferVariable = view.indexBuffer;
}

void Tr2LightManager::Clear( Tr2RenderContext& renderContext )
{
	for( auto& view: m_views )
	{
		ClearLightIndices( view, renderContext );
		for( auto& data : view.tlsLightData )
		{
			data.clear();
		}
		view.lightData.clear();
		view.shadowCastingLights.clear();
		view.volumetricLights.clear();
	}
	
	// shadow map is only on the main view currently
	m_ShadowMap.m_atlasNodes.resize( 1 );
	m_ShadowMap.m_atlasNodes[0].x = 0;
	m_ShadowMap.m_atlasNodes[0].y = 0;
	m_ShadowMap.m_atlasNodes[0].width = m_ShadowMap.m_atlasSettings.size;
	m_ShadowMap.m_atlasNodes[0].height = m_ShadowMap.m_atlasSettings.size;
	m_ShadowMap.m_atlasNodes[0].children[0] = -1;
	m_ShadowMap.m_atlasNodes[0].children[1] = -1;
	m_ShadowMap.m_atlasNodes[0].lightIndex = -1;
}

void Tr2LightManager::AdjustLightCutoff( float lodFactor )
{
	m_adjustedCutoff = CUTOFF_PIXEL_SIZE * lodFactor;
}

void Tr2LightManager::SetReflectionQuality( ReflectionQuality reflectionQuality )
{
	m_reflectionQuality = reflectionQuality;
}

// This entire thing is annoyingly complex for what should be a simple thing...
// The reason why it's such a mess is because we have multiple eve space scenes with different
// shadow qualities, but want to share resources across them. But we also don't have a way
// of getting all eve space scenes, so we have to collect the qualities during a frame
// and based on that decide which resources are needed for the next frame.
// In addition we have different ways of rendering shadows: Shadowmapped with different
// atlas resolutions and raytraced shadows.
// On top of that we don't want to use any resources when the feature flag is false.
void Tr2LightManager::SetShadowQuality( ShadowQuality shadowQuality, uint64_t frameCounter )
{
	m_currentSpaceSceneShadowQuality = shadowQuality;

	if( m_currentFrameCounter != frameCounter )
	{
		if( !g_useDynamicLightsShadows )
		{
			nextFrameShadowQuality = 0u;
		}

		// there must be a more elegant way of doing this...
		if( nextFrameShadowQuality & ( 1 << (uint32_t)ShadowQuality::SHADOW_HIGH ) )
		{
			m_ShadowMap.m_qualityUsedByAtlas = ShadowQuality::SHADOW_HIGH;
		}
		else if( nextFrameShadowQuality & ( 1 << (uint32_t)ShadowQuality::SHADOW_LOW ) )
		{
			m_ShadowMap.m_qualityUsedByAtlas = ShadowQuality::SHADOW_LOW;
		}
		else
		{
			m_ShadowMap.m_qualityUsedByAtlas = ShadowQuality::SHADOW_DISABLED;
		}
		nextFrameShadowQuality = 1 << (uint32_t)shadowQuality;
		m_currentFrameCounter = frameCounter;
	}

	nextFrameShadowQuality |= 1 << (uint32_t)shadowQuality;

	ShadowQuality tmpShadowQuality = (ShadowQuality)min( (uint32_t)shadowQuality, (uint32_t)m_ShadowMap.m_qualityUsedByAtlas );
	m_ShadowMap.m_atlasSettings = CalculateShadowMapAtlasSettings( tmpShadowQuality );
	m_ShadowMap.m_atlasSettings.actualTextureSize = CalculateShadowMapAtlasSettings( m_ShadowMap.m_qualityUsedByAtlas ).size;
}

void Tr2LightManager::SetLightingQuality( LightingQuality lightingQuality )
{
	m_lightingQuality = lightingQuality;
}

LightingQuality Tr2LightManager::GetLightingQuality() const
{
	return m_lightingQuality;
}

void Tr2LightManager::AddPointLight( const Vector3& position, float radius, const Color& color, Float_16 innerRadius, uint16_t flags, bool scaleBrightness )
{
	if( !AreLightFlagsValid( flags ) )
	{
		return;
	}
	PerLightData data;
	data.position = position;
	data.radius = radius;
	float brightness = std::max( std::max( color.r, color.g ), color.b );
	if( brightness <= 0 )
	{
		return;
	}

	View& mainView = m_views[MAIN_VIEW];

	if( !mainView.frustum.IsSphereVisible( reinterpret_cast<Vector4*>( &data.position ) ) )
	{
		return;
	}

	float size = mainView.frustum.GetPixelSizeAccross( reinterpret_cast<Vector4*>( &data.position ) );

	if( size > m_adjustedCutoff )
	{
		float dimming = std::min( ( size - m_adjustedCutoff ) / FADE_SIZE, 1.f );
		data.color = reinterpret_cast<const Vector3&>( color );
		if( scaleBrightness )
		{
			dimming *= radius;
		}
		data.color.x *= dimming;
		data.color.y *= dimming;
		data.color.z *= dimming;
		data.innerRadius = innerRadius;
		data.flags = flags;
		data.direction = Vector3_16( Vector3( 1.f, 0.f, 0.f ) );
		data.outerAngle = Float_16( 0.f );
		data.innerAngle = Float_16( 0.f );
		mainView.tlsLightData.local().push_back( data );
	}
}

void Tr2LightManager::AddLight( PerLightData& data, bool scaleBrightness, const Tr2LightTags& tags )
{
	if( !AreLightFlagsValid( data.flags ) )
	{
		return;
	}

	float brightness = std::max( std::max( data.color.x, data.color.y ), data.color.z );
	if( brightness <= 0 || data.radius <= 0 )
	{
		return;
	}

	for( auto& view : m_views )
	{
		// a face view only takes the lights that are tagged for the current reflection quality
		if( view.reflection && !tags.reflects.HasBit( m_reflectionQuality ) )
		{
			continue;
		}

		if( !view.frustum.IsSphereVisible( reinterpret_cast<Vector4*>( &data.position ) ) )
		{
			continue;
		}
		float size = view.frustum.GetPixelSizeAccross( reinterpret_cast<Vector4*>( &data.position ) );
		if( size > m_adjustedCutoff )
		{
			// every view works on its own copy, as the fade and the flags differ per view
			PerLightData viewData = data;
			float dimming = std::min( ( size - m_adjustedCutoff ) / FADE_SIZE, 1.f );
			if( scaleBrightness )
			{
				dimming *= viewData.radius;
			}
			viewData.color.x *= dimming;
			viewData.color.y *= dimming;
			viewData.color.z *= dimming;

			bool usingShadowMap = m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_LOW || m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_HIGH;
			if( view.reflection ||
				m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_DISABLED ||
				( usingShadowMap && m_ShadowMap.m_qualityUsedByAtlas == ShadowQuality::SHADOW_DISABLED ) ||
				!g_useDynamicLightsShadows )
			{
				viewData.flags &= ~Tr2LightManager::FLAG_CASTS_SHADOWS;
			}
			view.tlsLightData.local().push_back( viewData );
		}
	}
}

ALResult Tr2LightManager::ClearLightIndices( Tr2LightManager::View& view, Tr2RenderContext& renderContext )
{
	if( !view.indexBuffer || !view.indexBuffer->GetGpuBuffer( 0 ) )
	{
		return S_OK;
	}
	const uint32_t clear[] = { 0, 0, 0, 0 };
	return renderContext.ClearUav( *view.indexBuffer->GetGpuBuffer( 0 ), clear );
}

ALResult Tr2LightManager::UpdateLightBuffer( Tr2RenderContext& renderContext )
{
	View& view = GetActiveView();
	if( view.lightBuffer->GetGpuBuffer( 0 )->GetDesc().count < uint32_t( view.lightData.size() ) )
	{
		CR_RETURN_HR( view.lightBuffer->Create(
			std::max( view.lightBuffer->GetGpuBuffer( 0 )->GetDesc().count + 1024, uint32_t( view.lightData.size() ) ),
			sizeof( PerLightData ),
			Tr2GpuBuffer::CPU_WRITABLE ) );
		view.lightBuffer->SetName( "Light buffer" );
	}

	auto lightCount = std::min( view.lightBuffer->GetGpuBuffer( 0 )->GetDesc().count, uint32_t( view.lightData.size() ) );

	Vector4* data;
	CR_RETURN_HR( view.lightBuffer->GetGpuBuffer( 0 )->MapForWriting( data, renderContext ) );
	memcpy( data, view.lightData.data(), lightCount * sizeof( PerLightData ) );
	CCP_STATS_ADD( lightsGathered, view.lightData.size() );
	view.lightBuffer->GetGpuBuffer( 0 )->UnmapForWriting( renderContext );
	return S_OK;
}

ALResult Tr2LightManager::DoUpdateLists( const Tr2TextureAL& depthMap, Tr2RenderContext& renderContext )
{
	View& view = GetActiveView();
	if( !view.lightBuffer->IsValid() || !view.indexBuffer->IsValid() || !view.indexBufferCounter->IsValid() )
	{
		return E_INVALIDCALL;
	}

	CR_RETURN_HR( UpdateLightBuffer( renderContext ) );

	// the effect is shared by all the views, so it has to be pointed at the buffers of the active one
	m_effect->SetParameter( BlueSharedString( "LightBuffer" ), view.lightBuffer );
	m_effect->SetParameter( BlueSharedString( "LightIndices" ), view.indexBuffer );
	m_effect->SetParameter( BlueSharedString( "LightIndexCount" ), view.indexBufferCounter );

	PerFrameData perFrameData;
	perFrameData.projInverse = Transpose( Tr2Renderer::GetInverseProjectionTransform() );
	perFrameData.viewInverse = Transpose( Tr2Renderer::GetInverseViewTransform() );
	perFrameData.cameraPos = Tr2Renderer::GetViewPosition();
	perFrameData.width = uint32_t( renderContext.m_esm.GetRenderTargetWidth() );
	perFrameData.height = uint32_t( renderContext.m_esm.GetRenderTargetHeight() );
	perFrameData.tilesX = ( perFrameData.width + ( TILE_WIDTH - 1 ) ) / TILE_WIDTH;
	perFrameData.tilesY = ( perFrameData.height + ( TILE_HEIGHT - 1 ) ) / TILE_HEIGHT;
	perFrameData.lightCount = std::min( view.lightBuffer->GetGpuBuffer( 0 )->GetDesc().count, uint32_t( view.lightData.size() ) );
	perFrameData.indexBufferSize = view.indexBufferSize;


	m_effect->SetParameter( BlueSharedString( "DepthMap" ), depthMap );
	ON_BLOCK_EXIT( [&] { m_effect->SetParameter( BlueSharedString( "DepthMap" ), Tr2TextureAL() ); } );

	if( !FillAndSetConstants( m_perFrameData, perFrameData, Tr2RenderContextEnum::COMPUTE_SHADER, Tr2Renderer::GetPerFramePSStartRegister(), renderContext ) )
	{
		return E_FAIL;
	}

	if( !Tr2Renderer::RunComputeShader( m_effect, BlueSharedString( "Clear" ), 1, 1, 1, renderContext ) )
	{
		return E_FAIL;
	}

	if( !Tr2Renderer::RunComputeShader( m_effect, perFrameData.tilesX, perFrameData.tilesY, 1, renderContext ) )
	{
		return E_FAIL;
	}
	return S_OK;
}

void Tr2LightManager::CreateShadowMapAtlas( std::vector<PerLightData>& lightDataList, uint32_t numShadowCastingLights, const std::vector<LightScreenSizeTuple>& lightTuples )
{
	bool everythingFit = false;
	// 5 iterations are more than enough. With the current values we should actually only need at most 4 iterations.
	for( uint32_t j = 0; j < 5 && !everythingFit; j++ )
	{
		everythingFit = true;
		uint32_t entryInverseScaleFactorLog2 = m_ShadowMap.m_atlasSettings.entryInverseScaleFactorLog2 + j;
		uint32_t entryMaxSize = m_ShadowMap.m_atlasSettings.entryMaxSize >> j;

		// clear atlas entries
		m_ShadowMap.m_atlasNodes.resize( 1 );
		m_ShadowMap.m_atlasNodes[0].x = 0;
		m_ShadowMap.m_atlasNodes[0].y = 0;
		m_ShadowMap.m_atlasNodes[0].width = m_ShadowMap.m_atlasSettings.size;
		m_ShadowMap.m_atlasNodes[0].height = m_ShadowMap.m_atlasSettings.size;
		m_ShadowMap.m_atlasNodes[0].children[0] = -1;
		m_ShadowMap.m_atlasNodes[0].children[1] = -1;
		m_ShadowMap.m_atlasNodes[0].lightIndex = -1;

		// make entries into the shadow map atlas
		for( uint32_t i = 0; i < numShadowCastingLights; i++ )
		{
			uint32_t lightIndex = lightTuples[i].lightIndex;
			Tr2LightManager::PerLightData& lightData = lightDataList[lightIndex];

			uint32_t size = ( (uint32_t)lightTuples[i].sizeAcross ) >> entryInverseScaleFactorLog2;
			size = ClampUInt( size, m_ShadowMap.m_atlasSettings.entryMinSize, entryMaxSize );
			size = CCP_ALIGN( size, m_ShadowMap.m_atlasSettings.entryMinSize );

			uint32_t width;
			uint32_t height;
			if( lightData.innerAngle <= 0.f )
			{
				// pointlight
				width = 3 * size;
				height = 2 * size;
			}
			else
			{
				// spotlight
				width = size;
				height = size;
			}

			uint32_t offsetX;
			uint32_t offsetY;
			bool gotShadowMapAtlasEntry = GetShadowMapAtlasEntry( lightIndex, width, height, offsetX, offsetY );

			if( gotShadowMapAtlasEntry )
			{
				lightData.ShadowMapping.shadowMapOffsetX = offsetX >> m_ShadowMap.m_atlasSettings.entryMinSizeLog2;
				lightData.ShadowMapping.shadowMapOffsetY = offsetY >> m_ShadowMap.m_atlasSettings.entryMinSizeLog2;
				lightData.ShadowMapping.shadowMapScale = size >> m_ShadowMap.m_atlasSettings.entryMinSizeLog2;
			}
			else
			{
				//lightData.flags &= ~Tr2LightManager::FLAG_CASTS_SHADOWS;
				lightData.ShadowMapping.shadowMapOffsetX = 0;
				lightData.ShadowMapping.shadowMapOffsetY = 0;
				lightData.ShadowMapping.shadowMapScale = 0;
				everythingFit = false;
			}
		}
	}
	CCP_ASSERT_M( everythingFit, "Not all entries fit into the shadow map atlas!" );
}

Tr2RaytracingPipelineStateManager* Tr2LightManager::GetRaytracingPipelineManager()
{
	return &m_Raytracing.m_pipelineManager;
}

Tr2RtShaderTableDescriptionAL* Tr2LightManager::GetRaytracingShaderTableDesc()
{
	return &m_Raytracing.m_shaderTableDesc;
}

void Tr2LightManager::ResolveLightData()
{
	for( auto& view : m_views )
	{
		ResolveView( view );
	}
}

void Tr2LightManager::ResolveView( View& view )
{
	std::vector<PerLightData>& lightData = view.lightData;
	lightData.clear();
	for( auto& data : view.tlsLightData )
	{
		if( !data.empty() )
		{
			lightData.insert( end( lightData ), begin( data ), end( data ) );
		}
		data.clear();
	}

	view.shadowCastingLights.clear();
	view.volumetricLights.clear();

	// filter volumetric lights
	std::vector<LightScreenSizeTuple> lightTuples;
	{
		for( uint32_t i = 0; i < lightData.size(); i++ )
		{
			if( ( lightData[i].flags & FLAG_IS_VOLUMETRIC ) != 0 )
			{
				float sizeAcross = view.frustum.GetPixelSizeAccrossEst( reinterpret_cast<Vector4*>( &lightData[i].position ) );
				sizeAcross = sizeAcross == std::numeric_limits<float>::max() ? ( 1 << HIGH_QUALITY_ATLAS_SIZE_LOG2 ) : sizeAcross;
				lightTuples.push_back( LightScreenSizeTuple{ i, sizeAcross } );
			}
		}

		// sort volumetric lights by size on screen
		std::sort( lightTuples.begin(), lightTuples.end(), []( const LightScreenSizeTuple& a, const LightScreenSizeTuple& b ) {
			return a.sizeAcross > b.sizeAcross;
		} );

		// keep only the largest lights
		uint32_t numVolumetricLights = min( view.maxVolumetricLights, (uint32_t)lightTuples.size() );
		view.volumetricLights.resize( numVolumetricLights );
		uint32_t i = 0;
		for( ; i < numVolumetricLights; i++ )
		{
			view.volumetricLights[i] = lightTuples[i].lightIndex;
		}
		for( ; i < lightTuples.size(); i++ )
		{
			Tr2LightManager::PerLightData& light = lightData[lightTuples[i].lightIndex];
			light.flags &= ~Tr2LightManager::FLAG_IS_VOLUMETRIC;
		}
	}

	// the shadow map atlas is only on the main view currently
	if( view.reflection || !g_useDynamicLightsShadows || m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_DISABLED )
	{
		return;
	}

	// filter shadowcasting lights
	uint32_t numShadowCastingLights = 0;
	lightTuples.resize( 0 );
	{
		for( uint32_t i = 0; i < lightData.size(); i++ )
		{
			if( ( lightData[i].flags & FLAG_CASTS_SHADOWS ) != 0 )
			{
				float sizeAcross = view.frustum.GetPixelSizeAccrossEst( reinterpret_cast<Vector4*>( &lightData[i].position ) );
				sizeAcross = sizeAcross == std::numeric_limits<float>::max() ? ( 1 << HIGH_QUALITY_ATLAS_SIZE_LOG2 ) : sizeAcross;
				lightTuples.push_back( LightScreenSizeTuple{ i, sizeAcross } );
			}
		}

		// sort shadowcasting lights by size on screen
		std::sort( lightTuples.begin(), lightTuples.end(), []( const LightScreenSizeTuple& a, const LightScreenSizeTuple& b ) {
			return a.sizeAcross > b.sizeAcross;
		} );

		// keep only the largest lights
		numShadowCastingLights = min( view.maxShadowCastingLights, (uint32_t)lightTuples.size() );
		view.shadowCastingLights.resize( numShadowCastingLights );
		uint32_t i = 0;
		for( ; i < numShadowCastingLights; i++ )
		{
			view.shadowCastingLights[i] = lightTuples[i].lightIndex;
			if( m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_RAYTRACED )
			{
				Tr2LightManager::PerLightData& light = lightData[lightTuples[i].lightIndex];
				light.Raytracing.shadowMask = 1 << i;
			}
		}
		for( ; i < lightTuples.size(); i++ )
		{
			Tr2LightManager::PerLightData& light = lightData[lightTuples[i].lightIndex];
			light.flags &= ~Tr2LightManager::FLAG_CASTS_SHADOWS;
			if( m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_RAYTRACED )
			{
				light.Raytracing.shadowMask = 0;
			}
		}
	}

	if( m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_LOW || m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_HIGH )
	{
		CreateShadowMapAtlas( lightData, numShadowCastingLights, lightTuples );
	}
}

ALResult Tr2LightManager::UpdateLists( const Tr2TextureAL& depthMap, Tr2RenderContext& renderContext )
{
	View& view = GetActiveView();
	m_lightBufferVariable = view.lightBuffer;
	m_indexBufferVariable = view.indexBuffer;

	if( view.lightData.empty() )
	{
		return ClearLightIndices( view, renderContext );
	}

	auto hr = DoUpdateLists( depthMap, renderContext );
	if( FAILED( hr ) )
	{
		ClearLightIndices( view, renderContext );
	}

	return hr;
}

void Tr2LightManager::ReleaseResources( TriStorage s )
{
	if( s & m_perFrameData.GetMemoryClass() )
	{
		m_perFrameData = Tr2ConstantBufferAL();
	}

	// light manager does not release all sorts of buffers/effects. raytracing manager on the other hand does.
	// probably because raytracing manager expects to get destroyed, unlike light manager, which is a singleton...?

	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		m_Raytracing.m_perFrameData = Tr2ConstantBufferAL();
	}
}

bool Tr2LightManager::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	for( auto& view : m_views )
	{
		CreateViewBuffers( view, renderContext );
	}
	m_perFrameData.Create( sizeof( PerFrameData ), Tr2ConstantUsageAL::ONE_SHOT, nullptr, renderContext );

	// DoUpdateLists points the effect at the buffers of the view it works on
	m_effect->SetParameter( BlueSharedString( "LightBuffer" ), m_views[MAIN_VIEW].lightBuffer );
	m_effect->SetParameter( BlueSharedString( "LightIndices" ), m_views[MAIN_VIEW].indexBuffer );
	m_effect->SetParameter( BlueSharedString( "LightIndexCount" ), m_views[MAIN_VIEW].indexBufferCounter );
	m_effect->SetParameter( BlueSharedString( "DepthMap" ), Tr2TextureAL() );

	return true;
}

bool Tr2LightManager::AreLightFlagsValid( uint16_t flags )
{
	return ( flags & ( FLAG_AFFECTS_SURFACES | FLAG_AFFECTS_PARTICLES ) ) != 0;
}

Tr2TextureArray& Tr2LightManager::GetLightProfileArray()
{
	static CTr2TextureArray lightProfiles;
	return lightProfiles;
}

const std::vector<uint32_t>& Tr2LightManager::GetShadowCastingLights() const
{
	return GetActiveView().shadowCastingLights;
}

const std::vector<uint32_t>& Tr2LightManager::GetVolumetricLights() const
{
	return GetActiveView().volumetricLights;
}

const Tr2LightManager::PerLightData& Tr2LightManager::GetLightData( uint32_t index ) const
{
	return GetActiveView().lightData[index];
}

Tr2GpuResourcePool::Texture Tr2LightManager::GetShadowMapAtlas( Tr2GpuResourcePool& gpuResourcePool )
{
	return gpuResourcePool.GetTempTexture( "EveSpaceSceneDynamicShadowMap", m_ShadowMap.m_atlasSettings.actualTextureSize, m_ShadowMap.m_atlasSettings.actualTextureSize, ImageIO::PIXEL_FORMAT_D32_FLOAT, Tr2GpuUsage::DEPTH_STENCIL | Tr2GpuUsage::SHADER_RESOURCE );
}

const Tr2LightManager::ShadowMapAtlasSettings& Tr2LightManager::GetShadowMapAtlasSettings() const
{
	return m_ShadowMap.m_atlasSettings;
}

ShadowQuality Tr2LightManager::GetCurrentSpaceSceneShadowQuality()
{
	return m_currentSpaceSceneShadowQuality;
}

// based on: https://blackpawn.com/texts/lightmaps/default.html
uint32_t Tr2LightManager::InsertAtlasNode( std::vector<AtlasNode>& atlasNodes, uint32_t nodeId, uint32_t lightIndex, int32_t width, int32_t height )
{
	if( atlasNodes[nodeId].children[0] != -1 || atlasNodes[nodeId].children[1] != -1 )
	{
		uint32_t newNode = InsertAtlasNode( atlasNodes, atlasNodes[nodeId].children[0], lightIndex, width, height );
		if( newNode != -1 )
		{
			return newNode;
		}
		return InsertAtlasNode( atlasNodes, atlasNodes[nodeId].children[1], lightIndex, width, height );
	}
	else
	{
		if( atlasNodes[nodeId].lightIndex != -1 )
		{
			return -1;
		}
		if( atlasNodes[nodeId].width < width || atlasNodes[nodeId].height < height )
		{
			return -1;
		}
		if( atlasNodes[nodeId].width == width && atlasNodes[nodeId].height == height )
		{
			atlasNodes[nodeId].lightIndex = lightIndex;
			return nodeId;
		}

		atlasNodes.insert( atlasNodes.end(), 2, AtlasNode() );

		atlasNodes[nodeId].children[0] = uint32_t( atlasNodes.size() - 2 );
		atlasNodes[nodeId].children[1] = uint32_t( atlasNodes.size() - 1 );

		AtlasNode& child0 = atlasNodes[atlasNodes[nodeId].children[0]];
		AtlasNode& child1 = atlasNodes[atlasNodes[nodeId].children[1]];

		int32_t deltaWidth = atlasNodes[nodeId].width - width;
		int32_t deltaHeight = atlasNodes[nodeId].height - height;

		if( deltaWidth > deltaHeight )
		{
			child0.x = atlasNodes[nodeId].x;
			child0.y = atlasNodes[nodeId].y;
			child0.width = width;
			child0.height = atlasNodes[nodeId].height;
			child1.x = atlasNodes[nodeId].x + width;
			child1.y = atlasNodes[nodeId].y;
			child1.width = atlasNodes[nodeId].width - width;
			child1.height = atlasNodes[nodeId].height;
		}
		else
		{
			child0.x = atlasNodes[nodeId].x;
			child0.y = atlasNodes[nodeId].y;
			child0.width = atlasNodes[nodeId].width;
			child0.height = height;
			child1.x = atlasNodes[nodeId].x;
			child1.y = atlasNodes[nodeId].y + height;
			child1.width = atlasNodes[nodeId].width;
			child1.height = atlasNodes[nodeId].height - height;
		}
		child0.lightIndex = -1;
		child0.children[0] = -1;
		child0.children[1] = -1;
		child1.lightIndex = -1;
		child1.children[0] = -1;
		child1.children[1] = -1;

		return InsertAtlasNode( atlasNodes, atlasNodes[nodeId].children[0], lightIndex, width, height );
	}
}

bool Tr2LightManager::GetShadowMapAtlasEntry( uint32_t lightIndex, uint32_t width, uint32_t height, uint32_t& out_posX, uint32_t& out_posY )
{
	width = CCP_ALIGN( width, m_ShadowMap.m_atlasSettings.entryMinSize );
	height = CCP_ALIGN( height, m_ShadowMap.m_atlasSettings.entryMinSize );

	uint32_t nodeId = InsertAtlasNode( m_ShadowMap.m_atlasNodes, 0, lightIndex, (int32_t)width, (int32_t)height );
	//CCP_ASSERT_M( nodeId != -1, "Shadow map atlas could not fit the requested entry." );

	if( nodeId != -1 )
	{
		out_posX = m_ShadowMap.m_atlasNodes[nodeId].x;
		out_posY = m_ShadowMap.m_atlasNodes[nodeId].y;

		CCP_ASSERT_M( ( out_posX & ( m_ShadowMap.m_atlasSettings.entryMinSize - 1 ) ) == 0, "Shadow map atlas entry posX is not aligned!" );
		CCP_ASSERT_M( ( out_posY & ( m_ShadowMap.m_atlasSettings.entryMinSize - 1 ) ) == 0, "Shadow map atlas entry posY is not aligned!" );
	}

	return nodeId != -1;
}

void Tr2LightManager::GetUnpackedShadowMapData( const PerLightData& lightData, uint32_t& shadowMapScale, uint32_t& shadowMapOffsetX, uint32_t& shadowMapOffsetY ) const
{
	shadowMapScale = lightData.ShadowMapping.shadowMapScale << m_ShadowMap.m_atlasSettings.entryMinSizeLog2;
	shadowMapOffsetX = lightData.ShadowMapping.shadowMapOffsetX << m_ShadowMap.m_atlasSettings.entryMinSizeLog2;
	shadowMapOffsetY = lightData.ShadowMapping.shadowMapOffsetY << m_ShadowMap.m_atlasSettings.entryMinSizeLog2;
}

Tr2GpuResourcePool::Texture Tr2LightManager::RenderRaytracedShadows(
	Tr2RaytracingGeometry* geometry,
	const Tr2TextureAL& depth,
	const Tr2TextureAL& normal,
	const CcpMath::Sphere* planets,
	size_t planetCount,
	Tr2GpuResourcePool& gpuResourcePool,
	Tr2RenderContext& renderContext )
{
	renderContext.AddGpuMarker( __FUNCTION__ );
	GPU_REGION( renderContext, "Raytraced dynamic shadows" );
	TRINITY_STATS_ZONE( __FUNCTION__ );

	const View& view = GetActiveView();
	if( view.shadowCastingLights.size() == 0 || !g_useDynamicLightsShadows )
	{
		return {};
	}

	auto destTex = gpuResourcePool.GetTempTexture( "raytracing_dynamic_shadow_dest", depth.GetWidth(), depth.GetHeight(), Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::UNORDERED_ACCESS );

	// texture uav
	m_Raytracing.m_effect->SetParameter( RtShadowMapTechniqueName, destTex );
	m_Raytracing.m_effect->SetParameter( RtNormalBufferTechniqueName, normal );
	ON_BLOCK_EXIT( [&] { m_Raytracing.m_effect->SetParameter( RtNormalBufferTechniqueName, Tr2TextureAL() ); } );

	// scene srv
	m_Raytracing.m_effect->SetParameter( RtSceneTechniqueName, geometry );

	if( !m_Raytracing.m_effect->GetEffectRes() || !m_Raytracing.m_effect->GetEffectRes()->IsGood() )
	{
		return {};
	}

	if( !geometry->HasGeometry() )
	{
		return {};
	}

	uint32_t techniqueIndex;
	if( !m_Raytracing.m_effect->GetShaderStateInterface()->GetTechniqueIndex( RtShadowTechniqueName, techniqueIndex ) )
	{
		return {};
	}

	BlueSharedStringW rayGenName, missName;
	m_Raytracing.m_pipelineManager.AddLibrary( rayGenName, missName, m_Raytracing.m_effect, RtShadowTechniqueName );

	auto pipelineState = m_Raytracing.m_pipelineManager.GetPipelineState( renderContext );

	if( !pipelineState.IsValid() )
	{
		return {};
	}

	if( !m_Raytracing.m_perFrameData.IsValid() )
	{
		m_Raytracing.m_perFrameData.Create( sizeof( RtShadowPerFrameData ), renderContext.GetPrimaryRenderContext() );
	}

	{
		RtShadowPerFrameData* data;
		m_Raytracing.m_perFrameData.Lock( reinterpret_cast<void**>( &data ), renderContext );
		data->projectionInverse = Transpose( Inverse( Tr2Renderer::GetReversedDepthProjectionTransform() ) );
		data->viewInverse = Transpose( Tr2Renderer::GetInverseViewTransform() );

		std::fill( std::begin( data->planets ), std::end( data->planets ), CcpMath::Sphere( Vector3( 0, 0, 0 ), 0 ) );

		for( size_t i = 0; i < std::min( planetCount, sizeof( data->planets ) / sizeof( data->planets[0] ) ); ++i )
		{
			data->planets[i] = planets[i];
		}

		data->resolution = Vector2( float( destTex->GetWidth() ), float( destTex->GetHeight() ) );

		for( uint32_t j = 0; j < view.shadowCastingLights.size(); j++ )
		{
			uint32_t lightIndex = view.shadowCastingLights[j];
			const PerLightData& lightData = view.lightData[lightIndex];
			data->dynamicLights[j] = lightData;
		}
		data->numDynamicLights = (uint32_t)view.shadowCastingLights.size();

		m_Raytracing.m_perFrameData.Unlock( renderContext );
	}

	{
		TRINITY_STATS_ZONE( "Create shader table" );
		Tr2RtLocalMaterialDescriptionAL material;
		material.SetConstants( 7, m_Raytracing.m_perFrameData );

		m_Raytracing.m_shaderTableDesc.AddRayGenShader( rayGenName.c_str(), material );
		m_Raytracing.m_shaderTableDesc.AddMissShader( missName.c_str(), material );
		m_Raytracing.m_shaderTable.Create( m_Raytracing.m_shaderTableDesc, pipelineState, renderContext.GetPrimaryRenderContext() );
	}

	const uint32_t clearValue[] = { 0, 0, 0, 0 };
	renderContext.ClearUav( destTex, 0, clearValue );

	m_Raytracing.m_effect->ApplyMaterialDataForRtState( techniqueIndex, pipelineState, renderContext );
	renderContext.UseAccelerationStructure( geometry->GetTLAS() );

	{
		TRINITY_STATS_ZONE( "renderContext.UseResources" );
		renderContext.UseResources( Tr2UseResourceDestination::COMPUTE, Tr2GpuUsage::SHADER_RESOURCE, geometry->GetBindlessResources() );
	}

	renderContext.DispatchRays( pipelineState, m_Raytracing.m_shaderTable, rayGenName.c_str(), destTex->GetWidth(), destTex->GetHeight(), 1 );
	return destTex;
}
