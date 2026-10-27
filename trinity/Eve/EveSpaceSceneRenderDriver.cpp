// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveSpaceSceneRenderDriver.h"
#include "EveSpaceScene.h"
#include "../Tr2Renderer.h"
#include "../TriProjection.h"
#include "../TriView.h"
#include "../Shader/Tr2Effect.h"
#include "../RenderJob/TriStepRenderFps.h"
#include "../Tr2SSAO.h"
#include "Eve/EveCamera.h"
#include "../Particle/Tr2GpuParticleSystem.h"
#include "../Tr2TextureReference.h"
#include "Resources/TriTextureRes.h"
#include "Tr2ReflectionProbe.h"


CCP_STATS_DECLARE( updateDynamicLightLists, "Trinity/EveSpaceScene/updateDynamicLights", true, CST_TIME, "Time took to gather dynamic lights for EveSpaceScene" );

extern bool g_eveSpaceSceneDynamicLighting;
extern bool g_lensflaresInReflections;

namespace
{

void BeginRenderPass( Tr2RenderContext& renderContext, const std::initializer_list<std::pair<Tr2TextureAL, uint32_t>>& colorAttachments, const Tr2TextureAL& depthAttachment )
{
	uint32_t index = 0;
	for( auto& colorAttachment : colorAttachments )
	{
		renderContext.m_esm.SetRenderTarget( index++, colorAttachment.first, true, colorAttachment.second );
	}
	// clear out remaining slots, generously assuming max 4 render targets
	const uint32_t maxRenderTargets = 4;
	for( ; index < maxRenderTargets; ++index )
	{
		renderContext.m_esm.SetRenderTarget( index, Tr2TextureAL{} );
	}
	renderContext.m_esm.SetDepthStencilBuffer( depthAttachment );
	renderContext.m_esm.SetFullScreenViewport();
}

Tr2GpuResourcePool::Texture GetEmptySSAO( Tr2GpuResourcePool& gpuResourcePool )
{
	const uint8_t white[] = { 255, 255, 255, 255 };
	Tr2SubresourceData initData = { white, 4, 4 };

	return gpuResourcePool.GetPersistentTexture( "EmptySSAO", 1, 1, ImageIO::PIXEL_FORMAT_R8G8B8A8_UNORM, Tr2GpuUsage::SHADER_RESOURCE, &initData );
}

void DeleteUpscalingContext( Tr2UpscalingContextAL* context )
{
	if( context )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		context->SetHudLessTexture( nullptr );
		renderContext.DeleteUpscalingContext( context->GetID() );
	}
}

void ApplyDistortion( const Tr2TextureAL& destination, const Tr2TextureAL& distortion, Tr2GpuResourcePool& gpuResourcePool, const Tr2EffectPtr& effect, Tr2RenderContext& renderContext )
{
	auto backBufferCopy = gpuResourcePool.GetTempTexture( "backBufferCopy", destination.GetWidth(), destination.GetHeight(), destination.GetFormat(), Tr2GpuUsage::COPY_DESTINATION | Tr2GpuUsage::SHADER_RESOURCE );
	backBufferCopy->CopySubresourceRegion( {}, destination, {}, renderContext );

	BeginRenderPass( renderContext, { { destination, 0 } }, {} );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
	effect->SetParameter( BlueSharedString( "BlitCurrent" ), backBufferCopy );
	effect->SetParameter( BlueSharedString( "TexDistortion" ), distortion );
	Tr2Renderer::DrawFullScreenWithShader( renderContext, effect );
	effect->SetParameter( BlueSharedString( "BlitCurrent" ), Tr2TextureAL{} );
	effect->SetParameter( BlueSharedString( "TexDistortion" ), Tr2TextureAL{} );
}

struct TimeSection
{
	TimeSection( Tr2ProfileTimer& timer, const char* name, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext ) :
		m_timer( timer )
	{
		if( rootTimer.GetCaptureCpuTime() || rootTimer.GetCaptureGpuTime() )
		{
			m_timer.SetStatName( ( rootTimer.GetStatName() + std::string( "/" ) + name ).c_str() );
			m_timer.SetCaptureCpuTime( rootTimer.GetCaptureCpuTime() );
			m_timer.SetCaptureGpuTime( rootTimer.GetCaptureGpuTime() );
			m_timer.Begin( renderContext );
			m_renderContext = &renderContext;
		}
		else
		{
			m_timer.SetCaptureCpuTime( false );
			m_timer.SetCaptureGpuTime( false );
		}
	}

	TimeSection( const TimeSection& ) = delete;
	TimeSection& operator=( const TimeSection& ) = delete;

	void Stop()
	{
		if( m_renderContext )
		{
			m_timer.End( *m_renderContext );
			m_renderContext = nullptr;
		}
	}

	~TimeSection()
	{
		if( m_renderContext )
		{
			m_timer.End( *m_renderContext );
		}
	}

	Tr2ProfileTimer& m_timer;
	Tr2RenderContext* m_renderContext = nullptr;
};

void SetNamedOutput( const ITr2RenderNode::Span<ITr2RenderNode::TempOutput>& outputs, const char* name, const Tr2GpuResourcePool::Texture& texture )
{
	auto sname = BlueSharedString( name );
	for( auto& output : outputs )
	{
		if( output.name == sname )
		{
			output.texture = texture;
			return;
		}
	}
}

