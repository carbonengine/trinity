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
#include "Tr2LightManager.h"
#include "Tr2TextureArray.h"
#include "include/TriMath.h"
#include "Utilities/Vector4d.h"


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

	rtCalculateFroxels.CreateInstance();
	rtCalculateFroxels->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/Fog/RtCalculateFroxels.fx" );

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
	m_fogReflectionResources( false ),
	m_logBlending( true ),
	m_logBlendingSmoothness( 4.0 )
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

	m_gameBackClip = 1E6f; //must match what the actual game uses; not what Graphite is currently set to, as the user can change the back clip.

	
	int noiseResolution = 64;
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		int numTexels = noiseResolution * noiseResolution * noiseResolution;
		Tr2BitmapDimensions dimensions( Tr2RenderContextEnum::TEX_TYPE_3D, Tr2RenderContextEnum::PIXEL_FORMAT_R8_SNORM, noiseResolution, noiseResolution, noiseResolution, 1, 1 );

		int8_t* noiseData = new int8_t[numTexels];
		for( int i = 0; i < numTexels; i++ )
		{
			noiseData[i] = (int8_t)TriRandInt( -127, +128 );
		}

		Tr2SubresourceData initialData;
		initialData.m_sysMem = noiseData;
		initialData.m_sysMemPitch = noiseResolution;
		initialData.m_sysMemSlicePitch = noiseResolution * noiseResolution;

		m_froxel3DNoise.CreateInstance();
		m_froxel3DNoise->GetTexture()->Create( dimensions, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, &initialData, renderContext );

		delete[] noiseData;
	}

	{
		m_testValue = 0.0f;
		Matrix4dFromMatrix( m_godRayNoiseMatrix, IdentityMatrix() );
	}


	{
		m_mieEnvironmentMap.CreateInstance();
		GlobalStore().RegisterVariable( "EveSceneMieEnvironmentMap", m_mieEnvironmentMap );
		m_environmentJitter = Vector2( 0, 0 );
		m_environmentRandom = 0.0f;
		m_previousEnvironmentG = 0.0f;
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
	bool raytracingEnabled,
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
	sceneInfo.raytracedShadows = raytracingEnabled;

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

void Tr2VolumetricsRenderer::UpdateFogSettings( const EveComponentRegistry& registry, const EveUpdateContext& updateContext )
{
	std::vector<ITr2FroxelFogSettings::FroxelFogSettings*> overrides;
	double logBlendingSmoothness = m_logBlendingSmoothness;

	registry.ProcessComponents<ITr2FroxelFogSettings>( [&overrides, logBlendingSmoothness]( ITr2FroxelFogSettings* component ) -> void {

		ITr2FroxelFogSettings::FroxelFogSettings* fogSettings = component->GetFroxelFogSettings();

		//Compute log(1 + thickness * m_logBlendingSmoothness) for each fog
		fogSettings->logThickness.value = log1p( (double)fogSettings->thickness.value * logBlendingSmoothness );
		fogSettings->logThickness.enabled = fogSettings->thickness.enabled;

		overrides.push_back( fogSettings );
	} );
	sort( begin( overrides ), end( overrides ), []( const auto& a, const auto& b ) {
		return a->priority > b->priority;
	} );

	//m_froxelFogSettings = OldPriorityBlend( overrides );

	//std::vector<ITr2FroxelFogSettings::FroxelFogSettings*> test;

#define FROXEL_ACCUMULATE( setting ) m_froxelFogSettings.setting = PriorityBlend::Accumulate( &ITr2FroxelFogSettings::FroxelFogSettings::setting, overrides )

	FROXEL_ACCUMULATE( thickness );

	FROXEL_ACCUMULATE( lightDirectionality );

	FROXEL_ACCUMULATE( environmentIntensity );
	FROXEL_ACCUMULATE( environmentDirectionality );

	FROXEL_ACCUMULATE( fogColor );
	FROXEL_ACCUMULATE( backgroundVisibility );

	FROXEL_ACCUMULATE( godRayNoiseIntensity );
	FROXEL_ACCUMULATE( godRayNoiseFrequency );
	FROXEL_ACCUMULATE( godRayNoiseAnimationSpeed );

	FROXEL_ACCUMULATE( fogNoiseIntensity );
	FROXEL_ACCUMULATE( fogNoiseFrequency );
	FROXEL_ACCUMULATE( fogNoiseMovementSpeed );

	if (m_logBlending)
	{
		//Replace the thickness with a logarithmically interpolated one, making the transition from 0 thickness more graceful.
		FROXEL_ACCUMULATE( logThickness );
		m_froxelFogSettings.thickness = (float)( expm1( m_froxelFogSettings.logThickness.value ) / logBlendingSmoothness );
	}

	float delta = updateContext.GetDeltaT();

	m_godRayNoiseAnimation += m_froxelFogSettings.godRayNoiseAnimationSpeed.value * (delta / m_froxel3DNoise->GetDepth());
	m_godRayNoiseAnimation -= floor( m_godRayNoiseAnimation );

	float fogFrequency = exp2f( -m_froxelFogSettings.fogNoiseFrequency.value );
	m_fogNoiseMovement += m_froxelFogSettings.fogNoiseMovementSpeed.value * delta;


}

bool Tr2VolumetricsRenderer::HasFog() const
{
	return m_froxelFogSettings.thickness.value > 0.0f;
}


void Tr2VolumetricsRenderer::UpdateFogEnvironmentMap( Tr2RenderContext& renderContext )
{
	bool froxelsEnabled = m_froxelFogSettings.thickness.value > 0.0f;
	bool environmentLightingEnabled = froxelsEnabled && m_froxelFogSettings.environmentIntensity.value > 0;


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
		GPU_REGION( renderContext, "Update Fog Environment Map" );

		const float g = 1.61803398874989484820;
		const float g2 = 1.32471795724474602596;

		m_environmentJitter += Vector2( 1.0f / ( g2 * g2 ), 1.0f / g2 );
		m_environmentJitter.x -= floor( m_environmentJitter.x );
		m_environmentJitter.y -= floor( m_environmentJitter.y );

		m_environmentRandom += g;
		m_environmentRandom = m_environmentRandom - floor( m_environmentRandom );

		
		float lightG = -std::clamp( m_froxelFogSettings.lightDirectionality.value, 0.001f, 0.999f );
		float environmentG = -std::clamp( m_froxelFogSettings.environmentDirectionality.value, 0.001f, 0.999f );

		m_environmentBlendCounter++;
		float delta = abs( m_previousEnvironmentG - environmentG );
		m_environmentBlendCounter = min( m_environmentBlendCounter, 1.0f / delta );
		float blendWeight = max( 1.0f / m_environmentBlendCounter, 0.005f );
		m_previousEnvironmentG = environmentG;

		m_updateMieEnvironmentMap->SetParameter( BlueSharedString( "Resolution" ), environmentMapResolution );
		m_updateMieEnvironmentMap->SetParameter( BlueSharedString( "Jitter" ), m_environmentJitter );
		m_updateMieEnvironmentMap->SetParameter( BlueSharedString( "EnvironmentG" ), environmentG );
		m_updateMieEnvironmentMap->SetParameter( BlueSharedString( "BlendWeight" ), blendWeight );
		m_updateMieEnvironmentMap->SetParameter( BlueSharedString( "Random" ), m_environmentRandom );
		m_updateMieEnvironmentMap->SetParameter( BlueSharedString( "PrecomputedMieEnvironmentMap" ), m_mieEnvironmentMap );
		Tr2Renderer::RunComputeShader( m_updateMieEnvironmentMap, environmentMapResolution / 8, environmentMapResolution / 8, 6, renderContext );
	}
}


