////////////////////////////////////////////////////////////
//
//    Created:   October 2014
//    Copyright: CCP 2014
//

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


CCP_STATS_DECLARE( lightsGathered, "Trinity/Tr2LightManager/lightsGathered", true, CST_COUNTER_LOW, "How many lights were pushed to GPU" );

namespace
{

// Initial size of the light buffer (number of elements)
const uint32_t LIGHT_BUFFER_SIZE = 1024;
// Size of the light index buffer (number of indices)
const uint32_t INDEX_BUFFER_SIZE = 8 * 1024 * 1024;
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
const uint32_t MAX_NUM_VOLUMETRIC_LIGHTS = 16;

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

Tr2LightManager::Tr2LightManager( const char* effectPath )
{
	m_effect.CreateInstance();
	m_effect->SetEffectPathName( effectPath );

	m_lightBuffer.CreateInstance();
	m_indexBuffer.CreateInstance();

	m_indexBufferCounter.CreateInstance();

	m_lightBufferVariable.Register( "LightBuffer", m_lightBuffer );
	m_indexBufferVariable.Register( "LightIndexBuffer", m_indexBuffer );
	GlobalStore().RegisterVariable( "LightProfileArray", &GetLightProfileArray() );

	m_currentSpaceSceneShadowQuality = ShadowQuality::SHADOW_DISABLED;


	m_ShadowMap.m_atlasVariable.Register( "ShadowMapAtlas", m_ShadowMap.m_atlasDepthStencil );

	m_ShadowMap.m_qualityUsedByAtlas = ShadowQuality::SHADOW_DISABLED;

	m_Raytracing.m_destTex.CreateInstance();

	GlobalStore().RegisterVariable( "EveSpaceSceneDynamicShadowMap", m_Raytracing.m_destTex );

	m_Raytracing.m_effect.CreateInstance();
	m_Raytracing.m_effect->SetEffectPathName( "res:/graphics/effect/managed/space/system/raytracing/rtdynamicshadows.fx" );

	m_Raytracing.m_whiteTexture.CreateInstance();

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

void Tr2LightManager::SetVariableStore()
{
	m_lightBufferVariable = m_lightBuffer;
	m_indexBufferVariable = m_indexBuffer;
	m_ShadowMap.m_atlasVariable = m_ShadowMap.m_atlasDepthStencil;
}

void Tr2LightManager::Clear( Tr2RenderContext& renderContext )
{
	m_lightBufferVariable = m_lightBuffer;
	m_indexBufferVariable = m_indexBuffer;
	m_ShadowMap.m_atlasVariable = m_ShadowMap.m_atlasDepthStencil;
	ClearLightIndices( renderContext );

	for ( auto& data : m_tlsLightData )
	{
		data.clear();
	}
	m_lightData.clear();
	m_shadowCastingLights.clear();
	m_volumetricLights.clear();

	m_ShadowMap.m_atlasNodes.resize(1);
	m_ShadowMap.m_atlasNodes[0].x = 0;
	m_ShadowMap.m_atlasNodes[0].y = 0;
	m_ShadowMap.m_atlasNodes[0].width = m_ShadowMap.m_atlasSettings.size;
	m_ShadowMap.m_atlasNodes[0].height = m_ShadowMap.m_atlasSettings.size;
	m_ShadowMap.m_atlasNodes[0].children[0] = -1;
	m_ShadowMap.m_atlasNodes[0].children[1] = -1;
	m_ShadowMap.m_atlasNodes[0].lightIndex = -1;
}

void Tr2LightManager::SetFrustum( const TriFrustum& frustum )
{
	m_frustum = frustum;
}

void Tr2LightManager::AdjustLightCutoff( float lodFactor )
{
	m_adjustedCutoff = CUTOFF_PIXEL_SIZE * lodFactor;
}

void Tr2LightManager::SetShadowQuality( ShadowQuality shadowQuality, uint64_t frameCounter )
{
	m_currentSpaceSceneShadowQuality = shadowQuality;

	if ( m_currentFrameCounter != frameCounter )
	{
		// there must be a more elegant way of doing this...
		if ( nextFrameShadowQuality & ( 1 << ( uint32_t)ShadowQuality::SHADOW_HIGH ) )
		{
			UpdateShadowAtlasSize( ShadowQuality::SHADOW_HIGH );
		}
		else if( nextFrameShadowQuality & ( 1 << (uint32_t)ShadowQuality::SHADOW_LOW ) )
		{
			UpdateShadowAtlasSize( ShadowQuality::SHADOW_LOW );
		}
		else
		{
			UpdateShadowAtlasSize( ShadowQuality::SHADOW_DISABLED );
		}
		if( ! ( nextFrameShadowQuality & ( 1 << (uint32_t)ShadowQuality::SHADOW_RAYTRACED ) ) )
		{
			UpdateRaytracingDestination( ShadowQuality::SHADOW_DISABLED );
		}
		
		nextFrameShadowQuality = 1 << (uint32_t)shadowQuality;

		m_currentFrameCounter = frameCounter;
	}

	nextFrameShadowQuality |= 1 << (uint32_t)shadowQuality;

	ShadowQuality tmpShadowQuality = (ShadowQuality)min( (uint32_t)shadowQuality, (uint32_t)m_ShadowMap.m_qualityUsedByAtlas );
	m_ShadowMap.m_atlasSettings = CalculateShadowMapAtlasSettings( tmpShadowQuality );
	m_ShadowMap.m_atlasSettings.actualTextureSize = m_ShadowMap.m_atlasDepthStencil ? m_ShadowMap.m_atlasDepthStencil->GetWidth() : 0;
}

void Tr2LightManager::UpdateShadowAtlasSize( ShadowQuality shadowQuality )
{
	if( m_ShadowMap.m_qualityUsedByAtlas != shadowQuality )
	{
		// Setup depth stencil texture
		m_ShadowMap.m_atlasDepthStencil = Tr2DepthStencilPtr();
		m_ShadowMap.m_atlasDepthStencil.CreateInstance();
		if ( shadowQuality != ShadowQuality::SHADOW_DISABLED )
		{
			Tr2LightManager::ShadowMapAtlasSettings settings = CalculateShadowMapAtlasSettings( shadowQuality );
			m_ShadowMap.m_atlasDepthStencil->Create( settings.size, settings.size, Tr2RenderContextEnum::DSFMT_D32F, 0, 0 );
		}
	}
	m_ShadowMap.m_qualityUsedByAtlas = shadowQuality;
}

void Tr2LightManager::UpdateRaytracingDestination( ShadowQuality shadowQuality )
{
	if( ( shadowQuality == ShadowQuality::SHADOW_DISABLED ) && m_Raytracing.m_destTex->IsValid() )
	{
		// Setup depth stencil texture
		m_Raytracing.m_destTex = Tr2RenderTargetPtr();
		m_Raytracing.m_destTex.CreateInstance();
		// Texture is created on the fly in the render function when needed.
	}
}

void Tr2LightManager::AddPointLight( const Vector3& position, float radius, const Color& color, Float_16 innerRadius, uint16_t flags )
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
	if( !m_frustum.IsSphereVisible( reinterpret_cast<Vector4*>( &data.position ) ) )
	{
		return;
	}

	float size = m_frustum.GetPixelSizeAccross( reinterpret_cast<Vector4*>( &data.position ) );

	if( size > m_adjustedCutoff )
	{
		float dimming = std::min( ( size - m_adjustedCutoff ) / FADE_SIZE, 1.f );
		data.color = reinterpret_cast<const Vector3&>( color );
		data.color.x *= radius * dimming;
		data.color.y *= radius * dimming;
		data.color.z *= radius * dimming;
		data.innerRadius = innerRadius;
		data.flags = flags;
		data.direction = Vector3_16( Vector3( 1.f, 0.f, 0.f ) );
		data.outerAngle = Float_16( 0.f );
		data.innerAngle = Float_16( 0.f );
		m_tlsLightData.local().push_back( data );
	}
}

void Tr2LightManager::AddLight( PerLightData& data )
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
	if( !m_frustum.IsSphereVisible( reinterpret_cast<Vector4*>( &data.position ) ) )
	{
		return;
	}

