////////////////////////////////////////////////////////////
//
//    Created:   February 2023
//    Copyright: CCP 2023
//

#include "StdAfx.h"
#include "Tr2VolumetricsRenderer.h"
#include "ITr2VolumetricRenderable.h"
#include "Tr2Renderer.h"
#include "Tr2VariableStore.h"
#include "Tr2TextureReference.h"
#include "Shader/Tr2Effect.h"
#include "TriRenderBatch.h"
#include "Tr2RenderTarget.h"
#include "Tr2DepthStencil.h"
#include "Eve/SpaceObject/Children/EveChildCloud2.h"
#include "PriorityBlend.h"


ITr2FroxelFogSettings::FroxelFogSettings ITr2FroxelFogSettings::FroxelFogSettings::operator*( float rhs ) const
{
	FroxelFogSettings result;
	result.thickness = thickness * rhs;
	result.directionality = directionality * rhs;
	result.environmentIntensity = environmentIntensity * rhs;
	result.fogColor = fogColor * rhs;
	result.backgroundColor = backgroundColor * rhs;
	return result;

}

ITr2FroxelFogSettings::FroxelFogSettings ITr2FroxelFogSettings::FroxelFogSettings::operator+(const FroxelFogSettings& rhs) const
{
	FroxelFogSettings result;
	result.thickness = thickness + rhs.thickness;
	result.directionality = directionality + rhs.directionality;
	result.environmentIntensity = environmentIntensity + rhs.environmentIntensity;
	result.fogColor = fogColor + rhs.fogColor;
	result.backgroundColor = backgroundColor + rhs.backgroundColor;
	return result;
}


Tr2VolumetricsRenderer::FogViewDependentResources::FogViewDependentResources( bool temporalFroxels )
{
	fogFroxels.CreateInstance();
	if( temporalFroxels )
	{
		temporalFroxels0.CreateInstance();
		temporalFroxels1.CreateInstance();
	}
	currentTemporalFroxels = false;
	froxelJitter = Vector3( 0, 0, 0 );

	calculateFroxels.CreateInstance();
	calculateFroxels->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/Fog/CalculateFroxels.fx" );

	filterFroxels.CreateInstance();
	filterFroxels->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/Fog/FilterFroxels.fx" );

	raymarchFroxels.CreateInstance();
	raymarchFroxels->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/Fog/RaymarchFroxels.fx" );
	raymarchFroxels->SetOption( BlueSharedString( "INPUT_SOURCE" ), temporalFroxels ? BlueSharedString( "INPUT_SOURCE_INPUT_TEXTURE" ) : BlueSharedString( "INPUT_SOURCE_OUTPUT_TEXTURE" ) );

	applyFroxels.CreateInstance();
	applyFroxels->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/Fog/ApplyFroxels.fx" );
}


Tr2VolumetricsRenderer::Tr2VolumetricsRenderer( IRoot* ) :
	m_quality( Tr2VolumerticQuality::High ),
	m_scaleFactor( 0.7f ),
	m_blur( true ),
	m_volumeHasContent( false ),
	m_castShadows( false ),
	m_receiveShadows( false ),
	m_fogResources( true ),
	m_fogReflectionResources( false )
{

	m_volumeSlices.CreateInstance();
	GlobalStore().RegisterVariable( "EveSceneFogVolumeMap", m_volumeSlices );

	m_downsampledDepth.CreateInstance();
	GlobalStore().RegisterVariable( "VolumetricDepthMap", m_downsampledDepth );

	m_blurScratch.CreateInstance();


	m_volumeBlit.CreateInstance();
	m_volumeBlit->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/VolumeBlit.fx" );

	m_downsampleDepth.CreateInstance();
	m_downsampleDepth->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/DownsampleDepth.fx" );

	m_hBlur.CreateInstance();
	m_hBlur->SetOption( BlueSharedString( "SOURCE_TYPE" ), BlueSharedString( "SOURCE_TYPE_ARRAY" ) );
	m_hBlur->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/BlurVolumetric.fx" );

	m_vBlur.CreateInstance();
	m_vBlur->SetOption( BlueSharedString( "SOURCE_TYPE" ), BlueSharedString( "SOURCE_TYPE_TEXTURE" ) );
	m_vBlur->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/BlurVolumetric.fx" );
	m_vBlur->SetParameter( BlueSharedString( "SourceMap" ), m_blurScratch );

	m_batches.reset( new TriRenderBatchAccumulator<>( Tr2Renderer::GetPoolAllocator() ) );

	m_gameBackClip = 1E6f; //must match what the actual game uses; not what Graphite is set to.

	m_froxelFogSettings.thickness = 0.0f;
	m_froxelFogSettings.directionality = 0.5f;
	m_froxelFogSettings.environmentIntensity = 1.0f;
	m_froxelFogSettings.fogColor = Color( 1.0f, 1.0f, 1.0f, 1.0f );
	m_froxelFogSettings.backgroundColor = Color( 0.0f, 0.0f, 0.0f, 1.0f );


	{
		m_mieEnvironmentMap.CreateInstance();
		GlobalStore().RegisterVariable( "EveSceneMieEnvironmentMap", m_mieEnvironmentMap );
		m_environmentJitter = Vector2( 0, 0 );
		m_environmentRandom = 0.0f;
		m_environmentDirectionality = 0.0f;
		m_environmentBlendCounter = 0.0f;

		GlobalStore().RegisterVariable( "EveSceneDistanceFogMap", m_fogResources.fogFroxels );

	}

	{
		m_updateMieEnvironmentMap.CreateInstance();
		m_updateMieEnvironmentMap->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/Fog/UpdateMieEnvironmentMap.fx" );
	}
}