ITr2RenderNode::TempOutput* FindNamedOutput( const ITr2RenderNode::Span<ITr2RenderNode::TempOutput>& outputs, const char* name )
{
	auto sname = BlueSharedString( name );
	auto found = std::find_if( outputs.begin(), outputs.end(), [&]( const auto& output ) { return output.name == sname; } );
	if( found != outputs.end() )
	{
		return found;
	}
	return nullptr;
}

}


EveSpaceSceneRenderDriver::EveSpaceSceneRenderDriver( IRoot* lockobj ) :
	PARENTLOCK( m_toolsScenes ),
	m_gpuResourcePool( &GetGlobalGpuResourcePool() ),
	m_reflectionCorrectionEnabled( true )
{
	m_distortionEffect.CreateInstance();
	m_distortionEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Distortion.fx" );

	m_postProcess.CreateInstance();
	m_fpsRenderer.CreateInstance();


	//We have a few different resolution lookup tables to choose from. Use the highest quality one for now.
	//BeResMan->GetResource( "res:/texture/reflectioncorrection/32x32.dds", "", m_reflectionCorrectionMap );
	BeResMan->GetResource( "res:/texture/reflectioncorrection/128x128.dds", "", m_reflectionCorrectionMap );

	BeResMan->GetResource( "res:/texture/global/black.dds", "", m_blackReflectionCorrectionMap );
}

EveSpaceSceneRenderDriver::~EveSpaceSceneRenderDriver()
{
	DeleteUpscalingContext( m_upscalingContext );
}

void EveSpaceSceneRenderDriver::ReleaseResources( TriStorage s )
{
	if( s == TriStorageFlags::TRISTORAGE_ALL )
	{
		DeleteUpscalingContext( m_upscalingContext );
		m_upscalingContext = nullptr;
		m_upscalingInfo = Tr2UpscalingAL::UpscalingInfo();
	}
}

bool EveSpaceSceneRenderDriver::OnPrepareResources()
{
	return true;
}

void EveSpaceSceneRenderDriver::SetupUpscaling( const TextureSize2D& displaySize )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( m_settings.enableUpscaling )
	{
		uint32_t width = 0;
		uint32_t height = 0;
		if( m_upscalingContext )
		{
			m_upscalingContext->GetDisplayDimensions( width, height );
		}
		if( width != displaySize.width || height != displaySize.height )
		{
			Tr2UpscalingAL::UpscalingContextParams params = Tr2UpscalingAL::UpscalingContextParams( renderContext );
			params.allowFramegen = true;
			params.displayWidth = displaySize.width;
			params.displayHeight = displaySize.height;
			params.sourceFormat = m_internalPixelFormat;
			params.depthFormat = Tr2RenderContextEnum::DSFMT_D32F;

			m_upscalingContext = renderContext.CreateUpscalingContext( params, m_upscalingContext ? m_upscalingContext->GetID() : Tr2UpscalingAL::INVALID_CONTEXT_ID );
			// m_upscalingContext may not have been successfully created, so we need to check it before calling GetUpscalingInfo
			m_upscalingInfo = renderContext.GetPrimaryRenderContext().GetUpscalingInfo( m_upscalingContext ? m_upscalingContext->GetID() : Tr2UpscalingAL::INVALID_CONTEXT_ID );
		}
	}
	else if( m_upscalingContext )
	{
		DeleteUpscalingContext( m_upscalingContext );
		m_upscalingContext = nullptr;
		m_upscalingInfo = Tr2UpscalingAL::UpscalingInfo();
	}
}

void EveSpaceSceneRenderDriver::PropagateSettings()
{
	// AA setting
	if( m_scene->m_sceneDefaultPostProcess )
	{
		if( m_settings.antiAliasingQuality == AntiAliasingQuality::Disabled )
		{
			m_scene->m_sceneDefaultPostProcess->SetTaa( nullptr );
		}
		else
		{
			if( !m_scene->m_sceneDefaultPostProcess->GetTaaIfAvailable() )
			{
				Tr2PPTaaEffectPtr taa;
				taa.CreateInstance();
				m_scene->m_sceneDefaultPostProcess->SetTaa( taa );
			}
			m_scene->m_sceneDefaultPostProcess->GetTaaIfAvailable()->m_quality = (Tr2PPTaaEffect::Quality)m_settings.antiAliasingQuality;
		}
	}

	// Shadow quality setting
	if( m_settings.shadowQuality == ShadowQuality::SHADOW_DISABLED )
	{
		m_scene->m_cascadedShadowMap = nullptr;
		m_scene->m_rtManager = nullptr;
	}
	else if( m_settings.shadowQuality == ShadowQuality::SHADOW_RAYTRACED )
	{
		m_scene->m_cascadedShadowMap = nullptr;
		if( !m_scene->m_rtManager )
		{
			m_scene->m_rtManager.CreateInstance();
		}
	}
	else
	{
		m_scene->m_rtManager = nullptr;
		if( !m_scene->m_cascadedShadowMap )
		{
			m_scene->m_cascadedShadowMap.CreateInstance();
		}
	}
	m_scene->m_shadowQuality = m_settings.shadowQuality;

	// AO quality setting
	if( m_ssao )
	{
		if( m_settings.aoQuality == AmbientOcclusionQuality::Disabled )
		{
			m_ssao->Enable( false );
		}
		else
		{
			m_ssao->Enable( true );

			switch( m_settings.aoQuality )
			{
			case AmbientOcclusionQuality::Low:
				m_ssao->SetQuality( SSAOQuality::LOW, true );
				break;
			case AmbientOcclusionQuality::Medium:
				m_ssao->SetQuality( SSAOQuality::MEDIUM, false );
				break;
			default:
				m_ssao->SetQuality( SSAOQuality::HIGHEST, false );
				break;
			}
		}
	}

	if( m_settings.visualizeMethod != EveSpaceScene::VM_NONE )
	{
		m_postProcess->SetPostProcessingQuality( PostProcess::LOW );
	}
	else
	{
		m_postProcess->SetPostProcessingQuality( m_settings.postProcessingQuality );
	}

	if( m_scene->m_volumetricsRenderer )
	{
		m_scene->m_volumetricsRenderer->SetQuality( m_settings.volumetricQuality );
	}

	// The reflection quality is set from the global variable g_eveReflectionMode, which is set from the graphics settings.
	// Leaving the global in for now, until a broader refactor is done.
	switch( g_eveReflectionMode )
	{
	case EntityComponents::ReflectionSetting::REFLECTION_SETTING_OFF:
		m_settings.reflectionQuality = ReflectionQuality::OFF;
		break;
	case EntityComponents::ReflectionSetting::REFLECTION_SETTING_LOW:
		m_settings.reflectionQuality = ReflectionQuality::LOW;
		break;
	case EntityComponents::ReflectionSetting::REFLECTION_SETTING_MEDIUM:
		m_settings.reflectionQuality = ReflectionQuality::MEDIUM;
		break;
	case EntityComponents::ReflectionSetting::REFLECTION_SETTING_HIGH:
		m_settings.reflectionQuality = ReflectionQuality::HIGH;
		break;
	case EntityComponents::ReflectionSetting::REFLECTION_SETTING_ULTRA:
		m_settings.reflectionQuality = ReflectionQuality::ULTRA;
		break;
	default:
		break;
	}
}