	float size = m_frustum.GetPixelSizeAccross( reinterpret_cast<Vector4*>( &data.position ) );
	if( size > m_adjustedCutoff )
	{
		float dimming = std::min( ( size - m_adjustedCutoff ) / FADE_SIZE, 1.f );
		data.color.x *= data.radius * dimming;
		data.color.y *= data.radius * dimming;
		data.color.z *= data.radius * dimming;

		bool usingShadowMap = m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_LOW || m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_HIGH;
		if( m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_DISABLED || 
			( usingShadowMap && m_ShadowMap.m_qualityUsedByAtlas == ShadowQuality::SHADOW_DISABLED ) )
		{
			data.flags &= ~Tr2LightManager::FLAG_CASTS_SHADOWS;
		}
		m_tlsLightData.local().push_back( data );
	}
}

ALResult Tr2LightManager::ClearLightIndices( Tr2RenderContext& renderContext )
{
	if( !m_indexBuffer || !m_indexBuffer->GetGpuBuffer( 0 ) )
	{
		return S_OK;
	}
	const uint32_t clear[] = { 0, 0, 0, 0 };
	return renderContext.ClearUav( *m_indexBuffer->GetGpuBuffer( 0 ), clear );
}

ALResult Tr2LightManager::UpdateLightBuffer( Tr2RenderContext& renderContext )
{
	if( m_lightBuffer->GetGpuBuffer( 0 )->GetDesc().count < uint32_t( m_lightData.size() ) )
	{
		CR_RETURN_HR( m_lightBuffer->Create(
			std::max( m_lightBuffer->GetGpuBuffer( 0 )->GetDesc().count + 1024, uint32_t( m_lightData.size() ) ),
			sizeof( PerLightData ),
			Tr2GpuBuffer::CPU_WRITABLE ) );
		m_lightBuffer->SetName( "Light buffer" );
	}

	auto lightCount = std::min( m_lightBuffer->GetGpuBuffer( 0 )->GetDesc().count, uint32_t( m_lightData.size() ) );

	Vector4* data;
	CR_RETURN_HR( m_lightBuffer->GetGpuBuffer( 0 )->MapForWriting( data, renderContext ) );
	memcpy( data, m_lightData.data(), lightCount * sizeof( PerLightData ) );
	CCP_STATS_ADD( lightsGathered, m_lightData.size() );
	m_lightBuffer->GetGpuBuffer( 0 )->UnmapForWriting( renderContext );
	return S_OK;
}