void Tr2VolumetricsRenderer::RenderFog(
	Tr2RenderContext& renderContext,
	uint32_t width,
	uint32_t height,
	Tr2ShadowMap* cascadedShadowMap,
	Tr2RaytracingGeometryPtr raytracingGeometry,
	ShadowQuality shadowQuality,
	const Vector3& sunDirection,
	const Color& sunColor,
	const Vector3d origin,
	const Vector3d originShift,
	const Matrix& view,
	const Matrix& projection,
	const Matrix& viewLast,
	const Matrix& projectionLast )
{
	RenderFog( m_fogResources, renderContext, width, height, cascadedShadowMap, raytracingGeometry, shadowQuality, sunDirection, sunColor, origin, originShift, view, projection, viewLast, projectionLast );
}

void Tr2VolumetricsRenderer::RenderFogIntoReflectionMap(
	Tr2RenderContext& renderContext,
	uint32_t width,
	uint32_t height,
	const Vector3& sunDirection,
	const Color& sunColor,
	const Vector3d origin,
	const Matrix& view,
	const Matrix& projection )
{
	RenderFog( m_fogReflectionResources, renderContext, width, height, nullptr, nullptr, ShadowQuality::SHADOW_DISABLED, sunDirection, sunColor, origin, Vector3d( 0, 0, 0 ), view, projection, view, projection );
}