void Tr2VolumetricsRenderer::RenderVolumetrics(
	const EveComponentRegistry& registry,
	const TriFrustum& frustum,
	Tr2DepthStencil& sceneDepth,
	const Vector3& sunDirection,
	const float depthSlices[4],
	Tr2RenderContext& renderContext )
{
	uint32_t slices = 4;

	auto originalWidth = sceneDepth.GetWidth();
	auto originalHeight = sceneDepth.GetHeight();

	auto width = std::min( originalWidth, std::max( 1u, uint32_t( originalWidth * m_scaleFactor ) ) );
	auto height = std::min( originalHeight, std::max( 1u, uint32_t( originalHeight * m_scaleFactor ) ) );

	auto& volumeSlices = *m_volumeSlices->GetTexture();

	auto volumetricsCount = registry.ComponentCount<ITr2VolumetricRenderable>();

	if( volumetricsCount == 0 )
	{
		if( !volumeSlices.IsValid() )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			uint8_t black[4] = {};
			Tr2SubresourceData initialData[4];
			for( uint32_t i = 0; i < 4; ++i )
			{
				initialData[i].m_sysMem = black;
				initialData[i].m_sysMemPitch = 4;
				initialData[i].m_sysMemSlicePitch = 4;
			}
			volumeSlices.Create( Tr2BitmapDimensions(
									 Tr2RenderContextEnum::TEX_TYPE_2D,
									 Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM,
									 1,
									 1,
									 1,
									 1,
									 slices ),
								 Tr2GpuUsage::SHADER_RESOURCE,
								 Tr2CpuUsage::NONE,
								 initialData,
								 renderContext );
			m_volumeSlices->OnTextureChange().Broadcast();
			m_volumeHasContent = false;
		}
		if( m_volumeHasContent && volumeSlices.IsValid() )
		{
			renderContext.m_esm.PushRenderTarget( 0 );
			renderContext.m_esm.PushRenderTarget( 1 );
			renderContext.m_esm.PushRenderTarget( 2 );
			renderContext.m_esm.PushRenderTarget( 3 );
			renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );

			renderContext.m_esm.SetRenderTarget( 0, volumeSlices, true, 0 );
			renderContext.m_esm.SetRenderTarget( 1, volumeSlices, true, 1 );
			renderContext.m_esm.SetRenderTarget( 2, volumeSlices, true, 2 );
			renderContext.m_esm.SetRenderTarget( 3, volumeSlices, true, 3 );

			renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 0, 0, 0 );
			renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 0, 0, 1 );
			renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 0, 0, 2 );
			renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 0, 0, 3 );
		
			renderContext.m_esm.PopRenderTarget( 3 );
			renderContext.m_esm.PopRenderTarget( 2 );
			renderContext.m_esm.PopRenderTarget( 1 );
			renderContext.m_esm.PopRenderTarget( 0 );
			renderContext.m_esm.PopDepthStencilBuffer();
			m_volumeHasContent = false;
		}
		return;
	}

	CCP_STATS_ZONE( __FUNCTION__ );

	renderContext.AddGpuMarker( __FUNCTION__ );
	GPU_REGION( renderContext, "Volumetrics" );

	ITr2VolumetricRenderable::SceneInformation sceneInfo;
	sceneInfo.quality = m_quality;
	std::copy_n( depthSlices, 4, sceneInfo.depthSlices );
	sceneInfo.targetWidth = width;
	sceneInfo.targetHeight = height;
	sceneInfo.sunDirection = sunDirection;
	sceneInfo.receiveShadows = m_receiveShadows;
	sceneInfo.castShadows = m_castShadows;

	registry.ProcessComponents<ITr2VolumetricRenderable>( [&sceneInfo] ( ITr2VolumetricRenderable* volumetric ) -> void {
		volumetric->SetSceneInformation( sceneInfo );
		} );

	{
		GPU_REGION( renderContext, "Lightmaps" );
		registry.ProcessComponentsUntil<ITr2VolumetricRenderable>( [&renderContext] ( ITr2VolumetricRenderable* volumetric ) -> bool {
			return volumetric->UpdateVolumetricLightmap( renderContext );
			} );
	}
	if( !volumeSlices.IsValid() || volumeSlices.GetWidth() != width || volumeSlices.GetHeight() != height )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		volumeSlices.Create( Tr2BitmapDimensions(
								 Tr2RenderContextEnum::TEX_TYPE_2D,
								 Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT,
								 width,
								 height,
								 1,
								 1,
								 slices ),
							 Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE,
							 renderContext );
		m_volumeSlices->OnTextureChange().Broadcast();
		if( !volumeSlices.IsValid() )
		{
			CCP_LOGERR( "Tr2VolumetricsRenderer failed to create slices texture with size %ix%ix%i", int( width ), int( height ), int( slices ) );
			return;
		}
	}

	if( m_blur )
	{
		if( !m_blurScratch->IsValid() || m_blurScratch->GetWidth() != width || m_blurScratch->GetHeight() != height )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			m_blurScratch->Create( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT );
			if( !m_blurScratch->IsValid() )
			{
				CCP_LOGERR( "Tr2VolumetricsRenderer failed to create blur scratch texture with size %ix%i", int( width ), int( height ) );
				return;
			}
		}
	}
	else
	{
		m_blurScratch->Destroy();
	}

	renderContext.m_esm.PushRenderTarget( 0 );
	renderContext.m_esm.PushRenderTarget( 1 );
	renderContext.m_esm.PushRenderTarget( 2 );
	renderContext.m_esm.PushRenderTarget( 3 );
	renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );

	if( originalWidth == width && originalHeight == height )
	{
		GlobalStore().RegisterVariable( "VolumetricDepthMap", &sceneDepth );
		m_volumeBlit->SetOption( BlueSharedString( "CLOUD_UPSAMPLING" ), BlueSharedString( "CLOUD_UPSAMPLING_NONE" ) );
		if( m_downsampledDepth->IsValid() )
		{
			m_downsampledDepth->Destroy();
		}
	}
	else
	{
		m_volumeBlit->SetOption( BlueSharedString( "CLOUD_UPSAMPLING" ), BlueSharedString( "CLOUD_UPSAMPLING_BILINEAR" ) );

		if( !m_downsampledDepth->IsValid() || m_downsampledDepth->GetWidth() != width || m_downsampledDepth->GetHeight() != height )
		{
			if( FAILED( m_downsampledDepth->Create( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT ) ) )
			{
				CCP_LOGERR( "Tr2VolumetricsRenderer failed to create depth map with size %ix%i", int( width ), int( height ) );
				renderContext.m_esm.PopRenderTarget( 3 );
				renderContext.m_esm.PopRenderTarget( 2 );
				renderContext.m_esm.PopRenderTarget( 1 );
				renderContext.m_esm.PopRenderTarget( 0 );
				renderContext.m_esm.PopDepthStencilBuffer();
				return;
			}
		}
		GlobalStore().RegisterVariable( "VolumetricDepthMap", m_downsampledDepth );

		renderContext.m_esm.SetRenderTarget( 0, *m_downsampledDepth );
		renderContext.m_esm.SetRenderTarget( 1, Tr2TextureAL() );
		m_downsampleDepth->SetParameter( BlueSharedString( "DepthSizes" ), Vector4( float( width ), float( height ), float( originalWidth ), float( originalHeight ) ) );
		Tr2GpuProfiler::GetProfiler().Begin( m_downsampleDepth->GetRawRoot(), "Pass #1", renderContext );
		Tr2Renderer::DrawScreenQuad( renderContext, m_downsampleDepth );
		Tr2GpuProfiler::GetProfiler().End( renderContext );
	}

	renderContext.m_esm.SetRenderTarget( 0, volumeSlices, true, 0 );
	renderContext.m_esm.SetRenderTarget( 1, volumeSlices, true, 1 );
	renderContext.m_esm.SetRenderTarget( 2, volumeSlices, true, 2 );
	renderContext.m_esm.SetRenderTarget( 3, volumeSlices, true, 3 );

	renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 0, 0, 0 );
	renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 0, 0, 1 );
	renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 0, 0, 2 );
	renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 0, 0, 3 );

	std::vector<std::pair<ITr2VolumetricRenderable*, float>> renderables;
	renderables.reserve( volumetricsCount );
	registry.ProcessComponents<ITr2VolumetricRenderable>( [&renderables, &frustum] ( ITr2VolumetricRenderable* renderable ) -> void {
		renderables.push_back( { renderable, renderable->GetSortValue( frustum ) } );
		} );

	std::stable_sort( begin( renderables ), end( renderables ), []( auto x, auto y ) { return x.second > y.second; } );

	for( auto& ref : renderables )
	{
		ref.first->GetVolumetricBatches( frustum, m_batches.get() );
	}

	if( m_batches->GetBatchCount() )
	{
		m_batches->Finalize();
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
		renderContext.RenderBatches( m_batches.get() );
		m_batches->Clear();
	}

	if( m_blur )
	{
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
		renderContext.m_esm.SetRenderTarget( 0, *m_blurScratch );
		renderContext.m_esm.SetRenderTarget( 1, Tr2TextureAL() );
		renderContext.m_esm.SetRenderTarget( 2, Tr2TextureAL() );
		renderContext.m_esm.SetRenderTarget( 3, Tr2TextureAL() );
		m_hBlur->SetParameter( BlueSharedString( "DepthSizes" ), Vector4( float( width ), float( height ), 1.f / float( width ), 0 ) );
		Tr2GpuProfiler::GetProfiler().Begin( m_hBlur->GetRawRoot(), "Pass #1", renderContext );
		Tr2Renderer::DrawScreenQuad( renderContext, m_hBlur );
		Tr2GpuProfiler::GetProfiler().End( renderContext );

		renderContext.m_esm.SetRenderTarget( 0, volumeSlices, true, 3 );
		m_vBlur->SetParameter( BlueSharedString( "DepthSizes" ), Vector4( float( width ), float( height ), 0, 1.f / float( height ) ) );
		Tr2GpuProfiler::GetProfiler().Begin( m_vBlur->GetRawRoot(), "Pass #1", renderContext );
		Tr2Renderer::DrawScreenQuad( renderContext, m_vBlur );
		Tr2GpuProfiler::GetProfiler().End( renderContext );
	}

	renderContext.m_esm.PopRenderTarget( 3 );
	renderContext.m_esm.PopRenderTarget( 2 );
	renderContext.m_esm.PopRenderTarget( 1 );
	renderContext.m_esm.PopRenderTarget( 0 );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
	Tr2GpuProfiler::GetProfiler().Begin( m_volumeBlit->GetRawRoot(), "Pass #1", renderContext );
	m_volumeBlit->SetParameter( BlueSharedString( "DepthSizes" ), Vector4( float( width ), float( height ), float( originalWidth ), float( originalHeight ) ) );
	Tr2Renderer::DrawScreenQuad( renderContext, m_volumeBlit );
	Tr2GpuProfiler::GetProfiler().End( renderContext );

	renderContext.m_esm.PopDepthStencilBuffer();

	m_volumeHasContent = true;
}