ALResult Tr2LightManager::DoUpdateLists(Tr2RenderContext& renderContext )
{
	if( !m_lightBuffer->IsValid() || !m_indexBuffer->IsValid() || !m_indexBufferCounter->IsValid() )
	{
		return E_INVALIDCALL;
	}

	CR_RETURN_HR( UpdateLightBuffer( renderContext ) );

	PerFrameData perFrameData;
	perFrameData.projInverse = Transpose( Tr2Renderer::GetInverseProjectionTransform() );
	perFrameData.viewInverse = Transpose( Tr2Renderer::GetInverseViewTransform() );
	perFrameData.cameraPos = Tr2Renderer::GetViewPosition();
	perFrameData.width = uint32_t( renderContext.m_esm.GetRenderTargetWidth() );
	perFrameData.height = uint32_t( renderContext.m_esm.GetRenderTargetHeight() );
	perFrameData.tilesX = ( perFrameData.width + ( TILE_WIDTH - 1 ) ) / TILE_WIDTH;
	perFrameData.tilesY = ( perFrameData.height + ( TILE_HEIGHT - 1 ) ) / TILE_HEIGHT;
	perFrameData.lightCount = std::min( m_lightBuffer->GetGpuBuffer( 0 )->GetDesc().count, uint32_t( m_lightData.size() ) );
	perFrameData.indexBufferSize = INDEX_BUFFER_SIZE;
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

void Tr2LightManager::CreateShadowMapAtlas( uint32_t numShadowCastingLights, const std::vector<LightScreenSizeTuple>& lightTuples )
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
			Tr2LightManager::PerLightData& lightData = m_lightData[lightIndex];

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
	m_lightData.clear();
	for( auto& data : m_tlsLightData )
	{
		if( !data.empty() )
		{
			m_lightData.insert( end( m_lightData ), begin( data ), end( data ) );
		}
		data.clear();
	}

	m_shadowCastingLights.clear();
	m_volumetricLights.clear();

	if( m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_DISABLED )
	{
		return;
	}

	// filter volumetric lights
	std::vector<LightScreenSizeTuple> lightTuples;
	{
		for( uint32_t i = 0; i < m_lightData.size(); i++ )
		{
			if( ( m_lightData[i].flags & FLAG_IS_VOLUMETRIC ) != 0 )
			{
				float sizeAcross = m_frustum.GetPixelSizeAccrossEst( reinterpret_cast<Vector4*>( &m_lightData[i].position ) );
				sizeAcross = sizeAcross == std::numeric_limits<float>::max() ? ( 1 << HIGH_QUALITY_ATLAS_SIZE_LOG2 ) : sizeAcross;
				lightTuples.push_back( LightScreenSizeTuple{ i, sizeAcross } );
			}
		}

		// sort volumetric lights by size on screen
		std::sort( lightTuples.begin(), lightTuples.end(), []( const LightScreenSizeTuple& a, const LightScreenSizeTuple& b ) {
			return a.sizeAcross > b.sizeAcross;
		} );

		// keep only the largest lights
		uint32_t numVolumetricLights = min( MAX_NUM_VOLUMETRIC_LIGHTS, (uint32_t)lightTuples.size() );
		m_volumetricLights.resize( numVolumetricLights );
		uint32_t i = 0;
		for( ; i < numVolumetricLights; i++ )
		{
			m_volumetricLights[i] = lightTuples[i].lightIndex;
		}
		for( ; i < lightTuples.size(); i++ )
		{
			Tr2LightManager::PerLightData& lightData = m_lightData[lightTuples[i].lightIndex];
			lightData.flags &= ~Tr2LightManager::FLAG_IS_VOLUMETRIC;
		}
	}

	// filter shadowcasting lights
	uint32_t numShadowCastingLights = 0;
	lightTuples.resize( 0 );
	{
		for( uint32_t i = 0; i < m_lightData.size(); i++ )
		{
			if( ( m_lightData[i].flags & FLAG_CASTS_SHADOWS ) != 0 )
			{
				float sizeAcross = m_frustum.GetPixelSizeAccrossEst( reinterpret_cast<Vector4*>( &m_lightData[i].position ) );
				sizeAcross = sizeAcross == std::numeric_limits<float>::max() ? ( 1 << HIGH_QUALITY_ATLAS_SIZE_LOG2 ) : sizeAcross;
				lightTuples.push_back( LightScreenSizeTuple{ i, sizeAcross } );
			}
		}

		// sort shadowcasting lights by size on screen
		std::sort( lightTuples.begin(), lightTuples.end(), []( const LightScreenSizeTuple& a, const LightScreenSizeTuple& b ) {
			return a.sizeAcross > b.sizeAcross;
		} );

		// keep only the largest lights
		numShadowCastingLights = min( MAX_NUM_SHADOWCASTING_LIGHTS, (uint32_t)lightTuples.size() );
		m_shadowCastingLights.resize( numShadowCastingLights );
		uint32_t i = 0;
		for( ; i < numShadowCastingLights; i++ )
		{
			m_shadowCastingLights[i] = lightTuples[i].lightIndex;
			if( m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_RAYTRACED )
			{
				Tr2LightManager::PerLightData& lightData = m_lightData[lightTuples[i].lightIndex];
				lightData.Raytracing.shadowMask = 1 << i;
			}
		}
		for( ; i < lightTuples.size(); i++ )
		{
			Tr2LightManager::PerLightData& lightData = m_lightData[lightTuples[i].lightIndex];
			lightData.flags &= ~Tr2LightManager::FLAG_CASTS_SHADOWS;
			if( m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_RAYTRACED )
			{
				lightData.Raytracing.shadowMask = 0;
			}
		}
	}

	if( m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_LOW || m_currentSpaceSceneShadowQuality == ShadowQuality::SHADOW_HIGH )
	{
		CreateShadowMapAtlas( numShadowCastingLights, lightTuples );
	}
}

