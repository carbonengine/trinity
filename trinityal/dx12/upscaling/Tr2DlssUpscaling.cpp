////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2DlssUpscaling.h"
#include "../Tr2TextureALDx12.h"
#include "../Tr2RenderContextDx12.h"
#include "../Tr2PrimaryRenderContextDx12.h"
#include "../Tr2VideoAdapterInfoALDx12.h"
#include "../Utilities.h"
#include "include/Tr2StreamlineAL.h"

extern bool g_upscalingDebug;
extern uint32_t g_streamlineAppID;

CCP_STATS_DECLARED_ELSEWHERE( generatedFrames );

namespace DlssUtils
{
sl::Resource GenerateTextureResource( Tr2TextureAL* texture )
{
	sl::Resource resource;
	if( texture && texture->IsValid() )
	{
		auto trinityAlImpObject = texture->TrinityALImpl_GetObject();
		return { sl::ResourceType::eTex2d, trinityAlImpObject->GetResourceDx12(), (uint32_t)trinityAlImpObject->GetResourceState() };
	}

	return sl::Resource{ sl::ResourceType::eTex2d, nullptr };
}

}

Tr2DlssUpscalingTechnique::Tr2DlssUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	TrinityALImpl::Tr2UpscalingTechniqueDx12( renderContext, technique, setting, frameGeneration, adapter ),
	m_adapter( adapter ),
	m_frameToken( 0 ), 
	m_supportsFrameGeneration( false ),
	m_contextIndex( 0 )
{
	m_isAvailable = false;
	m_streamlineSetup = false;

	
	//We need to create a dummy device to figure out if DLSS and frame generation actually is supported.
	{
		if( SL_FAILED( res, Tr2StreamlineAL::InitializeStreamline( g_streamlineAppID ) ) )
		{
			CCP_LOGERR( "Streamline initialization failed with error %d", res );
			return;
		}


		CComPtr<ID3D12Device> device;
		CComPtr<IDXGIAdapter1> dxgiAdapter;
		CComPtr<IDXGIOutput> output;
		TrinityALImpl::GetVideoAdapter( adapter, &dxgiAdapter, &output );
		if( SUCCEEDED( D3D12CreateDevice( dxgiAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS( &device ) ) ) )
		{

			if( Tr2StreamlineAL::SetDevice( device, adapter ) == sl::Result::eOk )
			{
				m_isAvailable = Tr2StreamlineAL::IsDLSSAvailable();
				m_supportsFrameGeneration = Tr2StreamlineAL::IsFrameGenerationAvailable();
			}
		}

		Tr2StreamlineAL::ReleaseStreamline();
	}


	//We're done gathering info, initialize Streamline again and await the actual device!
	if( SL_FAILED( res, Tr2StreamlineAL::InitializeStreamline( g_streamlineAppID ) ) )
	{
		CCP_LOGERR( "Streamline initialization failed with error %d", res );
		return;
	}

	m_streamlineSetup = true;

	SanitizeState();
}

Tr2DlssUpscalingTechnique::~Tr2DlssUpscalingTechnique()
{
	m_renderContext.FlushAndSyncDx12();

	Tr2StreamlineAL::ReleaseStreamline(); //this should be enough, no need to shut down plugins manually.
}

bool Tr2DlssUpscalingTechnique::IsAvailable( ) const 
{
	return m_streamlineSetup && m_isAvailable;
}

std::vector<Tr2UpscalingAL::Setting> Tr2DlssUpscalingTechnique::GetAvailableSettings() const
{
	return {
		Tr2UpscalingAL::Setting::QUALITY,
		Tr2UpscalingAL::Setting::BALANCED,
		Tr2UpscalingAL::Setting::PERFORMANCE,
		Tr2UpscalingAL::Setting::ULTRA_PERFORMANCE
	};
}

bool Tr2DlssUpscalingTechnique::IsTemporal() const
{
	return true;
}

bool Tr2DlssUpscalingTechnique::SupportsFrameGeneration( ) const
{
	return m_supportsFrameGeneration;
}

bool Tr2DlssUpscalingTechnique::ReplacesDevice() const
{
	return true;
}

bool Tr2DlssUpscalingTechnique::ReplacesCommandQueue() const
{
	return false;
}

bool Tr2DlssUpscalingTechnique::ReplacesFactory() const
{
	return true;
}