void Tr2VolumetricsRenderer::RenderFog(
	FogViewDependentResources& resources,
	Tr2RenderContext& renderContext,
	uint32_t originalWidth,
	uint32_t originalHeight,
	Tr2ShadowMap* cascadedShadowMap,
	Tr2RaytracingGeometryPtr raytracingGeometry,
	ShadowQuality shadowQuality, 
	const Vector3& sunDirection,
	const Color& sunColor,
	const Vector3d origin,
	const Vector3d originShift,
	const Matrix& view,
	const Matrix& projection,
	const Matrix& viewLast,
	const Matrix& projectionLast )
{

	// Figure out the resolution we need.
	uint32_t width = 1;
	uint32_t height = 1;
	uint32_t depth = 1;

	bool froxelsEnabled = m_froxelFogSettings.thickness.value > 0.0f;
	if( froxelsEnabled )
	{

		float scale;
		uint32_t numLayers;

		switch (m_quality)
		{
		case Tr2VolumerticQuality::Ultra:
			scale = 1 / 6.0;
			numLayers = 128;
			break;

		case Tr2VolumerticQuality::High:
			scale = 1 / 8.0;
			numLayers = 128;
			break;

		case Tr2VolumerticQuality::Medium:
			scale = 1 / 12.0;
			numLayers = 96;
			break;

		case Tr2VolumerticQuality::Low:
		default:
			scale = 1 / 16.0;
			numLayers = 64;
			break;
		}

		width = std::max( 4u, uint32_t( originalWidth * scale ) );
		height = std::max( 4u, uint32_t( originalHeight * scale ) );
		depth = std::max( 1u, numLayers );
	}

	//Update textures to the new resolution, if needed.
	UpdateTextures(resources, width, height, depth, froxelsEnabled );


	auto temporalFog = resources.temporalFroxels0 && resources.temporalFroxels1;
	GlobalStore().RegisterVariable( "EveSceneFroxelFogMap", resources.fogFroxels );


	if (!froxelsEnabled)
	{
		//We have nothing more we need to do.
		return;
	}

	
	GPU_REGION( renderContext, "Froxel Fog" );


	//Determine the shadow type
	enum SHADOW_TYPE
	{
		SHADOWS_DISABLED,
		SHADOWS_CASCADED,
		SHADOWS_RAYTRACED
	};

	SHADOW_TYPE shadowType;
	if( raytracingGeometry && raytracingGeometry->HasGeometry() && shadowQuality == ShadowQuality::SHADOW_RAYTRACED && resources.rtCalculateFroxels && resources.rtCalculateFroxels->GetShaderStateInterface() )
	{
		shadowType = SHADOWS_RAYTRACED;
	}
	else if( cascadedShadowMap && ( shadowQuality == ShadowQuality::SHADOW_LOW || shadowQuality == ShadowQuality::SHADOW_HIGH ) )
	{
		shadowType = SHADOWS_CASCADED;
	}
	else
	{
		shadowType = SHADOWS_DISABLED;
	}


	if( temporalFog )
	{
		//Update the per-frame jitter offset
		const float g = 1.61803398874989484820;
		const float g2 = 1.32471795724474602596;

		resources.froxelJitter += Vector3( 1.0f / ( g2 * g2 ), 1.0f / g2, 1.0f / g );
		resources.froxelJitter.x -= floor( resources.froxelJitter.x );
		resources.froxelJitter.y -= floor( resources.froxelJitter.y );
		resources.froxelJitter.z -= floor( resources.froxelJitter.z );
	}

	
	if( !m_fogConstantBuffer.IsValid() )
	{
		m_fogConstantBuffer.Create( uint32_t( sizeof( FogPerObjectData ) ), renderContext.GetPrimaryRenderContext() );
	}

	FogPerObjectData* data;
	m_fogConstantBuffer.Lock( (void**)&data, renderContext );

	UpdatePerObjectData( data, view, projection, viewLast, projectionLast, origin, originShift, sunDirection, sunColor, width, height, depth, resources.froxelJitter, shadowType == SHADOWS_CASCADED ? cascadedShadowMap : nullptr );

	m_fogConstantBuffer.Unlock( renderContext );

	renderContext.SetConstants( m_fogConstantBuffer, Tr2RenderContextEnum::COMPUTE_SHADER, Tr2Renderer::GetPerObjectVSStartRegister() );

	{

		int workgroupSize = 4;
		int wgX = ( width + workgroupSize - 1 ) / workgroupSize;
		int wgY = ( height + workgroupSize - 1 ) / workgroupSize;
		int wgZ = ( depth + workgroupSize - 1 ) / workgroupSize;

		{
			GPU_REGION( renderContext, "Calculate froxels" );

			if( shadowType == SHADOWS_RAYTRACED )
			{
				uint32_t techniqueIndex;
				if( !resources.rtCalculateFroxels->GetShaderStateInterface()->GetTechniqueIndex( BlueSharedString( "RtShadow" ), techniqueIndex ) )
				{
					CCP_LOGERR( "Failed to get technique index for raytraced froxel fog shadows!" );
					return;
				}

				std::wstring rayGenName;
				std::wstring missName;
				m_pipelineManager.AddLibrary( rayGenName, missName, resources.rtCalculateFroxels, BlueSharedString( "RtShadow" ) );

				Tr2RtPipelineStateAL pipelineState = m_pipelineManager.GetPipelineState( renderContext );

				if( !pipelineState.IsValid() )
				{
					CCP_LOGERR( "Failed to get pipeline state for raytraced froxel fog shadows!" );
					return;
				}

				Tr2RtLocalMaterialDescriptionAL material;
				material.SetConstants( Tr2Renderer::GetPerObjectVSStartRegister(), m_fogConstantBuffer );

				CCP_STATS_ZONE( "Create shader table" );
				m_shaderTableDesc.AddRayGenShader( rayGenName.c_str(), material );
				m_shaderTableDesc.AddMissShader( missName.c_str(), material );

				Tr2RtShaderTableAL shadowShaderTable;
				shadowShaderTable.Create( m_shaderTableDesc, pipelineState, renderContext.GetPrimaryRenderContext() );

				resources.rtCalculateFroxels->SetOption( BlueSharedString( "GOD_RAY_NOISE" ), BlueSharedString( m_froxelFogSettings.godRayNoiseIntensity.value > 0.0f ? "GOD_RAY_NOISE_ENABLED" : "GOD_RAY_NOISE_DISABLED" ) );
				resources.rtCalculateFroxels->SetOption( BlueSharedString( "FOG_NOISE" ), BlueSharedString( m_froxelFogSettings.fogNoiseIntensity.value > 0.0f ? "FOG_NOISE_ENABLED" : "FOG_NOISE_DISABLED" ) );

				resources.rtCalculateFroxels->SetParameter( BlueSharedString( "Noise3DTexture" ), m_froxel3DNoise );
				resources.rtCalculateFroxels->SetParameter( BlueSharedString( "RtFroxelOutputTexture" ), resources.fogFroxels );

				resources.rtCalculateFroxels->SetParameter( BlueSharedString( "RtShadowScene" ), raytracingGeometry );
				resources.rtCalculateFroxels->ApplyMaterialDataForRtState( techniqueIndex, pipelineState, renderContext );
				renderContext.UseAccelerationStructure( raytracingGeometry->GetTLAS() );

				{
					CCP_STATS_ZONE( "renderContext.UseResources" );
					renderContext.UseResources( Tr2UseResourceDestination::COMPUTE, Tr2GpuUsage::SHADER_RESOURCE, raytracingGeometry->GetBindlessResources() );
				}

				renderContext.DispatchRays( pipelineState, shadowShaderTable, rayGenName.c_str(), width, height, depth );
			}
			else
			{
				//Otherwise, just use a plain compute shader.
				resources.calculateFroxels->SetOption( BlueSharedString( "SHADOWS" ), BlueSharedString( shadowType == SHADOWS_CASCADED  ? "SHADOWS_ENABLED" : "SHADOWS_DISABLED" ) );
				resources.calculateFroxels->SetOption( BlueSharedString( "GOD_RAY_NOISE" ), BlueSharedString( m_froxelFogSettings.godRayNoiseIntensity.value > 0.0f ? "GOD_RAY_NOISE_ENABLED" : "GOD_RAY_NOISE_DISABLED" ) );
				resources.calculateFroxels->SetOption( BlueSharedString( "FOG_NOISE" ), BlueSharedString( m_froxelFogSettings.fogNoiseIntensity.value > 0.0f ? "FOG_NOISE_ENABLED" : "FOG_NOISE_DISABLED" ) );

				resources.calculateFroxels->SetParameter( BlueSharedString( "Noise3DTexture" ), m_froxel3DNoise );
				resources.calculateFroxels->SetParameter( BlueSharedString( "RtFroxelOutputTexture" ), resources.fogFroxels );

				Tr2Renderer::RunComputeShader( resources.calculateFroxels, wgX, wgY, wgZ, renderContext );
			}
		}

		if( temporalFog )
		{
			GPU_REGION( renderContext, "Temporal filter" );
			Tr2TextureReferencePtr& temporalInput = resources.currentTemporalFroxels ? resources.temporalFroxels0 : resources.temporalFroxels1;
			Tr2TextureReferencePtr& temporalOutput = resources.currentTemporalFroxels ? resources.temporalFroxels1 : resources.temporalFroxels0;
			resources.currentTemporalFroxels = !resources.currentTemporalFroxels;

			resources.filterFroxels->SetParameter( BlueSharedString( "InputTexture" ), resources.fogFroxels );
			resources.filterFroxels->SetParameter( BlueSharedString( "PreviousTexture" ), temporalInput );
			resources.filterFroxels->SetParameter( BlueSharedString( "OutputTexture" ), temporalOutput );
			Tr2Renderer::RunComputeShader( resources.filterFroxels, wgX, wgY, wgZ, renderContext );

			resources.raymarchFroxels->SetParameter( BlueSharedString( "InputTexture" ), temporalOutput );
		}
		
		{
			GPU_REGION( renderContext, "Raymarch" );
			resources.raymarchFroxels->SetParameter( BlueSharedString( "OutputTexture" ), resources.fogFroxels );
			Tr2Renderer::RunComputeShader( resources.raymarchFroxels, ( width + 7 ) / 8, ( height + 3 ) / 4, 1, renderContext ); // 8x4 workgroup is faster here
		}
	}


	{
		GPU_REGION( renderContext, "Apply to scene" );
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA );

		resources.applyFroxels->SetOption( BlueSharedString( "ENVIRONMENT_LIGHTING" ), BlueSharedString( m_froxelFogSettings.environmentIntensity.value > 0 ? "ENVIRONMENT_LIGHTING_ENABLED" : "ENVIRONMENT_LIGHTING_DISABLED" ) );
		Tr2Renderer::DrawScreenQuad( renderContext, resources.applyFroxels );
	}
}

