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
	case ShadowQuality::SHADOW_RAYTRACED:
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
	m_shadowMapAtlasVariable.Register( "ShadowMapAtlas", m_shadowMapAtlasDS );

	m_qualityUsedByShadowAtlas = ShadowQuality::SHADOW_DISABLED;
	m_currentSpaceSceneShadowQuality = ShadowQuality::SHADOW_DISABLED;

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
	m_shadowMapAtlasVariable = m_shadowMapAtlasDS;
}

void Tr2LightManager::Clear( Tr2RenderContext& renderContext )
{
	m_lightBufferVariable = m_lightBuffer;
	m_indexBufferVariable = m_indexBuffer;
	m_shadowMapAtlasVariable = m_shadowMapAtlasDS;
	ClearLightIndices( renderContext );

	for( auto& data : m_tlsLightData )
	{
		data.clear();
	}
	m_lightData.clear();
	m_shadowCastingLights.clear();
	m_volumetricLights.clear();

	m_shadowMapNodes.resize( 1 );
	m_shadowMapNodes[0].x = 0;
	m_shadowMapNodes[0].y = 0;
	m_shadowMapNodes[0].width = m_shadowMapAtlasSettings.size;
	m_shadowMapNodes[0].height = m_shadowMapAtlasSettings.size;
	m_shadowMapNodes[0].children[0] = -1;
	m_shadowMapNodes[0].children[1] = -1;
	m_shadowMapNodes[0].lightIndex = -1;
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
		UpdateShadowAtlasSize( m_nextFrameQuality );
		m_nextFrameQuality = shadowQuality;
		m_currentFrameCounter = frameCounter;
	}
	m_nextFrameQuality = (ShadowQuality)max( (uint32_t)shadowQuality, (uint32_t)m_nextFrameQuality );
	
	ShadowQuality tmpShadowQuality = (ShadowQuality)min( (uint32_t)shadowQuality, (uint32_t)m_qualityUsedByShadowAtlas );
	m_shadowMapAtlasSettings = CalculateShadowMapAtlasSettings( tmpShadowQuality );
	m_shadowMapAtlasSettings.actualTextureSize = m_shadowMapAtlasDS ? m_shadowMapAtlasDS->GetWidth() : 0;
}