CComPtr<ID3D12Device> Tr2DlssUpscalingTechnique::ReplaceDevice( CComPtr<ID3D12Device>& nativeDevice )
{
	if( SL_FAILED( res, Tr2StreamlineAL::SetDevice( nativeDevice, m_adapter ) ) )
	{
		CCP_LOGWARN( "Could not attach NVidia Streamline to device (%d)", res );
		return nativeDevice;
	}
	CComPtr<ID3D12Device> proxy = nativeDevice;
	if( SL_FAILED( res, Tr2StreamlineAL::UpgradeInterface( (void**)&proxy.p ) ) )
	{
		CCP_LOGWARN( "Could not upgrade device to sl proxy device (%d)", res );
		return nativeDevice;
	}

	CCP_ASSERT_M( m_isAvailable == Tr2StreamlineAL::IsDLSSAvailable(), "DLSS is unexpectedly unavailable!" );
	CCP_ASSERT_M( m_supportsFrameGeneration == Tr2StreamlineAL::IsFrameGenerationAvailable(), "Frame generation is unexpectedly unavailable!" );

	if( m_frameGeneration )
	{
		auto reflexConst = sl::ReflexOptions{};
		reflexConst.mode = sl::ReflexMode::eLowLatency;

		if( SL_FAILED( result, Tr2StreamlineAL::SetReflexOptions( reflexConst ) ) )
		{
			CCP_LOGERR( "Reflex failed to set options (%d)", result );
		}
	}
	
	return proxy;
}

CComPtr<ID3D12CommandQueue> Tr2DlssUpscalingTechnique::ReplaceCommandQueue( CComPtr<ID3D12CommandQueue>& native )
{
	CComPtr<ID3D12CommandQueue> proxy = native;

	if( SL_FAILED( res, Tr2StreamlineAL::UpgradeInterface( (void**)&proxy.p ) ) )
	{
		CCP_LOGWARN( "Could not upgrade device to sl proxy device (%d)", res );
		return native;
	}
	return proxy;
}

CComPtr<IDXGIFactory4> Tr2DlssUpscalingTechnique::ReplaceFactory( CComPtr<IDXGIFactory4>& native )
{
	CComPtr<IDXGIFactory4> proxy = native;

	if( SL_FAILED( res, Tr2StreamlineAL::UpgradeInterface( (void**)&proxy.p ) ) )
	{
		CCP_LOGWARN( "Could not upgrade device to sl proxy device (%d)", res );
		return native;
	}
	return proxy;
}


void Tr2DlssUpscalingTechnique::MarkFrameEvent( Tr2RenderContextEnum::FrameEvent& frameEvent )
{
	Tr2UpscalingTechniqueDx12::MarkFrameEvent( frameEvent );


	if( frameEvent == Tr2RenderContextEnum::FRAME_EVENT_RENDERING_STARTED )
	{
		if( SL_FAILED( res, Tr2StreamlineAL::GetNewFrameToken( m_frameToken ) ) )
		{
			CCP_LOGERR( "Could not get new frame token for Nvidia Streamline (%d)", res );
		}
		for( auto& context : m_contexts )
		{
			auto dlssContext = (Tr2DlssUpscalingContext*)( context.second.get() );
			dlssContext->SetFrameToken( m_frameToken );
		}
	}

	if (m_frameGeneration)
	{
		Tr2StreamlineAL::SetPCLMarker( frameEvent, m_frameToken );
	}
}

Tr2UpscalingContextAL* Tr2DlssUpscalingTechnique::CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params )
{
	return new Tr2DlssUpscalingContext( m_setting, m_frameGeneration, params, ++m_contextIndex, m_frameToken );
}

