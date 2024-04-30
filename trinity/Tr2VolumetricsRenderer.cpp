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


Tr2VolumetricsRenderer::Tr2VolumetricsRenderer( IRoot* ) :
	m_quality( Tr2VolumerticQuality::High ),
	m_scaleFactor( 0.7f ),
	m_blur( true ),
	m_volumeHasContent( false ),
	m_castShadows( false ),
	m_receiveShadows( false )
{
	m_volumeSlices.CreateInstance();
	GlobalStore().RegisterVariable( "EveSceneFogVolumeMap", m_volumeSlices );

	m_downsampledDepth.CreateInstance();
	GlobalStore().RegisterVariable( "VolumetricDepthMap", m_downsampledDepth );


	m_volumeBlit.CreateInstance();
	m_volumeBlit->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/VolumeBlit.fx" );

	m_downsampleDepth.CreateInstance();
	m_downsampleDepth->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/DownsampleDepth.fx" );

	m_blurScratch.CreateInstance();

	m_hBlur.CreateInstance();
	m_hBlur->SetOption( BlueSharedString( "SOURCE_TYPE" ), BlueSharedString( "SOURCE_TYPE_ARRAY" ) );
	m_hBlur->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/BlurVolumetric.fx" );

	m_vBlur.CreateInstance();
	m_vBlur->SetOption( BlueSharedString( "SOURCE_TYPE" ), BlueSharedString( "SOURCE_TYPE_TEXTURE" ) );
	m_vBlur->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Volumetric/BlurVolumetric.fx" );
	m_vBlur->SetParameter( BlueSharedString( "SourceMap" ), m_blurScratch );

	m_batches.reset( new TriRenderBatchAccumulator<>( Tr2Renderer::GetPoolAllocator() ) );
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