EveSpaceSceneRenderDriver::Features EveSpaceSceneRenderDriver::Features::Main()
{
	Features features;
	features.upscaling = true;
	features.antiAliasing = true;
	features.ssao = true;
	features.shadows = true;
	features.velocityMap = true;
	features.background = true;
	features.overlay = true;
	features.dynamicLights = true;
	return features;
}

EveSpaceSceneRenderDriver::Features EveSpaceSceneRenderDriver::Features::ReflectionFace()
{
	// faces are rendered without TAA/upscaling, SSAO, velocity or the render nodes
	Features features;
	features.upscaling = false;
	features.antiAliasing = false;
	features.ssao = false;
	features.shadows = true;
	features.velocityMap = false;
	features.background = false;
	features.overlay = false;
	features.dynamicLights = true;
	return features;
}

// --------------------------------------------------------------------------------------
// Description:
//   Set the camera's view transform to the renderer and the camera to the scene, which
//   also applies its projection to the renderer.
// --------------------------------------------------------------------------------------
void EveSpaceSceneRenderDriver::ApplyCamera( const EveSceneView::Camera& camera ) const
{
	Tr2Renderer::SetViewTransform( camera.view );
	m_scene->SetCamera( camera );
}

// --------------------------------------------------------------------------------------
// Description:
//   The scene's per-frame update, done once and from the main camera before any view is
//   rendered, so the frame's object updates and LOD follow the main view.
// --------------------------------------------------------------------------------------
void EveSpaceSceneRenderDriver::UpdateSceneFrame( const EveSceneView::Camera& mainCamera, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext )
{
	ApplyCamera( mainCamera );
	{
		TimeSection updateSection( m_timers.update, "Update", rootTimer, renderContext );
		m_scene->Update( realTime, simTime );
	}
	UpdateGpuParticleSystem( renderContext );
}

bool EveSpaceSceneRenderDriver::Validate( const Span<const Tr2BitmapDimensions>& destDimensions, const Span<const BlueSharedString>& outputs, Be::Time realTime, Be::Time simTime )
{
	if( destDimensions.size == 0 )
	{
		CCP_ASSERT_M( false, "EveSpaceSceneRenderDriver requires at least one destination texture" );
		return false;
	}
	if( !m_enableRendering )
	{
		return true;
	}
	if( !m_scene )
	{
		CCP_LOG( "EveSpaceSceneRenderDriver::Validate failed because the render driver does not have a valid scene" );
		return false;
	}
	bool hasCamera = m_camera || ( m_view && m_projection );
	if( !hasCamera )
	{
		CCP_LOG( "EveSpaceSceneRenderDriver::Validate failed because the render driver does not have a valid camera" );
		return false;
	}
	const TextureSize2D displaySize = destDimensions[0];

	SetupUpscaling( displaySize );
	const auto renderSize = GetRenderSize( displaySize );

	if( m_background )
	{
		auto bgDimensions = Tr2BitmapDimensions( renderSize.width, renderSize.height, 1, m_internalPixelFormat );
		if( !m_background->Validate( { &bgDimensions, 1 }, {}, realTime, simTime ) )
		{
			return false;
		}
	}
	if( m_sceneOverlay )
	{
		Tr2BitmapDimensions overlayDimensions[] = {
			Tr2BitmapDimensions( renderSize.width, renderSize.height, 1, m_internalPixelFormat ),
			Tr2BitmapDimensions( renderSize.width, renderSize.height, 1, ImageIO::PIXEL_FORMAT_R16G16_FLOAT )
		};
		if( !m_sceneOverlay->Validate( { overlayDimensions, 2 }, {}, realTime, simTime ) )
		{
			return false;
		}
	}

	for( auto& output : outputs )
	{
		if( strcmp( output.c_str(), "DepthMap" ) != 0 &&
			strcmp( output.c_str(), "VelocityMap" ) != 0 &&
			strcmp( output.c_str(), "NormalMap" ) != 0 &&
			strcmp( output.c_str(), "CustomStencilMap" ) != 0 )
		{
			CCP_LOGERR( "EveSpaceSceneRenderDriver does not support the output '%s'", output.c_str() );
			return false;
		}
	}
	return true;
}