ALResult Tr2LightManager::UpdateLists( Tr2RenderContext& renderContext )
{
	m_lightBufferVariable = m_lightBuffer;
	m_indexBufferVariable = m_indexBuffer;
	m_ShadowMap.m_atlasVariable = m_ShadowMap.m_atlasDepthStencil;

	if( m_lightData.empty() )
	{
		return ClearLightIndices( renderContext );
	}

	auto hr = DoUpdateLists( renderContext );
	if( FAILED( hr ) )
	{
		ClearLightIndices( renderContext );
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

	m_lightBuffer->Create( LIGHT_BUFFER_SIZE, sizeof( PerLightData ), Tr2GpuBuffer::CPU_WRITABLE );
	m_lightBuffer->SetName( "Light buffer" );
	m_indexBuffer->Create( INDEX_BUFFER_SIZE, sizeof( uint32_t ), Tr2GpuStructuredBuffer::GPU_WRITABLE );
	m_indexBuffer->SetName( "Light indices" );
	m_indexBufferCounter->Create( 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32_SINT, Tr2GpuBuffer::GPU_WRITABLE );
	ClearLightIndices( renderContext );
	m_perFrameData.Create( sizeof( PerFrameData ), Tr2ConstantUsageAL::ONE_SHOT, nullptr, renderContext );

	m_effect->SetParameter( BlueSharedString( "LightBuffer" ), m_lightBuffer );
	m_effect->SetParameter( BlueSharedString( "LightIndices" ), m_indexBuffer );
	m_effect->SetParameter( BlueSharedString( "LightIndexCount" ), m_indexBufferCounter );

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
	return m_shadowCastingLights;
}

const std::vector<uint32_t>& Tr2LightManager::GetVolumetricLights() const
{
	return m_volumetricLights;
}

const Tr2LightManager::PerLightData& Tr2LightManager::GetLightData( uint32_t index ) const
{
	return m_lightData[index];
}

Tr2DepthStencilPtr Tr2LightManager::GetShadowMapAtlas()
{
	return m_ShadowMap.m_atlasDepthStencil;
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
        
        if (deltaWidth > deltaHeight)
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

ITr2TextureProvider* Tr2LightManager::GetRaytracedShadowMap() const
{
	return m_Raytracing.m_destTex;
}

void Tr2LightManager::RenderRaytracedShadows( Tr2RaytracingGeometryPtr geometry, ITr2TextureProvider* depth, ITr2TextureProvider* normal, const CcpMath::Sphere* planets, size_t planetCount, Tr2RenderContext& renderContext )
{
	renderContext.AddGpuMarker( __FUNCTION__ );
	GPU_REGION( renderContext, "Raytraced dynamic shadows" );
	CCP_STATS_ZONE( __FUNCTION__ );

	if ( m_shadowCastingLights.size() == 0 )
	{
		return;
	}

	auto depthTex = depth->GetTexture();
	if( !depthTex )
	{
		return;
	}

	if( !m_Raytracing.m_destTex->IsValid() || m_Raytracing.m_destTex->GetWidth() != depthTex->GetWidth() || m_Raytracing.m_destTex->GetHeight() != depthTex->GetHeight() )
	{
		// Create the output resource. The dimensions and format should match the swap-chain
		if( FAILED( m_Raytracing.m_destTex->Create( depthTex->GetWidth(), depthTex->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, 1, 0, Tr2RenderContextEnum::EX_BIND_UNORDERED_ACCESS ) ) )
		{
			return;
		}
		m_Raytracing.m_destTex->SetName( "raytracing_dynamic_shadow_dest" );
	}

	// texture uav
	m_Raytracing.m_effect->SetParameter( RtShadowMapTechniqueName, m_Raytracing.m_destTex );
	m_Raytracing.m_effect->SetParameter( RtNormalBufferTechniqueName, normal );

	// scene srv
	m_Raytracing.m_effect->SetParameter( RtSceneTechniqueName, geometry );

	if( !m_Raytracing.m_effect->GetEffectRes() || !m_Raytracing.m_effect->GetEffectRes()->IsGood() )
	{
		return;
	}

	if( !geometry->HasGeometry() )
	{
		return;
	}

	uint32_t techniqueIndex;
	if( !m_Raytracing.m_effect->GetShaderStateInterface()->GetTechniqueIndex( RtShadowTechniqueName, techniqueIndex ) )
	{
		return;
	}

	std::wstring rayGenName, missName;
	m_Raytracing.m_pipelineManager.AddLibrary( rayGenName, missName, m_Raytracing.m_effect, RtShadowTechniqueName );

	auto pipelineState = m_Raytracing.m_pipelineManager.GetPipelineState( renderContext );

	if( !pipelineState.IsValid() )
	{
		return;
	}

	if( !m_Raytracing.m_perFrameData.IsValid() )
	{
		m_Raytracing.m_perFrameData.Create( sizeof( RtShadowPerFrameData ), renderContext.GetPrimaryRenderContext() );
	}

	auto destTex = m_Raytracing.m_destTex->GetTexture();
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

		for( uint32_t j = 0; j < m_shadowCastingLights.size(); j++ )
		{
			uint32_t lightIndex = m_shadowCastingLights[j];
			PerLightData& lightData = m_lightData[lightIndex];
			data->dynamicLights[j] = lightData;
		}
		data->numDynamicLights = (uint32_t)m_shadowCastingLights.size();

		m_Raytracing.m_perFrameData.Unlock( renderContext );
	}

	{
		CCP_STATS_ZONE( "Create shader table" );
		Tr2RtLocalMaterialDescriptionAL material;
		material.SetConstants( 7, m_Raytracing.m_perFrameData );

		m_Raytracing.m_shaderTableDesc.AddRayGenShader( rayGenName.c_str(), material );
		m_Raytracing.m_shaderTableDesc.AddMissShader( missName.c_str(), material );
		m_Raytracing.m_shaderTable.Create( m_Raytracing.m_shaderTableDesc, pipelineState, renderContext.GetPrimaryRenderContext() );
	}

	const uint32_t clearValue[] = { 0, 0, 0, 0 };
	renderContext.ClearUav( *m_Raytracing.m_destTex->GetTexture(), 0, clearValue );

	m_Raytracing.m_effect->ApplyMaterialDataForRtState( techniqueIndex, pipelineState, renderContext );
	renderContext.UseAccelerationStructure( geometry->GetTLAS() );
	renderContext.DispatchRays( pipelineState, m_Raytracing.m_shaderTable, rayGenName.c_str(), destTex->GetWidth(), destTex->GetHeight(), 1 );
	GlobalStore().RegisterVariable( "EveSpaceSceneDynamicShadowMap", m_Raytracing.m_destTex );
}