void Tr2LightManager::UpdateShadowAtlasSize( ShadowQuality shadowQuality )
{
	if( m_qualityUsedByShadowAtlas != shadowQuality )
	{
		// Setup depth stencil texture
		m_shadowMapAtlasDS = Tr2DepthStencilPtr();
		m_shadowMapAtlasDS.CreateInstance();
		if ( shadowQuality != ShadowQuality::SHADOW_DISABLED )
		{
			Tr2LightManager::ShadowMapAtlasSettings settings = CalculateShadowMapAtlasSettings( shadowQuality );
			m_shadowMapAtlasDS->Create( settings.size, settings.size, Tr2RenderContextEnum::DSFMT_D32F, 0, 0 );
		}
	}
	m_qualityUsedByShadowAtlas = shadowQuality;
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

		if( m_shadowMapAtlasSettings.size == 0 || m_qualityUsedByShadowAtlas == ShadowQuality::SHADOW_DISABLED )
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

	// filter all shadowcasting and volumetric lights
	struct LightScreenSizeTuple
	{
		uint32_t lightIndex;
		float sizeAcross;
	};

	// filter volumetric lights
	std::vector<LightScreenSizeTuple> lightTuples;
	{
		for( uint32_t i = 0; i < m_lightData.size(); i++ )
		{
			if( ( m_lightData[i].flags & FLAG_IS_VOLUMETRIC ) != 0 )
			{
				float sizeAcross = m_frustum.GetPixelSizeAccrossEst( reinterpret_cast<Vector4*>( &m_lightData[i].position ) );
				sizeAcross = sizeAcross == std::numeric_limits<float>::max() ? m_shadowMapAtlasSettings.size : sizeAcross;
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
		}
		for( ; i < lightTuples.size(); i++ )
		{
			Tr2LightManager::PerLightData& lightData = m_lightData[lightTuples[i].lightIndex];
			lightData.flags &= ~Tr2LightManager::FLAG_CASTS_SHADOWS;
		}
	}

	bool everythingFit = false;
	// 5 iterations are more than enough. With the current values we should actually only need at most 4 iterations.
	for( uint32_t j = 0; j < 5 && !everythingFit; j++ )
	{
		everythingFit = true;
		uint32_t entryInverseScaleFactorLog2 = m_shadowMapAtlasSettings.entryInverseScaleFactorLog2 + j;
		uint32_t entryMaxSize = m_shadowMapAtlasSettings.entryMaxSize >> j;

		// clear atlas entries
		m_shadowMapNodes.resize( 1 );
		m_shadowMapNodes[0].x = 0;
		m_shadowMapNodes[0].y = 0;
		m_shadowMapNodes[0].width = m_shadowMapAtlasSettings.size;
		m_shadowMapNodes[0].height = m_shadowMapAtlasSettings.size;
		m_shadowMapNodes[0].children[0] = -1;
		m_shadowMapNodes[0].children[1] = -1;
		m_shadowMapNodes[0].lightIndex = -1;

		// make entries into the shadow map atlas
		for( uint32_t i = 0; i < numShadowCastingLights; i++ )
		{
			uint32_t lightIndex = lightTuples[i].lightIndex;
			Tr2LightManager::PerLightData& lightData = m_lightData[lightIndex];

			uint32_t size = ( (uint32_t)lightTuples[i].sizeAcross ) >> entryInverseScaleFactorLog2;
			size = ClampUInt( size, m_shadowMapAtlasSettings.entryMinSize, entryMaxSize );
			size = CCP_ALIGN( size, m_shadowMapAtlasSettings.entryMinSize );

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
				lightData.shadowMapOffsetX = offsetX >> m_shadowMapAtlasSettings.entryMinSizeLog2;
				lightData.shadowMapOffsetY = offsetY >> m_shadowMapAtlasSettings.entryMinSizeLog2;
				lightData.shadowMapScale = size >> m_shadowMapAtlasSettings.entryMinSizeLog2;
			}
			else
			{
				lightData.shadowMapOffsetX = 0;
				lightData.shadowMapOffsetY = 0;
				lightData.shadowMapScale = 0;
				everythingFit = false;
			}
		}
	}
	CCP_ASSERT_M( everythingFit, "Not all entries fit into the shadow map atlas!" );
}

ALResult Tr2LightManager::UpdateLists( Tr2RenderContext& renderContext )
{
	m_lightBufferVariable = m_lightBuffer;
	m_indexBufferVariable = m_indexBuffer;
	m_shadowMapAtlasVariable = m_shadowMapAtlasDS;

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
	return m_shadowMapAtlasDS;
}

const Tr2LightManager::ShadowMapAtlasSettings& Tr2LightManager::GetShadowMapAtlasSettings() const
{
	return m_shadowMapAtlasSettings;
}

ShadowQuality Tr2LightManager::GetCurrentSpaceSceneShadowQuality()
{
	return m_currentSpaceSceneShadowQuality;
}