float logBase(float base, float value)
{
	return log2( value ) / log2( base );
}

void Tr2VolumetricsRenderer::UpdateFogSettings( const EveComponentRegistry& registry )
{
	std::vector<ITr2FroxelFogSettings::FroxelFogWeightedSettings> overrides;
	registry.ProcessComponents<ITr2FroxelFogSettings>( [&overrides]( ITr2FroxelFogSettings* component ) -> void {
		overrides.push_back( component->GetFroxelFogSettings() );
	} );
	sort( begin( overrides ), end( overrides ), []( const auto& a, const auto& b ) {
		return a.priority > b.priority;
	} );

	ITr2FroxelFogSettings::FroxelFogWeightedSettings baseline;
	baseline.priority = (PostProcessEnums::Priority)-1;
	baseline.intensity = 1;
	baseline.value.thickness = 0.0f;
	baseline.value.directionality = 0.5f;
	baseline.value.environmentIntensity = 1.0f;
	baseline.value.fogColor = Color( 1.0f, 1.0f, 1.0f, 1.0f );
	baseline.value.backgroundColor = Color( 0.0f, 0.0f, 0.0f, 1.0f );
	overrides.push_back( baseline );

	m_froxelFogSettings = PriorityBlend( overrides );
}

bool Tr2VolumetricsRenderer::HasFog() const
{
	return m_froxelFogSettings.thickness > 0.0f;
}