Tr2GpuResourcePool::Texture EveSpaceSceneRenderDriver::RenderSSAO( const Tr2TextureAL& depthMap, const Tr2TextureAL& normalMap, Tr2RenderContext& renderContext )
{
	Tr2GpuResourcePool::Texture ssao;
	if( m_scene->m_display && m_ssao )
	{
		renderContext.SetReadOnlyDepth( true );

		auto scenePostProcess = m_scene->m_sceneDefaultPostProcess;
		bool temporal = m_upscalingInfo.temporal || ( scenePostProcess != nullptr && scenePostProcess->GetTaaIfAvailable() != nullptr );

		ssao = m_ssao->Filter( depthMap, normalMap, m_gpuResourcePool, renderContext, temporal );
		renderContext.SetReadOnlyDepth( false );
	}
	if( ssao.IsValid() )
	{
		return ssao;
	}
	return GetEmptySSAO( m_gpuResourcePool );
}

void EveSpaceSceneRenderDriver::SetCameraToRenderer( Tr2RenderContext& renderContext ) const
{
	auto projection = m_camera ? m_camera->GetProjection() : m_projection;
	auto view = m_camera ? m_camera->GetViewMatrix() : m_view;

	projection->SetProjection( renderContext );
	Tr2Renderer::SetViewTransform( view->GetTransform() );
}

TextureSize2D EveSpaceSceneRenderDriver::GetRenderSize( const TextureSize2D& displaySize ) const
{
	if( m_upscalingContext )
	{
		TextureSize2D result;
		m_upscalingContext->GetRenderDimensions( result.width, result.height );
		return result;
	}
	return displaySize;
}

EveSceneView::Camera EveSpaceSceneRenderDriver::GetMainCamera( const TextureSize2D& renderSize, const Matrix& jitterMatrix, float mipLodBias ) const
{
	auto projection = m_camera ? m_camera->GetProjection() : m_projection;
	auto view = m_camera ? m_camera->GetViewMatrix() : m_view;
	const float upscalingAmount = m_upscalingContext ? m_upscalingContext->GetUpscalingAmount() : 1.0f;

	return EveSceneView::Camera( view->GetTransform(), projection->GetTransform(), renderSize, jitterMatrix, mipLodBias, upscalingAmount );
}

void EveSpaceSceneRenderDriver::Execute( const Span<const Tr2TextureAL>& destinations, const Span<TempOutput>& outputs, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext )
{
	const bool hasCamera = m_camera || ( m_view && m_projection );

	if( !m_enableRendering )
	{
		if( !m_scene || !hasCamera )
		{
			return;
		}

		const TextureSize2D displaySize = destinations.size > 0 ? TextureSize2D( destinations.data->GetDesc() ) : TextureSize2D();
		ApplyCamera( GetMainCamera( GetRenderSize( displaySize ), IdentityMatrix(), 0.0f ) );
		m_scene->Update( realTime, simTime );
		UpdateGpuParticleSystem( renderContext );
		return;
	}

	if( !m_scene || !hasCamera )
	{
		CCP_LOGWARN( "EveSpaceSceneRenderDriver::Execute called without a valid scene or a camera" );
		return;
	}

	GlobalStore().RegisterVariable( "EveSpaceSceneReflectionCorrectionLookupTable", m_reflectionCorrectionEnabled ? m_reflectionCorrectionMap : m_blackReflectionCorrectionMap );

	m_scene->m_viewLast = m_viewLast;
	m_scene->m_projectionLast = m_projectionLast;

	if( m_camera )
	{
		m_camera->Update( simTime );
	}

	const TextureSize2D displaySize = destinations.data->GetDesc();
	const TextureSize2D renderSize = GetRenderSize( displaySize );

	PropagateSettings();

	GlobalStore().RegisterVariable( "SSAOMap", GetEmptySSAO( m_gpuResourcePool ) );

	renderContext.m_esm.SetInvertedDepthTest( true );
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.SetInvertedDepthTest( false ); } );

	renderContext.m_esm.PushRenderTarget( 0 );
	renderContext.m_esm.PushRenderTarget( 1 );
	renderContext.m_esm.PushDepthStencilBuffer();
	ON_BLOCK_EXIT( [&] {
		renderContext.m_esm.PopDepthStencilBuffer();
		renderContext.m_esm.PopRenderTarget( 1 );
		renderContext.m_esm.PopRenderTarget( 0 );
	} );

	PrepareLightManager();

	const RenderInfo mainView = {
		GetMainCamera( renderSize, GetJitterMatrix( renderSize.width, renderSize.height, renderContext ), GetMipLodBias() ),
		EveSceneView::ViewInfo::Main( m_settings.enableDistortion ),
		Features::Main()
	};
	auto colorTarget = m_gpuResourcePool.GetTempTexture( "customBackBuffer", renderSize, m_internalPixelFormat, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );

	ViewTargets targets;
	{
		auto prevVisualizeMethod = m_scene->m_visualizeMethod;
		m_scene->m_visualizeMethod = m_settings.visualizeMethod;
		ON_BLOCK_EXIT( [&] { m_scene->m_visualizeMethod = prevVisualizeMethod; } );
		BeginRenderPass( renderContext, { { colorTarget, 0 } }, {} );

		UpdateSceneFrame( mainView.camera, realTime, simTime, rootTimer, renderContext );
		ReflectionFaces reflectionFaces;
		{
			TimeSection beginRenderSection( m_timers.beginRender, "BeginRender", rootTimer, renderContext );

			m_scene->BeginFrame( renderContext );

			reflectionFaces = PrepareReflectionFaces( renderContext );

			// the lights are gathered once, for all views
			SetLightViews( mainView, reflectionFaces, renderContext );
			m_scene->GatherDynamicLights( renderContext );
		}
		{
			TimeSection reflectionsSection( m_timers.reflections, "RenderReflections", rootTimer, renderContext );
			RenderReflectionFaces( reflectionFaces, realTime, simTime, rootTimer, renderContext );
		}

		targets = RenderView( mainView, colorTarget, outputs, realTime, simTime, rootTimer, renderContext );
	}

	renderContext.m_esm.SetRenderTarget( 0, *destinations.data );
	renderContext.m_esm.SetDepthStencilBuffer( {} );

	{
		TimeSection postProcessSection( m_timers.postProcess, "PostProcess", rootTimer, renderContext );
		m_postProcess->Execute( *destinations.data, std::move( colorTarget ), targets.depth, std::move( targets.velocity ), std::move( targets.opaqueColor ), m_scene, m_upscalingContext, m_gpuResourcePool, renderContext );
	}
	SetCameraToRenderer( renderContext );
	{
		TimeSection ui3dSection( m_timers.ui3d, "Render3dUI", rootTimer, renderContext );
		m_scene->Render3DUI( renderContext );
	}

	for( auto& toolScene : m_toolsScenes )
	{
		if( ITr2UpdateablePtr updateable = BlueCastPtr( toolScene ) )
		{
			updateable->Update( realTime, simTime );
		}
		toolScene->Render( renderContext );
	}

	GlobalStore().RegisterVariable( "SSAOMap", GetEmptySSAO( m_gpuResourcePool ) );
	GlobalStore().RegisterVariable( "DepthMap", Tr2TextureAL{} );

	if( m_settings.showFPS )
	{
		m_fpsRenderer->Execute( realTime, simTime, renderContext );
	}
}