// based on: https://blackpawn.com/texts/lightmaps/default.html
uint32_t Tr2LightManager::InsertShadowMapNode( uint32_t nodeId, uint32_t lightIndex, int32_t width, int32_t height )
{
	if( m_shadowMapNodes[nodeId].children[0] != -1 || m_shadowMapNodes[nodeId].children[1] != -1 )
	{
		uint32_t newNode = InsertShadowMapNode( m_shadowMapNodes[nodeId].children[0], lightIndex, width, height );
		if( newNode != -1 )
		{
			return newNode;
		}
		return InsertShadowMapNode( m_shadowMapNodes[nodeId].children[1], lightIndex, width, height );
	}
	else
    {
		if( m_shadowMapNodes[nodeId].lightIndex != -1 )
		{
			return -1;
		}
		if( m_shadowMapNodes[nodeId].width < width || m_shadowMapNodes[nodeId].height < height )
		{
			return -1;
		}
        if( m_shadowMapNodes[nodeId].width == width && m_shadowMapNodes[nodeId].height == height )
		{
			m_shadowMapNodes[nodeId].lightIndex = lightIndex;
			return nodeId;
		}
		
        m_shadowMapNodes.insert( m_shadowMapNodes.end(), 2, ShadowMapNode() );

		m_shadowMapNodes[nodeId].children[0] = uint32_t(m_shadowMapNodes.size() - 2);
		m_shadowMapNodes[nodeId].children[1] = uint32_t(m_shadowMapNodes.size() - 1);

		ShadowMapNode& child0 = m_shadowMapNodes[m_shadowMapNodes[nodeId].children[0]];
		ShadowMapNode& child1 = m_shadowMapNodes[m_shadowMapNodes[nodeId].children[1]];
        
        int32_t deltaWidth = m_shadowMapNodes[nodeId].width - width;
		int32_t deltaHeight = m_shadowMapNodes[nodeId].height - height;
        
        if (deltaWidth > deltaHeight)
		{
			child0.x = m_shadowMapNodes[nodeId].x;
			child0.y = m_shadowMapNodes[nodeId].y;
			child0.width = width;
			child0.height = m_shadowMapNodes[nodeId].height;
			child1.x = m_shadowMapNodes[nodeId].x + width;
			child1.y = m_shadowMapNodes[nodeId].y;
			child1.width = m_shadowMapNodes[nodeId].width - width;
			child1.height = m_shadowMapNodes[nodeId].height;
		}
        else
		{
			child0.x = m_shadowMapNodes[nodeId].x;
			child0.y = m_shadowMapNodes[nodeId].y;
			child0.width = m_shadowMapNodes[nodeId].width;
			child0.height = height;
			child1.x = m_shadowMapNodes[nodeId].x;
			child1.y = m_shadowMapNodes[nodeId].y + height;
			child1.width = m_shadowMapNodes[nodeId].width;
			child1.height = m_shadowMapNodes[nodeId].height - height;
		}
		child0.lightIndex = -1;
		child0.children[0] = -1;
		child0.children[1] = -1;
		child1.lightIndex = -1;
		child1.children[0] = -1;
		child1.children[1] = -1;

		return InsertShadowMapNode( m_shadowMapNodes[nodeId].children[0], lightIndex, width, height );
	 }
}

bool Tr2LightManager::GetShadowMapAtlasEntry( uint32_t lightIndex, uint32_t width, uint32_t height, uint32_t& out_posX, uint32_t& out_posY )
{
	width = CCP_ALIGN( width, m_shadowMapAtlasSettings.entryMinSize );
	height = CCP_ALIGN( height, m_shadowMapAtlasSettings.entryMinSize );

	uint32_t nodeId = InsertShadowMapNode( 0, lightIndex, (int32_t)width, (int32_t)height );
	//CCP_ASSERT_M( nodeId != -1, "Shadow map atlas could not fit the requested entry." );

	if( nodeId != -1 )
	{
		out_posX = m_shadowMapNodes[nodeId].x;
		out_posY = m_shadowMapNodes[nodeId].y;

		CCP_ASSERT_M( ( out_posX & ( m_shadowMapAtlasSettings.entryMinSize - 1 ) ) == 0, "Shadow map atlas entry posX is not aligned!" );
		CCP_ASSERT_M( ( out_posY & ( m_shadowMapAtlasSettings.entryMinSize - 1 ) ) == 0, "Shadow map atlas entry posY is not aligned!" );
	}

	return nodeId != -1;
}

void Tr2LightManager::GetUnpackedShadowMapData( const PerLightData& lightData, uint32_t& shadowMapScale, uint32_t& shadowMapOffsetX, uint32_t& shadowMapOffsetY ) const
{
	shadowMapScale = lightData.shadowMapScale << m_shadowMapAtlasSettings.entryMinSizeLog2;
	shadowMapOffsetX = lightData.shadowMapOffsetX << m_shadowMapAtlasSettings.entryMinSizeLog2;
	shadowMapOffsetY = lightData.shadowMapOffsetY << m_shadowMapAtlasSettings.entryMinSizeLog2;
}