void Tr2VolumetricsRenderer::RenderFog(
	Tr2RenderContext& renderContext,
	uint32_t width,
	uint32_t height,
	Tr2ShadowMap* cascadedShadowMap,
	const Vector3& sunDirection,
	const Color& sunColor,
	const Matrix& view,
	const Matrix& projection,
	const Matrix& viewLast,
	const Matrix& projectionLast)
{
	RenderFog( m_fogResources, renderContext, width, height, cascadedShadowMap, sunDirection, sunColor, view, projection, viewLast, projectionLast );
}

void Tr2VolumetricsRenderer::RenderFogIntoReflectionMap(
	Tr2RenderContext& renderContext,
	uint32_t width,
	uint32_t height,
	const Vector3& sunDirection,
	const Color& sunColor,
	const Matrix& view,
	const Matrix& projection)
{
	RenderFog( m_fogReflectionResources, renderContext, width, height, nullptr, sunDirection, sunColor, view, projection, view, projection );
}


void Tr2VolumetricsRenderer::UpdateFogEnvironmentMap( Tr2RenderContext& renderContext )
{
	bool froxelsEnabled = m_froxelFogSettings.thickness > 0.0f;
	float environmentIntensity = m_froxelFogSettings.environmentIntensity;
	bool environmentLightingEnabled = froxelsEnabled && environmentIntensity > 0;


	uint32_t environmentMapResolution = 1;
	if( environmentLightingEnabled )
	{
		environmentMapResolution = 128;

		CCP_ASSERT( environmentMapResolution % 8 == 0 );
	}



	auto& mieEnvironmentMap = *m_mieEnvironmentMap->GetTexture();
	if( !mieEnvironmentMap.IsValid() || mieEnvironmentMap.GetWidth() != environmentMapResolution )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		m_environmentBlendCounter = 0; //reset counter so we start back from 0 when we enabled it next time.

		if( environmentLightingEnabled )
		{
			//Full 32-bit precision is needed due to blending to this texture with a low weight.
			Tr2BitmapDimensions dimensions( Tr2RenderContextEnum::TEX_TYPE_CUBE, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT, environmentMapResolution, environmentMapResolution, 1, 1, 6 );

			mieEnvironmentMap.Create( dimensions, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, renderContext );
		}
		else
		{
			//Create dummy texture
			Tr2BitmapDimensions dimensions( Tr2RenderContextEnum::TEX_TYPE_CUBE, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 1, 1, 1, 1, 6 );

			uint8_t black[6] = {};
			Tr2SubresourceData initialData[6];
			for( uint32_t i = 0; i < 6; ++i )
			{
				initialData[i].m_sysMem = black;
				initialData[i].m_sysMemPitch = 4;
				initialData[i].m_sysMemSlicePitch = 4;
			}

			mieEnvironmentMap.Create( dimensions, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, initialData, renderContext );
		}
		if( !mieEnvironmentMap.IsValid() )
		{
			CCP_LOGERR( "Tr2VolumetricsRenderer failed to create environment cubemap with size %ix", int( environmentMapResolution ) );
			return;
		}

		m_mieEnvironmentMap->OnTextureChange().Broadcast();
	}

	if( environmentLightingEnabled )
	{
		const float g = 1.61803398874989484820;
		const float g2 = 1.32471795724474602596;

		m_environmentJitter += Vector2( 1.0f / ( g2 * g2 ), 1.0f / g2 );
		m_environmentJitter.x -= floor( m_environmentJitter.x );
		m_environmentJitter.y -= floor( m_environmentJitter.y );

		m_environmentRandom += g;
		m_environmentRandom = m_environmentRandom - floor( m_environmentRandom );


		float mieG = -std::clamp( m_froxelFogSettings.directionality, 0.001f, 0.999f );

		m_environmentBlendCounter++;
		float delta = abs( m_environmentDirectionality - mieG );
		m_environmentBlendCounter = min( m_environmentBlendCounter, 1.0f / delta );
		float blendWeight = max( 1.0f / m_environmentBlendCounter, 0.005f );
		m_environmentDirectionality = mieG;

		m_updateMieEnvironmentMap->SetParameter( BlueSharedString( "Resolution" ), environmentMapResolution );
		m_updateMieEnvironmentMap->SetParameter( BlueSharedString( "Jitter" ), m_environmentJitter );
		m_updateMieEnvironmentMap->SetParameter( BlueSharedString( "MieG" ), mieG );
		m_updateMieEnvironmentMap->SetParameter( BlueSharedString( "BlendWeight" ), blendWeight );
		m_updateMieEnvironmentMap->SetParameter( BlueSharedString( "Random" ), m_environmentRandom );
		m_updateMieEnvironmentMap->SetParameter( BlueSharedString( "PrecomputedMieEnvironmentMap" ), m_mieEnvironmentMap );
		Tr2Renderer::RunComputeShader( m_updateMieEnvironmentMap, environmentMapResolution / 8, environmentMapResolution / 8, 6, renderContext );
	}
}