Tr2DlssUpscalingContext::Tr2DlssUpscalingContext( 
	Tr2UpscalingAL::Setting setting, 
	bool frameGeneration,
	Tr2UpscalingAL::UpscalingContextParams params,
	uint32_t contextNumber, 
	sl::FrameToken* frameToken ) :
	Tr2UpscalingContextAL( setting, frameGeneration, params ),
	m_dlssMode( sl::DLSSMode::eOff ),
	m_viewHandle( sl::ViewportHandle( contextNumber ) ), 
	m_frameToken( frameToken ),
	m_setup( false ),
	m_vramUsage( 0 ),
	m_minWidthHeight( 0 ),
	m_actualFrames( 0 )
{
	m_dlssMode = sl::DLSSMode::eOff;
	switch( m_setting )
	{
	case Tr2UpscalingAL::NATIVE:
		m_dlssMode = sl::DLSSMode::eDLAA;
		break;
	case Tr2UpscalingAL::ULTRA_QUALITY:
		m_dlssMode = sl::DLSSMode::eUltraQuality;
		break;
	case Tr2UpscalingAL::QUALITY:
		m_dlssMode = sl::DLSSMode::eMaxQuality;
		break;
	case Tr2UpscalingAL::BALANCED:
		m_dlssMode = sl::DLSSMode::eBalanced;
		break;
	case Tr2UpscalingAL::PERFORMANCE:
		m_dlssMode = sl::DLSSMode::eMaxPerformance;
		break;
	case Tr2UpscalingAL::ULTRA_PERFORMANCE:
		m_dlssMode = sl::DLSSMode::eUltraPerformance;
		break;
	default:
		CCP_LOGERR( "Invalid Setting Applied to Tr2DlssUpscaling: %d", m_setting );
		break;
	}

	m_dlssOptions.outputWidth = m_displayWidth;
	m_dlssOptions.outputHeight = m_displayHeight;
	m_dlssOptions.mode = m_dlssMode;

	if( SL_FAILED( result, Tr2StreamlineAL::GetDLSSOptimalSettings( m_dlssOptions, m_optimalSettings ) ) )
	{
		CCP_LOGERR( "Getting Optimal Settings for DLSS failed (%d)", result );
		return;
	}
	m_renderWidth = m_optimalSettings.optimalRenderWidth;
	m_renderHeight = m_optimalSettings.optimalRenderHeight;

	if( m_renderWidth == 0 || m_renderHeight == 0 )
	{
		m_renderWidth = m_displayWidth;
		m_renderHeight = m_displayHeight;
	}

	m_upscaling = (float)m_dlssOptions.outputHeight / (float)m_renderHeight;

	m_jitterSequence = Tr2UpscalingAL::GenerateHaltonSequence( 8 * (uint32_t)powf( m_upscaling, 2.0f ), 2, 3 );

	if( m_frameGeneration )
	{
		UpdateDlssG();
	}

	if( m_frameGeneration )
	{
		m_dlssgOptions.mode = sl::DLSSGMode::eOn;

		m_dlssgOptions.colorWidth = m_dlssOptions.outputWidth;
		m_dlssgOptions.colorHeight = m_dlssOptions.outputHeight;

		m_dlssgOptions.colorBufferFormat = m_params.sourceFormat;
		m_dlssgOptions.depthBufferFormat = m_params.depthFormat;
		m_dlssgOptions.mvecBufferFormat = Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_FLOAT;

		m_dlssgOptions.mvecDepthWidth = m_renderWidth;
		m_dlssgOptions.mvecDepthHeight = m_renderHeight;

		if( SL_FAILED( res, Tr2StreamlineAL::SetDLSSGOptions( m_viewHandle, m_dlssgOptions ) ) )
		{
			CCP_LOGERR( "Setting DLSSG Options failed with (%d)", res );
			return;
		}

		CCP_LOGNOTICE( "DLSSG Options set successfully" );
	}

	m_dlssOptions.colorBuffersHDR = sl::eTrue;
	m_dlssOptions.mode = m_dlssMode;
	m_dlssOptions.useAutoExposure = sl::eFalse;
	m_dlssOptions.alphaUpscalingEnabled = sl::eFalse;

	if( SL_FAILED( res, Tr2StreamlineAL::SetDLSSOptions( m_viewHandle, m_dlssOptions ) ) )
	{
		CCP_LOGERR( "Setting DLSS Options failed with (%d)", res );
		return;
	}

	CCP_LOGNOTICE( "DLSS Options set successfully" );
	m_setup = true;
}

Tr2DlssUpscalingContext::~Tr2DlssUpscalingContext()
{
	Tr2StreamlineAL::FreeResources( sl::kFeatureDLSS, m_viewHandle );

	m_params.renderContext.FlushAndSyncDx12();
}

void Tr2DlssUpscalingContext::SetupForReuse()
{
	Tr2StreamlineAL::FreeResources( sl::kFeatureDLSS, m_viewHandle );
}