Matrix EveSpaceSceneRenderDriver::GetJitterMatrix( uint32_t renderWidth, uint32_t renderHeight, Tr2RenderContext& renderContext ) const
{
	Matrix jitterMatrix;

	if( m_upscalingInfo.temporal )
	{
		float jitterX = 0.0f;
		float jitterY = 0.0f;
		m_upscalingContext->GetJitter( jitterX, jitterY );
		jitterMatrix = TranslationMatrix( Vector3( jitterX, jitterY, 0 ) );
	}
	else if( m_scene && m_scene->GetPostProcess() && m_scene->GetPostProcess()->GetTaaIfAvailable() != nullptr )
	{
		auto frameIndex = renderContext.GetPrimaryRenderContext().GetRenderedFrameNumber();
		jitterMatrix = Tr2PostProcessRenderer::GetJitter( frameIndex, renderWidth, renderHeight );
	}
	else
	{
		jitterMatrix = IdentityMatrix();
	}

	return jitterMatrix;
}

float EveSpaceSceneRenderDriver::GetMipLodBias() const
{
	// 0 if no upscaling is enabled
	float mipLodBias = m_upscalingInfo.mipLevelBias;

	// The post process bias belongs to its TAA. A temporal upscaler replaces the TAA and
	// its bias already accounts for it.
	if( !m_upscalingInfo.temporal && m_scene )
	{
		auto postprocess = m_scene->GetPostProcess();
		if( postprocess && postprocess->GetTaaIfAvailable() != nullptr )
		{
			mipLodBias += postprocess->GetMipLodBias();
		}
	}
	return mipLodBias;
}