void Tr2VolumetricsRenderer::UpdateTextures( FogViewDependentResources& resources, uint32_t width, uint32_t height, uint32_t depth, bool enabled )
{
	auto temporalFog = resources.temporalFroxels0 && resources.temporalFroxels1;

	auto& fogFroxels = *resources.fogFroxels->GetTexture();
	if( !fogFroxels.IsValid() || fogFroxels.GetWidth() != width || fogFroxels.GetHeight() != height || fogFroxels.GetDepth() != depth )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		auto temporalFroxels0 = resources.temporalFroxels0 ? resources.temporalFroxels0->GetTexture() : nullptr;
		auto temporalFroxels1 = resources.temporalFroxels0 ? resources.temporalFroxels1->GetTexture() : nullptr;

		if( enabled )
		{

			//Tr2BitmapDimensions dimensions( Tr2RenderContextEnum::TEX_TYPE_3D, Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT, width, height, depth, 1, 1 );
			Tr2BitmapDimensions dimensions( Tr2RenderContextEnum::TEX_TYPE_3D, Tr2RenderContextEnum::PIXEL_FORMAT_R11G11B10_FLOAT, width, height, depth, 1, 1 );

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
}

void Tr2VolumetricsRenderer::UpdatePerObjectData( FogPerObjectData* data, const Matrix& view, const Matrix& projection, const Matrix& viewLast, const Matrix& projectionLast, const Vector3d& origin, const Vector3d& originShift, const Vector3& sunDirection, const Color& sunColor, uint32_t width, uint32_t height, uint32_t depth, const Vector3& jitter, const Tr2ShadowMap* cascadedShadowMap )
{
	float maxDistance = m_gameBackClip;
	float maxDistanceVisibility = exp( -m_froxelFogSettings.thickness.value );
	float baseDensity = m_froxelFogSettings.thickness.value / maxDistance;

	float environmentIntensity = m_froxelFogSettings.environmentIntensity.value;

	Color fogColor = m_froxelFogSettings.fogColor.value;
	float backgroundVisibility = std::clamp( m_froxelFogSettings.backgroundVisibility.value, 0.0f, 1.0f );


	//G is negative when light is scattered in the direction the light was already going in.
	//Expose this value as positive, then negate and clamp it so that it's always negative.
	//Exactly 0.0 and 1.0 both cause issues with the math in the shader, so clamp to a slightly smaller range.
	float lightG = -std::clamp( m_froxelFogSettings.lightDirectionality.value, 0.001f, 0.999f );
	float environmentG = -std::clamp( m_froxelFogSettings.environmentDirectionality.value, 0.001f, 0.999f );

	Matrix inverseView = Inverse( view );
	Matrix inverseProjection = Inverse( projection );

	{

		data->ResolutionX = width;
		data->ResolutionY = height;
		data->ResolutionZ = depth;

		data->Jitter = jitter;
		data->Far = maxDistance;

		data->Scattering = Vector3( fogColor.r, fogColor.g, fogColor.b );
		data->BaseDensity = baseDensity;

		data->MaxDistanceVisibility = maxDistanceVisibility;
		data->LightG = lightG;
		data->EnvironmentIntensity = environmentIntensity;


		//data->Extinction = extinction;


		{
			float exactFrequency = exp2f( -m_froxelFogSettings.fogNoiseFrequency.value );
			float baseFrequency = exp2f( floor( -m_froxelFogSettings.fogNoiseFrequency.value ) );
			float nextFrequency = baseFrequency * 2.0f;


			double offsetX = origin.x * baseFrequency;
			double offsetY = origin.y * baseFrequency;
			double offsetZ = origin.z * baseFrequency;
			offsetX -= floor( offsetX );
			offsetY -= floor( offsetY );
			offsetZ -= floor( offsetZ );

			data->FogNoiseOffset = Vector3( (float)offsetX, (float)offsetY, (float)offsetZ );


			data->FogNoiseFrequency = baseFrequency;
			data->FogNoiseLerp = ( exactFrequency - baseFrequency ) / ( nextFrequency - baseFrequency );
			data->FogNoiseIntensity = m_froxelFogSettings.fogNoiseIntensity.value;
		}


		{

			/*
					This code is used to re-align m_godRayNoiseMatrix so that it is always aligned with the sun's direction.
					This gets complicated as we're trying to align a 2D texture along the sun's direction without causing popping/sudden changes.
					It does this by finding how many radians it is off by, constructing a rotation axis and then rotating the matrix by said angle to re-align it.

					This is the minimal rotation needed to re-align the matrix, but has some weird side effects:
					- Rotating like this means that if the direction changes a lot and then returns to a previous position, the orientation of the noise texture can be different.
					- When we rotate the matrix, the camera's position in "noise space" changes dramatically. We therefore cancel out this "motion" so that at least the camera ends up in the same position again.
				*/

			//Get the current direction of the god ray noise matrix
			Vector3 noiseDirection = Normalize( Vector3( (float)m_godRayNoiseMatrix[2], (float)m_godRayNoiseMatrix[6], (float)m_godRayNoiseMatrix[10] ) );

			//Check if the angle between the new sun direction is more than a small threshold
			float angle = acos( std::clamp( Dot( sunDirection, noiseDirection ), -1.0f, 1.0f ) ); //Clamp to avoid going out of the acos() domain of [-1, +1] due to rounding.
			if( angle > 0.001f )
			{
				//Construct the rotation axis
				Vector3 axis = Normalize( Cross( sunDirection, noiseDirection ) );

				//Create a rotation matrix and apply it to the god ray noise matrix.
				double rotationMatrix[16];
				Matrix4dFromMatrix( rotationMatrix, RotationMatrix( axis, angle ) );
				double newMatrix[16];
				Matrix4dMultiply( newMatrix, rotationMatrix, m_godRayNoiseMatrix );

				//Figure out where the camera used to be in noise space and where it is now.
				Vector3d cameraPosition = origin + TransformCoord( Vector3( 0, 0, 0 ), inverseView );
				Vector4d previousNoiseCoords = Matrix4dTransform( Vector4d( cameraPosition, 1.0 ), m_godRayNoiseMatrix );
				Vector4d noiseCoords = Matrix4dTransform( Vector4d( cameraPosition, 1.0 ), newMatrix );

				//Calculate the difference and update the new matrix to cancel out the motion resulting from the rotation.
				Vector4d difference = previousNoiseCoords - noiseCoords;
				newMatrix[12] += difference.x;
				newMatrix[13] += difference.y;
				newMatrix[14] += difference.z;

				//Finally, copy the new matrix back to m_godRayNoiseMatrix
				Matrix4dCopy( m_godRayNoiseMatrix, newMatrix );
			}

			//Since the shader operates relative to the origin, we need to add the origin in noise space to the translation here.
			//Also apply fmod() to it so that we don't get huge values that break the floating point precision when casting to float.

			//Build a translation matrix
			double originTranslation[16];
			Matrix4dFromMatrix( originTranslation, IdentityMatrix() );
			originTranslation[12] = origin.x;
			originTranslation[13] = origin.y;
			originTranslation[14] = origin.z;

			//Multiply it with the god ray noise matrix.
			double relativeNoiseMatrix[16];
			Matrix4dMultiply( relativeNoiseMatrix, originTranslation, m_godRayNoiseMatrix );

			//Apply fmod() to the translation based on the noise frequency to avoid precision issues when converting to a 32-bit float matrix later.
			float baseFrequency = exp2f( floor( -m_froxelFogSettings.godRayNoiseFrequency.value ) );
			float previousFrequency = baseFrequency * 2.0f;
			relativeNoiseMatrix[12] = fmod( relativeNoiseMatrix[12], 1.0 / baseFrequency );
			relativeNoiseMatrix[13] = fmod( relativeNoiseMatrix[13], 1.0 / baseFrequency );
			relativeNoiseMatrix[14] = fmod( relativeNoiseMatrix[14], 1.0 / baseFrequency );

			//Convert to float and pray that it works.
			data->GodRayNoiseMatrix = Transpose( Matrix4dToMatrix( relativeNoiseMatrix ) );



			data->GodRayNoiseFrequency = baseFrequency;
			data->GodRayNoiseLerp = ( exp2f( -m_froxelFogSettings.godRayNoiseFrequency.value ) - baseFrequency ) / ( previousFrequency - baseFrequency );
			data->GodRayNoiseIntensity = m_froxelFogSettings.godRayNoiseIntensity.value;
			data->GodRayNoiseAnimation = (float)m_godRayNoiseAnimation;
		}

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

		Matrix originShiftMatrix = TranslationMatrix( (float)-originShift.x, (float)-originShift.y, (float)-originShift.z );
		Matrix reprojectionMatrix = inverseView * originShiftMatrix * viewLast;
		data->ReprojectionMatrix = Transpose( reprojectionMatrix );

		Vector4 tempSunDir = Vector4( -sunDirection, 0.0f ) * view;
		data->SunViewDirection = Vector3( tempSunDir.x, tempSunDir.y, tempSunDir.z );
		data->SunAngle = m_sunAngle;
		data->SunWorldDirection = -sunDirection;

		data->SunColor = Vector3( sunColor.r, sunColor.g, sunColor.b );

		for( int32_t i = 0; i < m_planets.size(); i++ )
		{
			data->planets[i] = m_planets[i];
		}

		if( cascadedShadowMap != nullptr )
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
		}

		if( auto lightManager = Tr2LightManager::GetInstance() )
		{
			CCP_ASSERT_M( lightManager->GetVolumetricLights().size() <= 16, "LightManager does not meet expectation of VolumetricsRenderer!" );

			data->NumDynamicLights = (uint32_t)lightManager->GetVolumetricLights().size();
			data->InverseShadowMapAtlasSize = lightManager->GetShadowMapAtlasSettings().actualTextureSize > 0 ?
				1.f / lightManager->GetShadowMapAtlasSettings().actualTextureSize :
				0.f;
			data->ShadowMapAtlasEntryMinSizeLog2 = lightManager->GetShadowMapAtlasSettings().entryMinSizeLog2;

			for( uint32_t i = 0; i < lightManager->GetVolumetricLights().size(); i++ )
			{
				uint32_t lightIndex = lightManager->GetVolumetricLights()[i];
				data->DynamicLights[i] = lightManager->GetLightData( lightIndex );
			}

			data->LightProfileTextureWidth = (float)lightManager->GetLightProfileArray().GetWidth();
		}
		else
		{
			data->NumDynamicLights = 0;
			data->InverseShadowMapAtlasSize = 0.f;
			data->ShadowMapAtlasEntryMinSizeLog2 = 0;
		}
	}
}