void Tr2VolumetricsRenderer::RenderFog(
	FogViewDependentResources& resources,
	Tr2RenderContext& renderContext,
	uint32_t originalWidth,
	uint32_t originalHeight,
	Tr2ShadowMap* cascadedShadowMap,
	const Vector3& sunDirection,
	const Color& sunColor,
	const Matrix& view,
	const Matrix& projection,
	const Matrix& viewLast,
	const Matrix& projectionLast )
{
	uint32_t width = 1;
	uint32_t height = 1;
	uint32_t depth = 1;

	bool froxelsEnabled = m_froxelFogSettings.thickness > 0.0f;
	if( froxelsEnabled )
	{
		float scale = 1 / 8.0f; //clamp this to <=1.0 when it's no longer hardcoded
		width = std::max( 4u, uint32_t( originalWidth * scale ) );
		height = std::max( 4u, uint32_t( originalHeight * scale ) );
		depth = std::max( 1u, 128u );
	}

	auto temporalFog = resources.temporalFroxels0 && resources.temporalFroxels1;
	
	auto& fogFroxels = *resources.fogFroxels->GetTexture();

	if( !fogFroxels.IsValid() || fogFroxels.GetWidth() != width || fogFroxels.GetHeight() != height || fogFroxels.GetDepth() != depth )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		auto temporalFroxels0 = resources.temporalFroxels0 ? resources.temporalFroxels0->GetTexture() : nullptr;
		auto temporalFroxels1 = resources.temporalFroxels0 ? resources.temporalFroxels1->GetTexture() : nullptr;

		if( froxelsEnabled )
		{

			Tr2BitmapDimensions dimensions( Tr2RenderContextEnum::TEX_TYPE_3D, Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT, width, height, depth, 1, 1 );

			fogFroxels.Create( dimensions, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, renderContext );
			if( temporalFog )
			{
				temporalFroxels0->Create( dimensions, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, renderContext );
				temporalFroxels1->Create( dimensions, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, renderContext );
			}

		}
		else
		{
			//Create dummy textures
			Tr2BitmapDimensions dimensions( Tr2RenderContextEnum::TEX_TYPE_3D, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 1, 1, 1, 1, 1 );

			uint8_t black[4] = {};
			Tr2SubresourceData initialData;
			initialData.m_sysMem = black;
			initialData.m_sysMemPitch = 4;
			initialData.m_sysMemSlicePitch = 4;

			fogFroxels.Create( dimensions, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, &initialData, renderContext );
			if( temporalFog )
			{
				temporalFroxels0->Create( dimensions, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, &initialData, renderContext );
				temporalFroxels1->Create( dimensions, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, &initialData, renderContext );
			}
		}

		if( !fogFroxels.IsValid() || ( temporalFog && ( !temporalFroxels0->IsValid() || !temporalFroxels1->IsValid() ) ) )
		{
			CCP_LOGERR( "Tr2VolumetricsRenderer failed to create froxel textures with size %ix%ix%i", int( width ), int( height ), int( depth ) );
			return;
		}

		resources.fogFroxels->OnTextureChange().Broadcast();
		if( temporalFog )
		{
			resources.temporalFroxels0->OnTextureChange().Broadcast();
			resources.temporalFroxels1->OnTextureChange().Broadcast();
		}
	}

	int workgroupSize = 4;
	int wgX = ( width + workgroupSize - 1 ) / workgroupSize;
	int wgY = ( height + workgroupSize - 1 ) / workgroupSize;
	int wgZ = ( depth + workgroupSize - 1 ) / workgroupSize;


	if (!froxelsEnabled)
	{
		//We have nothing more we need to do.
		return;
	}

	const float g = 1.61803398874989484820;
	const float g2 = 1.32471795724474602596;

	float maxDistance = m_gameBackClip;
	float maxDistanceVisibility = exp(-m_froxelFogSettings.thickness);
	float baseDensity = m_froxelFogSettings.thickness / maxDistance;

	Color backgroundColor = m_froxelFogSettings.backgroundColor;

	

	//MieG is negative when light is scattered in the direction the light was already going.
	//Expose this value as positive, then clamp it so that it's always negative.
	//Exactly 0.0 and 1.0 both cause issues with the math, so clamp to a slightly larger value.
	float mieG = -std::clamp( m_froxelFogSettings.directionality, 0.001f, 0.999f );




	Matrix inverseView = Inverse( view );
	Matrix inverseProjection = Inverse( projection );

	if( temporalFog )
	{
		resources.froxelJitter += Vector3( 1.0f / ( g2 * g2 ), 1.0f / g2, 1.0f / g );
		resources.froxelJitter.x -= floor( resources.froxelJitter.x );
		resources.froxelJitter.y -= floor( resources.froxelJitter.y );
		resources.froxelJitter.z -= floor( resources.froxelJitter.z );
	}

	{
		bool hasShadows;
		{
			if( !m_fogConstantBuffer.IsValid() )
			{
				m_fogConstantBuffer.Create( uint32_t( sizeof( FogPerObjectData ) ), renderContext.GetPrimaryRenderContext() );
			}

			FogPerObjectData* data;

			m_fogConstantBuffer.Lock( (void**)&data, renderContext );

			data->ResolutionX = width;
			data->ResolutionY = height;
			data->ResolutionZ = depth;

			data->ProjectionMatrix = Transpose( projection );
			data->InverseProjectionMatrix = Transpose( inverseProjection );

			data->Jitter = resources.froxelJitter;
			data->Far = maxDistance;

			Color fogColor = m_froxelFogSettings.fogColor;
			data->Scattering = Vector3( fogColor.r, fogColor.g, fogColor.b );
			data->BaseDensity = baseDensity;

			data->MaxDistanceVisibility = maxDistanceVisibility;
			data->MieG = mieG;
			data->EnvironmentIntensity = m_froxelFogSettings.environmentIntensity;


			//data->Extinction = extinction;

			data->InverseViewMatrix = Transpose( inverseView );



			{
				Vector4 topLeft = Vector4( -1, +1, 0, 1 ) * inverseProjection; // flipped Y
				Vector4 botRight = Vector4( +1, -1, 0, 1 ) * inverseProjection;

				//No need for W-divide here, because it cancels out when we divide by z: (xy/w) / (-z/w) = xy/-z

				float invZ = 1.0f / -topLeft.z; //both have the same Z, since they have the same depth

				float addX = topLeft.x * invZ;
				float addY = topLeft.y * invZ;
				float mulX = botRight.x * invZ - addX;
				float mulY = botRight.y * invZ - addY;

				data->UnprojectParams = Vector4( mulX, mulY, addX, addY );
			}

			{
				Vector4 offset = Vector4( +1, -1, -1, 1 ) * projectionLast; //flipped Y

				//because we set view space Z to -1, W will be 1.0, so we don't need to do a W divide here

				float mulX = offset.x * 0.5f;
				float mulY = offset.y * 0.5f;
				float addX = 0.5f;
				float addY = 0.5f;

				data->PreviousProjectParams = Vector4( mulX, mulY, addX, addY );
			}

			Matrix reprojectionMatrix = inverseView * viewLast;
			data->ReprojectionMatrix = Transpose( reprojectionMatrix );

			Vector4 tempSunDir = Vector4( -sunDirection, 0.0f ) * view;
			data->SunDirection = Vector3(tempSunDir.x, tempSunDir.y, tempSunDir.z);

			data->SunColor = Vector3( sunColor.r, sunColor.g, sunColor.b );

			if( cascadedShadowMap )
			{
				data->ShadowMapValues[0] = cascadedShadowMap->m_perSplitData.ShadowMapValues[0];
				data->ShadowMapValues[1] = cascadedShadowMap->m_perSplitData.ShadowMapValues[1];
				data->ShadowMapValues[2] = cascadedShadowMap->m_perSplitData.ShadowMapValues[2];
				data->ShadowMapValues[3] = cascadedShadowMap->m_perSplitData.ShadowMapValues[3];

				for( int i = 0; i < SHADOW_FRUSTUM_COUNT; ++i )
				{
					Matrix matrix = inverseView * Transpose( cascadedShadowMap->m_perSplitData.ShadowMatrixVal[i] );

					matrix *= ScalingMatrix( 0.5f, -0.5f, 1 ) * TranslationMatrix( 0.5f, 0.5f, 0 ); //Flip y and change range from (-1, +1) to (0, 1)

					int cells_x = 8;
					int cells_y = 2;
					int x = i % cells_x;
					int y = i / cells_x;
					matrix *= ScalingMatrix( 1.0f / cells_x, 1.0f / cells_y, 1 ) * TranslationMatrix( (float)x / cells_x, (float)y / cells_y, 0 );

					data->ShadowMatrix[i] = Transpose( matrix );
				}
				data->SplitInfo = cascadedShadowMap->m_perSplitData.SplitInfo;

				hasShadows = true;
			}
			else
			{
				hasShadows = false;
			}

			m_fogConstantBuffer.Unlock( renderContext );

			renderContext.SetConstants( m_fogConstantBuffer, Tr2RenderContextEnum::COMPUTE_SHADER, Tr2Renderer::GetPerObjectVSStartRegister() );
		}

		int workgroupSize = 4;
		int wgX = ( width + workgroupSize - 1 ) / workgroupSize;
		int wgY = ( height + workgroupSize - 1 ) / workgroupSize;
		int wgZ = ( depth + workgroupSize - 1 ) / workgroupSize;

		
		resources.calculateFroxels->SetOption( BlueSharedString( "SHADOWS" ), BlueSharedString( hasShadows ? "SHADOWS_ENABLED" : "SHADOWS_DISABLED" ) );
		resources.calculateFroxels->SetParameter( BlueSharedString( "OutputTexture" ), resources.fogFroxels );
		Tr2Renderer::RunComputeShader( resources.calculateFroxels, wgX, wgY, wgZ, renderContext );

		if( temporalFog )
		{
			Tr2TextureReferencePtr& temporalInput = resources.currentTemporalFroxels ? resources.temporalFroxels0 : resources.temporalFroxels1;
			Tr2TextureReferencePtr& temporalOutput = resources.currentTemporalFroxels ? resources.temporalFroxels1 : resources.temporalFroxels0;
			resources.currentTemporalFroxels = !resources.currentTemporalFroxels;

			resources.filterFroxels->SetParameter( BlueSharedString( "InputTexture" ), resources.fogFroxels );
			resources.filterFroxels->SetParameter( BlueSharedString( "PreviousTexture" ), temporalInput );
			resources.filterFroxels->SetParameter( BlueSharedString( "OutputTexture" ), temporalOutput );
			Tr2Renderer::RunComputeShader( resources.filterFroxels, wgX, wgY, wgZ, renderContext );

			resources.raymarchFroxels->SetParameter( BlueSharedString( "InputTexture" ), temporalOutput );
		}
		
		resources.raymarchFroxels->SetParameter( BlueSharedString( "OutputTexture" ), resources.fogFroxels );
		Tr2Renderer::RunComputeShader( resources.raymarchFroxels, ( width + 7 ) / 8, ( height + 7 ) / 8, 1, renderContext ); // 8x8 workgroup
	}


	{
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );

		resources.applyFroxels->SetOption( BlueSharedString( "ENVIRONMENT_LIGHTING" ), BlueSharedString( m_froxelFogSettings.environmentIntensity > 0 ? "ENVIRONMENT_LIGHTING_ENABLED" : "ENVIRONMENT_LIGHTING_DISABLED" ) );
		resources.applyFroxels->SetParameter( BlueSharedString( "FroxelTexture" ), resources.fogFroxels );
		resources.applyFroxels->SetParameter( BlueSharedString( "MieEnvironmentMap" ), m_mieEnvironmentMap );
		resources.applyFroxels->SetParameter( BlueSharedString( "OriginalResolution" ), Vector2( float( originalWidth ), float( originalHeight ) ) );
		resources.applyFroxels->SetParameter( BlueSharedString( "MaxDistance" ), maxDistance );
		resources.applyFroxels->SetParameter( BlueSharedString( "MaxDistanceVisibility" ), maxDistanceVisibility );
		resources.applyFroxels->SetParameter( BlueSharedString( "BaseDensity" ), baseDensity );
		resources.applyFroxels->SetParameter( BlueSharedString( "BackgroundColor" ), Vector3( backgroundColor.r, backgroundColor.g, backgroundColor.b ) );
		resources.applyFroxels->SetParameter( BlueSharedString( "MieG" ), mieG );
		resources.applyFroxels->SetParameter( BlueSharedString( "EnvironmentIntensity" ), m_froxelFogSettings.environmentIntensity );
		Tr2Renderer::DrawScreenQuad( renderContext, resources.applyFroxels );
	}


}