// --------------------------------------------------------------------------------------
// Description:
//   Render the scene from one view (the main view or a reflection probe face) into
//   colorTarget. Returns the targets the view rendered, for post processing
// --------------------------------------------------------------------------------------
EveSpaceSceneRenderDriver::ViewTargets EveSpaceSceneRenderDriver::RenderView( const RenderInfo& view, const Tr2TextureAL& colorTarget, const Span<TempOutput>& outputs, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext, const uint32_t colorTargetSlice )
{
	const TextureSize2D& renderSize = view.camera.renderSize;

	ViewTargets targets;
	const Tr2TextureAL& backBuffer = colorTarget;
	ApplyCamera( view.camera );

	// every view with lights makes its own view active, so it can't read another view's lights
	if( view.features.dynamicLights )
	{
		if( auto lightManager = Tr2LightManager::GetInstance() )
		{
			lightManager->SetActiveView( view.lightView );
		}
	}

	const bool renderBackgroundNode = m_background && view.features.background;
	if( renderBackgroundNode )
	{
		// render background effect to back buffer before rendering the scene
		BeginRenderPass( renderContext, { { backBuffer, colorTargetSlice } }, {} );
		renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, m_settings.clearColor, 0 );

		m_background->Execute( { &backBuffer, 1 }, {}, realTime, simTime, rootTimer, renderContext );
	}

	targets.depth = m_gpuResourcePool.GetTempTexture( "depthBuffer", renderSize, ImageIO::PIXEL_FORMAT_D32_FLOAT, Tr2GpuUsage::DEPTH_STENCIL | Tr2GpuUsage::SHADER_RESOURCE );
	const Tr2TextureAL& depthBuffer = targets.depth;

	BeginRenderPass( renderContext, { { backBuffer, colorTargetSlice } }, depthBuffer );
	renderContext.Clear( renderBackgroundNode ? Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER : ( Tr2RenderContextEnum::CLEARFLAGS_TARGET | Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER ), m_settings.clearColor, 0 );
	
	{
		TimeSection beginRenderSection( m_timers.beginRender, "BeginRender", rootTimer, renderContext );
		m_scene->BeginView( view.scene, renderContext );
	}

	GlobalStore().RegisterVariable( "DepthMap", targets.depth );

	SetNamedOutput( outputs, "DepthMap", targets.depth );

	Tr2GpuResourcePool::Texture distortionMap = GetDistortionMapIfNeeded( renderSize, view.scene.distortion );
	targets.velocity = GetVelocityMapIfNeeded( renderSize, outputs, view.features );
	const Tr2TextureAL& velocityMap = targets.velocity;

	bool hasBackgroundDistortionBatches = false;
	{
		TimeSection backgroundSection( m_timers.background, "BackgroundPass", rootTimer, renderContext );
		hasBackgroundDistortionBatches = m_scene->RenderBackgroundPass( depthBuffer, distortionMap, velocityMap, view.scene, renderContext );
	}
	if( view.scene.distortion && hasBackgroundDistortionBatches )
	{
		ApplyDistortion( backBuffer, distortionMap, m_gpuResourcePool, m_distortionEffect, renderContext );

		renderContext.m_esm.SetDepthStencilBuffer( depthBuffer );
	}
	distortionMap = {};

	Tr2GpuResourcePool::Texture normalMap = GetNormalMapIfNeeded( renderSize, outputs, view.features );
	Tr2GpuResourcePool::Texture customStencil = GetCustomStencilMapIfNeeded( renderSize, outputs );
	{
		TimeSection depthPassSection( m_timers.depthPass, "DepthPass", rootTimer, renderContext );
		m_scene->RenderDepthPass( depthBuffer, normalMap, customStencil, view.scene, renderContext, m_depthPassTechnique );
	}
	GlobalStore().RegisterVariable( "SpaceSceneNormalMap", normalMap );
	GlobalStore().RegisterVariable( "SpaceSceneCustomStencil", customStencil );
	ON_BLOCK_EXIT( [&] { GlobalStore().RegisterVariable( "SpaceSceneCustomStencil", Tr2TextureAL{} ); } );

	EveSpaceScene::ShadowResources shadowResources;
	if( m_scene->m_display && m_mainPassRenderingEnabled )
	{
		if( view.features.shadows )
		{
			TimeSection shadowsSection( m_timers.shadows, "Shadows", rootTimer, renderContext );
			shadowResources = m_scene->RenderShadows( depthBuffer, normalMap, m_gpuResourcePool, view.scene, renderContext );
		}
		TimeSection ssaoSection( m_timers.ssao, "SSAO", rootTimer, renderContext );
		auto ssao = view.features.ssao ? RenderSSAO( depthBuffer, normalMap, renderContext ) : GetEmptySSAO( m_gpuResourcePool );
		ssaoSection.Stop();
		
		GlobalStore().RegisterVariable( "SSAOMap", ssao );

		auto lightManager = Tr2LightManager::GetInstance();
		if( lightManager && view.features.dynamicLights )
		{
			GPU_REGION( renderContext, "Lighting" );
			TRINITY_STATS_SCOPED_TIME( updateDynamicLightLists );
			renderContext.SetReadOnlyDepth( true );
			lightManager->UpdateLists( depthBuffer, renderContext );
			renderContext.SetReadOnlyDepth( false );
		}

		targets.opaqueColor = GetOpaqueColorMapIfNeeded( renderSize, outputs, view.features );

		RegisterWithVariableStore( shadowResources, m_gpuResourcePool );
		bool hasDistortionBatches = false;
		distortionMap = GetDistortionMapIfNeeded( renderSize, view.scene.distortion );
		{
			TimeSection mainPassSection( m_timers.mainPass, "MainPass", rootTimer, renderContext );
			hasDistortionBatches = m_scene->RenderMainPass( backBuffer, depthBuffer, distortionMap, velocityMap, targets.opaqueColor, m_gpuResourcePool, view.scene, renderContext );
		}
		RegisterWithVariableStore( {}, m_gpuResourcePool );

		if( view.scene.distortion && hasDistortionBatches )
		{
			ApplyDistortion( backBuffer, distortionMap, m_gpuResourcePool, m_distortionEffect, renderContext );
			renderContext.m_esm.SetDepthStencilBuffer( depthBuffer );
		}
		distortionMap = {};
		GlobalStore().RegisterVariable( "SSAOMap", Tr2TextureAL{} );
	}
	else
	{
		// Even if we skip main pass rendering, we might need opaqueBackBuffer for upscaling or TAA
		targets.opaqueColor = GetOpaqueColorMapIfNeeded( renderSize, outputs, view.features );
		if( targets.opaqueColor.IsValid() )
		{
			renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
			BeginRenderPass( renderContext, { { targets.opaqueColor.Get(), 0 } }, {} );
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
			Tr2Renderer::DrawTexture( renderContext, backBuffer );
		}
	}
	if( m_sceneOverlay && view.features.overlay )
	{
		Tr2TextureAL overlayTargets[] = { backBuffer, velocityMap };
		m_sceneOverlay->Execute( { overlayTargets, 2 }, {}, realTime, simTime, rootTimer, renderContext );
	}
	GlobalStore().RegisterVariable( "SpaceSceneNormalMap", Tr2TextureAL{} );
	normalMap = {};

	BeginRenderPass( renderContext, { { backBuffer, colorTargetSlice } }, depthBuffer );
	{
		renderContext.SetReadOnlyDepth( true );
		m_scene->RunLensflareOcclusionQueries( depthBuffer, renderContext );
		renderContext.SetReadOnlyDepth( false );
	}
	{
		TimeSection endRenderSection( m_timers.endRender, "EndRender", rootTimer, renderContext );
		m_scene->EndRender( view.scene, renderContext );
	}
	// non-temporal views leave the scene's history untouched, so this is a no-op for them
	m_viewLast = m_scene->m_viewLast;
	m_projectionLast = m_scene->m_projectionLast;
	m_scene->PopulateAndApplyPerFrameData( renderContext );

	GlobalStore().RegisterVariable( "EveSpaceSceneOpaqueMap", Tr2TextureAL{} );

	return targets;
}