sl::Result Tr2DlssUpscalingContext::UpdateDlssG()
{
	if( SL_FAILED( res, Tr2StreamlineAL::GetDLSSGState( m_viewHandle, m_dlssgState, &m_dlssgOptions ) ) )
	{
		CCP_LOGERR( "Getting DLSSG State failed with (%d) - disabling framegeneration", res );
		m_frameGeneration = false;
		switch( m_dlssgState.status )
		{
		case sl::DLSSGStatus::eFailResolutionTooLow:
			CCP_LOGERR( "DLSS - Frame generation disabled because the resolution is too small (%d, %d). Minimum resolution is %d", m_dlssOptions.outputWidth, m_dlssOptions.outputHeight, m_dlssgState.minWidthOrHeight );
			break;
		case sl::DLSSGStatus::eFailReflexNotDetectedAtRuntime:
			CCP_LOGERR( "DLSS - Frame generation disabled because Nvidia Reflex was not detected" );
			break;
		case sl::DLSSGStatus::eFailHDRFormatNotSupported:
			CCP_LOGERR( "DLSS - Frame generation disabled because HDR format is not supported" );
			break;
		case sl::DLSSGStatus::eFailCommonConstantsInvalid:
			CCP_LOGERR( "DLSS - Frame generation disabled because some common constants are invalid" );
			break;
		case sl::DLSSGStatus::eFailGetCurrentBackBufferIndexNotCalled:
			CCP_LOGERR( "DLSS - Frame generation disabled because D3D Integration is not using SwapChain::GetCurrentBackBufferIndex API" );
			break;
		default:
			break;
		}
		return res;
	}

	CCP_STATS_SET( generatedFrames, m_dlssgState.numFramesActuallyPresented );

	return sl::Result::eOk;
}

bool Tr2DlssUpscalingContext::HasSharpening() const
{
	// nvidia has their own sharpening which is probably faster,
	// but their FreeResources (for nis) is not working properly,
	// will reimplement when the bug is fixed: https://github.com/NVIDIA-RTX/Streamline/issues/85#issuecomment-3255500539
	return false;
}

void Tr2DlssUpscalingContext::UpdateJitter()
{
	if( m_setup )
	{
		m_jitterX = m_jitterSequence[m_jitterIndex].first;
		m_jitterY = m_jitterSequence[m_jitterIndex].second;

		m_jitterIndex = ++m_jitterIndex % m_jitterSequence.size();
	}
}

uint32_t Tr2DlssUpscalingContext::GetDispatchRequirements() const
{
	return Tr2UpscalingAL::DispatchRequirements::DEPTH | Tr2UpscalingAL::DispatchRequirements::OPAQUE_ONLY | Tr2UpscalingAL::DispatchRequirements::OPTIONAL_EXPOSURE | Tr2UpscalingAL::DispatchRequirements::VELOCITY;
}

sl::Result Tr2DlssUpscalingContext::EvaluateDLSS( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	auto& renderContext = m_params.renderContext;

	auto inputResource = DlssUtils::GenerateTextureResource( dispatchParameters.input );
	auto outputResource = DlssUtils::GenerateTextureResource( dispatchParameters.output );
	auto depthResource = DlssUtils::GenerateTextureResource( dispatchParameters.depth );
	auto velocityResource = DlssUtils::GenerateTextureResource( dispatchParameters.velocity ); 
	auto exposureResource = DlssUtils::GenerateTextureResource( dispatchParameters.exposure ); 
	auto opaqueOnlyResource = DlssUtils::GenerateTextureResource( dispatchParameters.opaqueOnly );

	sl::ResourceTag opaqueColorInTag = sl::ResourceTag{ &opaqueOnlyResource, sl::kBufferTypeOpaqueColor, sl::ResourceLifecycle::eValidUntilEvaluate };
	sl::ResourceTag colorInTag = sl::ResourceTag{ &inputResource, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilEvaluate };
	sl::ResourceTag colorOutTag = sl::ResourceTag{ &outputResource, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilEvaluate };
	sl::ResourceTag depthTag = sl::ResourceTag{ &depthResource, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilEvaluate };
	sl::ResourceTag mvecTag = sl::ResourceTag{ &velocityResource, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilEvaluate };
	sl::ResourceTag exposureTag = sl::ResourceTag{ &exposureResource, sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilEvaluate };
	
	const sl::BaseStructure* resources[] = { &m_viewHandle, &colorInTag, &opaqueColorInTag, &colorOutTag, &depthTag, &mvecTag, &exposureTag };

	return Tr2StreamlineAL::EvaluateFeature( renderContext, sl::kFeatureDLSS, *m_frameToken, resources, _countof( resources ) );
}