void Tr2VolumetricsRenderer::RenderShadows(
	const EveComponentRegistry& registry,
	ITr2TextureProvider* shadowMap,
	Tr2RenderContext& renderContext )
{
	auto volumetricsCount = registry.ComponentCount<ITr2VolumetricRenderable>();
	if( !shadowMap || !shadowMap->GetTexture() || !m_castShadows )
	{
		return;
	}
	auto accumulator = m_batches.get();

	registry.ProcessComponents<ITr2VolumetricRenderable>( [&accumulator] ( ITr2VolumetricRenderable* volumetric ) { 
		volumetric->GetVolumetricShadowBatches( accumulator ); } );

	if( m_batches->GetBatchCount() )
	{
		GPU_REGION( renderContext, "Volumetric Shadows" );

		m_batches->Finalize();

		renderContext.m_esm.PushRenderTarget( *shadowMap->GetTexture() );
		renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );

		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );
		renderContext.RenderBatches( m_batches.get(), BlueSharedString( "Shadow" ) );
		m_batches->Clear();

		renderContext.m_esm.PopRenderTarget();
		renderContext.m_esm.PopDepthStencilBuffer();
	}
}

void Tr2VolumetricsRenderer::UpdateVariableStore()
{
	GlobalStore().RegisterVariable( "EveSceneFogVolumeMap", m_volumeSlices );
}