EveSpaceSceneRenderDriver::ReflectionFaces EveSpaceSceneRenderDriver::PrepareReflectionFaces( Tr2RenderContext& renderContext )
{
	ReflectionFaces faces;

	Tr2ReflectionProbe* probe = m_scene->GetReflectionProbe();
	if( !probe || !probe->IsValid() || !m_scene->m_display )
	{
		return faces;
	}

	SetCameraToRenderer( renderContext );
	probe->UpdatePosition( Tr2Renderer::GetViewPosition() );
	const Matrix faceProjection = probe->GetFaceProjection( Tr2Renderer::GetProjectionTransform() );
	const TextureSize2D faceRenderSize = probe->GetCubeTarget().GetDesc();

	faces.probe = probe;
	faces.startFace = probe->GetStartFace();
	faces.viewDirection = Tr2Renderer::GetInverseViewTransform().GetZ();

	const bool lensflares = g_lensflaresInReflections && m_settings.reflectionQuality == ReflectionQuality::ULTRA;
	for( unsigned face = probe->GetStartFace(); face < probe->GetEndFace(); ++face )
	{
		const EveSceneView::Camera camera = EveSceneView::Camera( probe->GetFaceViewTransform( face ), faceProjection, faceRenderSize );
		faces.views.push_back( { camera, EveSceneView::ViewInfo::ReflectionFace( lensflares ), Features::ReflectionFace() } );
	}

	return faces;
}

// --------------------------------------------------------------------------------------
// Description:
//   Give the main view and every reflection face that has dynamic lights a view in the
//   light manager, in that order. A view's lightView is its position in that list.
// --------------------------------------------------------------------------------------
void EveSpaceSceneRenderDriver::SetLightViews( const RenderInfo& mainView, ReflectionFaces& faces, Tr2RenderContext& renderContext )
{
	auto lightManager = Tr2LightManager::GetInstance();
	if( !lightManager )
	{
		return;
	}

	// the main view is first, matching its lightView of 0
	std::vector<TriFrustum> frustums = { mainView.camera.frustum };
	for( RenderInfo& faceView : faces.views )
	{
		if( faceView.features.dynamicLights )
		{
			faceView.lightView = static_cast<uint32_t>( frustums.size() );
			frustums.push_back( faceView.camera.frustum );
		}
	}
	lightManager->SetViews( frustums, renderContext );
}

void EveSpaceSceneRenderDriver::RenderReflectionFaces( const ReflectionFaces& faces, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext )
{
	Tr2ReflectionProbe* probe = faces.probe;
	if( !probe )
	{
		return;
	}

	GPU_REGION( renderContext, "Reflection" );

	const bool prevCullInversion = renderContext.m_esm.IsCullModeInverted();
	renderContext.m_esm.SetInvertedCullMode( true );
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.SetInvertedCullMode( prevCullInversion ); } );

	const Tr2TextureAL& faceTarget = probe->GetCubeTarget();
	unsigned face = faces.startFace;
	for( const RenderInfo& faceView : faces.views )
	{
		std::string regionName = "Reflection Face " + std::to_string( face );
		GPU_REGION( renderContext, regionName.c_str() );

		// render straight into the probe's face
		RenderView( faceView, faceTarget, {}, realTime, simTime, rootTimer, renderContext, face++ );
	}

	// unbind the probe cube so it can be used as a shader resource in the main view
	renderContext.m_esm.SetRenderTarget( 0, Tr2TextureAL{} );
	probe->FinishFaces( faces.viewDirection, renderContext );
	SetCameraToRenderer( renderContext );
}

void EveSpaceSceneRenderDriver::PrepareLightManager()
{
	if( g_eveSpaceSceneDynamicLighting )
	{
		if( auto lightManager = Tr2LightManager::GetOrCreateInstance( "res:/graphics/effect/managed/space/system/computelightlists.fx" ) )
		{
			lightManager->SetLightingQuality( m_settings.lightingQuality );
			lightManager->SetReflectionQuality( m_settings.reflectionQuality );
		}
	}
	else
	{
		Tr2LightManager::DeleteInstance();
	}
}

void EveSpaceSceneRenderDriver::UpdateGpuParticleSystem( Tr2RenderContext& renderContext )
{
	if( m_scene->GetGpuParticleSystem() )
	{
		GPU_REGION( renderContext, "Particles" );
		m_scene->PopulateAndApplyPerFrameData( renderContext );
		m_scene->GetGpuParticleSystem()->Update( m_scene->m_updateTime, m_scene->m_updateContext.GetOriginShift(), renderContext );
	}
}