void Tr2VolumetricsRenderer::PopulatePerFrameData( FroxelPerFrameData& data )
{
	float maxDistance = m_gameBackClip;
	float maxDistanceVisibility = exp( -m_froxelFogSettings.thickness.value );
	float baseDensity = m_froxelFogSettings.thickness.value / maxDistance;

	float environmentG = -std::clamp( m_froxelFogSettings.environmentDirectionality.value, 0.001f, 0.999f );


	auto fogColor = m_froxelFogSettings.fogColor.value;
	data.FogColor = Vector3( fogColor.r, fogColor.g, fogColor.b );
	data.BackgroundVisibility = std::clamp( m_froxelFogSettings.backgroundVisibility.value, 0.0f, 1.0f );

	data.BaseDensity = baseDensity;
	data.MaxDistance = maxDistance;
	data.MaxDistanceVisibility = maxDistanceVisibility;
	data.EnvironmentIntensity = m_froxelFogSettings.environmentIntensity.value;

	data.EnvironmentG = environmentG;

	for( int32_t i = 0; i < m_planets.size(); i++ )
	{
		data.planets[i] = m_planets[i];
	}
}

void Tr2VolumetricsRenderer::SetPlanets( const CcpMath::Sphere* planets, size_t planetCount )
{
	CCP_ASSERT_M( planetCount == 2, "Tr2VolumetricsRenderer expects two planets!" );

	memcpy( m_planets.data(), planets, planetCount * sizeof( CcpMath::Sphere ) );
}



void Tr2VolumetricsRenderer::SetSunAngle( float angle )
{
	m_sunAngle = angle;
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
	GlobalStore().RegisterVariable( "EveSceneMieEnvironmentMap", m_mieEnvironmentMap );
}