void Tr2DlssUpscalingContext::SetCommonConstants( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	m_commonConstants = sl::Constants{};
	m_commonConstants.cameraAspectRatio = dispatchParameters.aspectRatio;
	m_commonConstants.cameraFar = dispatchParameters.backClip;
	m_commonConstants.cameraFOV = dispatchParameters.fieldOfView;
	m_commonConstants.cameraMotionIncluded = sl::eTrue;
	m_commonConstants.cameraNear = dispatchParameters.frontClip;
	m_commonConstants.cameraPinholeOffset = sl::float2( 0, 0 );

	m_commonConstants.cameraFwd = sl::float3( dispatchParameters.cameraForward[0], dispatchParameters.cameraForward[1], dispatchParameters.cameraForward[2] );
	m_commonConstants.cameraPos = sl::float3( dispatchParameters.cameraPos[0], dispatchParameters.cameraPos[1], dispatchParameters.cameraPos[2] );
	m_commonConstants.cameraRight = sl::float3( dispatchParameters.cameraRight[0], dispatchParameters.cameraRight[1], dispatchParameters.cameraRight[2] );
	m_commonConstants.cameraUp = sl::float3( dispatchParameters.cameraUp[0], dispatchParameters.cameraUp[1], dispatchParameters.cameraUp[2] );

	m_commonConstants.cameraViewToClip = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.projection );
	m_commonConstants.clipToCameraView = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.invProjection );
	m_commonConstants.clipToPrevClip = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.clipToPrevClip );
	m_commonConstants.depthInverted = sl::eTrue;
	m_commonConstants.jitterOffset = sl::float2( m_jitterX, m_jitterY );
	m_commonConstants.motionVectors3D = sl::eFalse;
	m_commonConstants.motionVectorsDilated = sl::eFalse;
	m_commonConstants.motionVectorsJittered = sl::eFalse;
	m_commonConstants.mvecScale = sl::float2( 1, 1 );
	m_commonConstants.orthographicProjection = sl::eFalse;
	m_commonConstants.prevClipToClip = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.prevClipToClip );
	m_commonConstants.reset = dispatchParameters.reset ? sl::eTrue : sl::eFalse;

	if( SL_FAILED( result, Tr2StreamlineAL::SetConstants( m_commonConstants, *m_frameToken, m_viewHandle ) ) )
	{
		CCP_LOGERR( "Setting Nvidia Streamline common constants failed (%d)", result );
	}
}

Tr2UpscalingAL::Result Tr2DlssUpscalingContext::Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	if( !m_setup )
	{
		return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
	}

	if( !AreDispatchParametersValid( dispatchParameters ) )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}
	auto& renderContext = m_params.renderContext;

	renderContext.FlushBarriersDx12();

	SetCommonConstants( dispatchParameters );

	if( m_frameGeneration )
	{
		UpdateDlssG();
	}

	if( SL_FAILED( result, EvaluateDLSS( dispatchParameters ) ) )
	{
		CCP_LOGERR( "DLSS Failed to dispatch DLSS (%d) - %s", result, Tr2StreamlineAL::GetSlResultMessage( result ) );
	}

	// the descriptor cache is dirty, mark it so
	renderContext.DirtyDescriptorCache();
	renderContext.FlushBarriersDx12();

	return Tr2UpscalingAL::OK;
}

void Tr2DlssUpscalingContext::SetFrameToken( sl::FrameToken* token )
{
	m_frameToken = token;
}

void Tr2DlssUpscalingContext::SetHudLessTexture( Tr2TextureAL* texture )
{
	if( !m_frameGeneration )
	{
		return;
	}
	sl::Resource resource = DlssUtils::GenerateTextureResource( texture );
	sl::ResourceTag tag = sl::ResourceTag{ &resource, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eOnlyValidNow };

	if( SL_FAILED( res, Tr2StreamlineAL::SetTagsForFrame( m_params.renderContext, *m_frameToken, m_viewHandle, &tag, 1 ) ) )
	{
		CCP_LOGERR( "Failed to tag HudLess texture (%d)", res );
	}
}

#endif