Tr2GpuResourcePool::Texture EveSpaceSceneRenderDriver::GetDistortionMapIfNeeded( const TextureSize2D& size, bool distortion )
{
	if( distortion )
	{
		return m_gpuResourcePool.GetTempTexture( "distortionMap", size, ImageIO::PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
	}
	return {};
}

Tr2GpuResourcePool::Texture EveSpaceSceneRenderDriver::GetVelocityMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs, const Features& featureSet )
{
	if( !featureSet.velocityMap )
	{
		return {};
	}

	// TAA and temporal upscaling reproject with the velocity map
	const bool needsVelocityMap = ( featureSet.antiAliasing && m_settings.antiAliasingQuality != AntiAliasingQuality::Disabled ) || ( featureSet.upscaling && m_upscalingInfo.temporal );
	auto namedOutput = FindNamedOutput( outputs, "VelocityMap" );
	Tr2GpuResourcePool::Texture velocityMap;
	if( needsVelocityMap || m_settings.forceVelocityMap || namedOutput )
	{
		auto texture = m_gpuResourcePool.GetTempTexture( "velocityMap", size, ImageIO::PIXEL_FORMAT_R16G16_FLOAT, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
		if( namedOutput )
		{
			namedOutput->texture = texture;
		}
		return texture;
	}
	return {};
}

Tr2GpuResourcePool::Texture EveSpaceSceneRenderDriver::GetNormalMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs, const Features& featureSet )
{
	// SSAO and raytraced shadows read the normal map
	const bool needsNormalMap = ( featureSet.ssao && m_settings.aoQuality != AmbientOcclusionQuality::Disabled ) || ( featureSet.shadows && m_settings.shadowQuality == ShadowQuality::SHADOW_RAYTRACED );
	auto namedOutput = FindNamedOutput( outputs, "NormalMap" );
	if( ( needsNormalMap && Tr2Renderer::GetShaderModel() != TR2SM_3_0_LO ) || m_settings.forceNormalMap || namedOutput )
	{
		auto texture = m_gpuResourcePool.GetTempTexture( "normalMap", size, ImageIO::PIXEL_FORMAT_R10G10B10A2_UNORM, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
		if( namedOutput )
		{
			namedOutput->texture = texture;
		}
		return texture;
	}
	return {};
}

Tr2GpuResourcePool::Texture EveSpaceSceneRenderDriver::GetCustomStencilMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs )
{
	auto namedOutput = FindNamedOutput( outputs, "CustomStencilMap" );
	if( m_customStencilFormat != ImageIO::PIXEL_FORMAT_UNKNOWN || namedOutput )
	{
		auto texture = m_gpuResourcePool.GetTempTexture( "customStencil", size, m_customStencilFormat, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
		if( namedOutput )
		{
			namedOutput->texture = texture;
		}
		return texture;
	}
	return {};
}

Tr2GpuResourcePool::Texture EveSpaceSceneRenderDriver::GetOpaqueColorMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs, const Features& featureSet )
{
	// the opaque copy is only used by views that can be upscaled or anti-aliased
	if( !featureSet.upscaling && !featureSet.antiAliasing )
	{
		return {};
	}

	auto namedOutput = FindNamedOutput( outputs, "OpaqueColorMap" );
	bool needsOpaqueBackBuffer = m_upscalingContext != nullptr || m_settings.antiAliasingQuality >= AntiAliasingQuality::Medium || m_settings.forceOpaqueBuffer || namedOutput;
	if( !needsOpaqueBackBuffer )
	{
		// If there are SSSSS batches, we need an opaque back buffer
		auto batches = m_scene->m_primaryBatches[TRIBATCHTYPE_OPAQUE];
		static const auto SSSSS = BlueSharedString( "SSSSS" );
		needsOpaqueBackBuffer = Tr2RenderContext::TechniqueInBatch( batches->GetGdprBatches(), SSSSS ) || Tr2RenderContext::TechniqueInBatch( batches->GetBatches(), SSSSS );
	}
	if( needsOpaqueBackBuffer )
	{
		auto texture = m_gpuResourcePool.GetTempTexture( "opaqueBackBuffer", size, m_internalPixelFormat, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
		if( namedOutput )
		{
			namedOutput->texture = texture;
		}
		return texture;
	}
	return {};
}

std::vector<Tr2TextureReferencePtr> EveSpaceSceneRenderDriver::GetAllTempTextures() const
{
	std::vector<Tr2TextureAL> textures;
	m_gpuResourcePool.DebugGetAllTempTextures( textures );

	std::vector<Tr2TextureReferencePtr> result;
	result.reserve( textures.size() );
	for( auto& tex : textures )
	{
		auto texRef = Tr2TextureReferencePtr();
		texRef.CreateInstance();
		*texRef->GetTexture() = tex;
		result.push_back( texRef );
	}
	return result;
}

void EveSpaceSceneRenderDriver::SetDebugMode( bool enable )
{
	if( enable != GetDebugMode() )
	{
		m_gpuResourcePool.SetDebugMode( enable );
	}
}

bool EveSpaceSceneRenderDriver::GetDebugMode() const
{
	return m_gpuResourcePool.GetDebugMode();
}

EveSpaceScene* EveSpaceSceneRenderDriver::GetScene() const
{
	return m_scene;
}

void EveSpaceSceneRenderDriver::SetScene( EveSpaceScene* scene )
{
	if( m_scene == scene )
	{
		return;
	}
	m_scene = scene;
}
