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

namespace DlssUtils
{
	sl::Resource GenerateTextureResource( Tr2TextureAL* texture )
	{
		if( texture && texture->IsValid() )
		{
			return { sl::ResourceType::eTex2d, texture->TrinityALImpl_GetObject()->GetResourceDx12(), nullptr, nullptr, (uint32_t)texture->TrinityALImpl_GetObject()->GetResourceState() };
		}

		return { sl::ResourceType::eTex2d, nullptr, nullptr, nullptr, 0 };
	}
}

Tr2DlssUpscalingTechnique::Tr2DlssUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	TrinityALImpl::Tr2UpscalingTechniqueDx12( renderContext, technique, setting, frameGeneration, adapter ),
	m_adapter( adapter ),
	m_frameToken( 0 ), 
	m_slGetFeatureFunction( nullptr ),
	m_slReflexSetOptions( nullptr ),
	m_slSetFeatureLoaded( nullptr ),
	m_slPCLSetMarker( nullptr ),
	m_slGetNewFrameToken( nullptr ),
	m_attachedToDevice( false ),
	m_supportsFrameGeneration( false ),
	m_contextIndex( 0 )
{
	m_isAvailable = false;
	m_streamlineSetup = false;

	m_streamlineModule = Tr2StreamlineAL::GetStreamlineModule();
	if( !m_streamlineModule )
	{
		return;
	}

	if( SL_FAILED( res, Tr2StreamlineAL::InitializeStreamline( m_streamlineModule ) ) )
	{
		return;
	}

	// setup the streamline function calls, now that we have the module
	INITIALIZE_SL_FUNC( m_streamlineModule, slGetFeatureFunction );
	INITIALIZE_SL_FUNC( m_streamlineModule, slSetFeatureLoaded );
	INITIALIZE_SL_FUNC( m_streamlineModule, slGetNewFrameToken );
	INITIALIZE_SL_FUNC( m_streamlineModule, slUpgradeInterface );

	INITIALIZE_SL_FEATURE_FUNC( slReflexSetOptions, sl::kFeatureReflex );
	INITIALIZE_SL_FEATURE_FUNC( slPCLSetMarker, sl::kFeaturePCL );

	Tr2AdapterInfo videoAdapterInfo;
	Tr2VideoAdapterInfo::GetAdapterInfo( adapter, videoAdapterInfo );

	sl::AdapterInfo adapterInfo{};
	adapterInfo.deviceLUID = videoAdapterInfo.luid;
	adapterInfo.deviceLUIDSizeInBytes = sizeof( LUID );


	std::vector<sl::Feature> dlssFeatures = {
		sl::kFeatureDLSS,
		sl::kFeatureNIS
	};
	m_isAvailable = true;

	for( auto& feature : dlssFeatures )
	{
		m_isAvailable &= Tr2StreamlineAL::CheckForAvailability( m_streamlineModule, feature, adapterInfo ) == sl::Result::eOk;
	}

	std::vector<sl::Feature> dlssgFeatures = {
		sl::kFeatureDLSS_G,
		sl::kFeatureReflex
	};
	m_supportsFrameGeneration = true;

	for( auto& feature : dlssgFeatures )
	{
		m_supportsFrameGeneration &= Tr2StreamlineAL::CheckForAvailability( m_streamlineModule, feature, adapterInfo ) == sl::Result::eOk;
	}

	m_streamlineSetup = true;
	SanitizeState();
}

Tr2DlssUpscalingTechnique::~Tr2DlssUpscalingTechnique()
{
	TogglePlugin( sl::kFeatureDLSS, false );
	TogglePlugin( sl::kFeatureDLSS_G, false );
	TogglePlugin( sl::kFeatureNIS, false );

	TogglePlugin( sl::kFeaturePCL, false );
	if( g_upscalingDebug )
	{
		TogglePlugin( sl::kFeatureImGUI, false );
	}

	if( m_attachedToDevice )
	{
		auto reflexConst = sl::ReflexOptions{};
		reflexConst.mode = sl::ReflexMode::eOff;
		if( SL_FAILED( result, m_slReflexSetOptions( reflexConst ) ) )
		{
			CCP_LOGERR( "Reflex failed to set options (%d)", result );
		}

		TogglePlugin( sl::kFeatureReflex, false );

		m_attachedToDevice = false;
	}

	m_renderContext.FlushAndSyncDx12();

	Tr2StreamlineAL::ReleaseStreamline( m_streamlineModule );
}

bool Tr2DlssUpscalingTechnique::IsAvailable( ) const 
{
	return m_isAvailable && m_streamlineSetup;
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

void Tr2DlssUpscalingTechnique::TogglePlugin( sl::Feature feature, bool enable )
{
	if( !m_attachedToDevice )
	{
		return;
	}
	if( SL_FAILED( res, m_slSetFeatureLoaded( feature, enable ) ) )
	{
		CCP_LOGERR( "Trying to %s Nvidia Streamline plugin '%s' but it failed (%d)", enable ? "enable" : "disable", 
			Tr2StreamlineAL::GetPluginName(feature), res );
	}
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
	DECLARE_SL_FUNC( m_streamlineModule, slSetD3DDevice );

	if( SL_FAILED( res, m_slSetD3DDevice( nativeDevice ) ) )
	{
		CCP_LOGWARN( "Could not attach NVidia Streamline to device (%d)", res );
		return nativeDevice;
	}
	CComPtr<ID3D12Device> proxy = nativeDevice;
	if( SL_FAILED( res, m_slUpgradeInterface( (void**)&proxy.p ) ) )
	{
		CCP_LOGWARN( "Could not upgrade device to sl proxy device (%d)", res );
		return nativeDevice;
	}
	
	m_attachedToDevice = true;
	TogglePlugin( sl::kFeatureDLSS, m_isAvailable );
	TogglePlugin( sl::kFeatureDLSS_G, m_supportsFrameGeneration && m_frameGeneration );
	TogglePlugin( sl::kFeatureNIS, m_isAvailable );
	TogglePlugin( sl::kFeatureReflex, m_supportsFrameGeneration && m_frameGeneration );
	TogglePlugin( sl::kFeaturePCL, m_frameGeneration );

	if( g_upscalingDebug )
	{
		TogglePlugin( sl::kFeatureImGUI, m_isAvailable );
	}

	auto reflexConst = sl::ReflexOptions{};
	reflexConst.mode = m_frameGeneration ? sl::ReflexMode::eLowLatency : sl::ReflexMode::eOff;
	reflexConst.useMarkersToOptimize = true;
	reflexConst.virtualKey = VK_F13;
	reflexConst.frameLimitUs = 0;

	if( SL_FAILED( result, m_slReflexSetOptions( reflexConst ) ) )
	{
		CCP_LOGERR( "Reflex failed to set options (%d)", result );
	}

	return proxy;
}

CComPtr<ID3D12CommandQueue> Tr2DlssUpscalingTechnique::ReplaceCommandQueue( CComPtr<ID3D12CommandQueue>& native )
{
	CComPtr<ID3D12CommandQueue> proxy = native;

	if( SL_FAILED( res, m_slUpgradeInterface( (void**)&proxy.p ) ) )
	{
		CCP_LOGWARN( "Could not upgrade device to sl proxy device (%d)", res );
		return native;
	}
	return proxy;
}

CComPtr<IDXGIFactory4> Tr2DlssUpscalingTechnique::ReplaceFactory( CComPtr<IDXGIFactory4>& native )
{
	CComPtr<IDXGIFactory4> proxy = native;

	if( SL_FAILED( res, m_slUpgradeInterface( (void**)&proxy.p ) ) )
	{
		CCP_LOGWARN( "Could not upgrade device to sl proxy device (%d)", res );
		return native;
	}
	return proxy;
}


void Tr2DlssUpscalingTechnique::MarkFrameEvent( Tr2RenderContextEnum::FrameEvent& frameEvent )
{
	Tr2UpscalingTechniqueDx12::MarkFrameEvent( frameEvent );
	
	sl::PCLMarker slEvent = (sl::PCLMarker)0;

	switch( frameEvent )
	{
	case Tr2RenderContextEnum::FRAME_EVENT_PRESENT_STARTED:
		slEvent = sl::PCLMarker::ePresentStart;
		break;
	case Tr2RenderContextEnum::FRAME_EVENT_PRESENT_FINISHED:
		slEvent = sl::PCLMarker::ePresentEnd;
		break;
	case Tr2RenderContextEnum::FRAME_EVENT_UPDATE_STARTED:
		slEvent = sl::PCLMarker::eSimulationStart;
		break;
	case Tr2RenderContextEnum::FRAME_EVENT_UPDATE_FINISHED:
		slEvent = sl::PCLMarker::eSimulationEnd;
		break;
	case Tr2RenderContextEnum::FRAME_EVENT_RENDERING_STARTED:
		slEvent = sl::PCLMarker::eRenderSubmitStart;
		if( SL_FAILED( res, m_slGetNewFrameToken( m_frameToken, nullptr ) ) )
		{
			CCP_LOGERR( "Could not get new frame token for Nvidia Streamline (%d)", res );
		}
		for( auto& context : m_contexts )
		{
			auto dlssContext = (Tr2DlssUpscalingContext*)( context.second.get() );
			dlssContext->SetFrameToken( m_frameToken );
		}
		break;
	case Tr2RenderContextEnum::FRAME_EVENT_RENDERING_FINISHED:
		slEvent = sl::PCLMarker::eRenderSubmitEnd;
		break;
	}
	if( m_frameToken )
	{
		if( SL_FAILED( result, m_slPCLSetMarker( slEvent, *m_frameToken ) ) )
		{
			CCP_LOGERR( "Reflex failed to set marker %d (%d)", slEvent, result );
		}
	}
}

Tr2UpscalingContextAL* Tr2DlssUpscalingTechnique::CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params )
{
	return new Tr2DlssUpscalingContext( m_setting, m_frameGeneration, params, ++m_contextIndex, m_streamlineModule, m_frameToken );
}

Tr2DlssUpscalingContext::Tr2DlssUpscalingContext( 
	Tr2UpscalingAL::Setting setting, 
	bool frameGeneration,
	Tr2UpscalingAL::UpscalingContextParams params,
	uint32_t contextNumber, 
	HMODULE streamlineModule, 
	sl::FrameToken* frameToken ) :
	Tr2UpscalingContextAL( setting, frameGeneration, params ),
	m_dlssMode( sl::DLSSMode::eOff ),
	m_viewHandle( sl::ViewportHandle( contextNumber ) ),
	m_streamlineModule( streamlineModule ),
	m_frameToken( frameToken ),
	m_setup( false ),
	m_vramUsage( 0 ),
	m_minWidthHeight( 0 ),
	m_actualFrames( 0 ),
	m_slDLSSGetOptimalSettings( nullptr ),
	m_slDLSSSetOptions( nullptr ),
	m_slDLSSGSetOptions( nullptr ),
	m_slSetConstants( nullptr ),
	m_slEvaluateFeature( nullptr ),
	m_slFreeResources( nullptr ),
	m_slDLSSGGetState( nullptr ),
	m_slNISSetOptions( nullptr ),
	m_slSetTag( nullptr )
{
	DECLARE_SL_FUNC( m_streamlineModule, slGetFeatureFunction );
	INITIALIZE_SL_FEATURE_FUNC( slNISSetOptions, sl::kFeatureNIS );

	INITIALIZE_SL_FEATURE_FUNC( slDLSSGetOptimalSettings, sl::kFeatureDLSS );
	INITIALIZE_SL_FEATURE_FUNC( slDLSSSetOptions, sl::kFeatureDLSS );
	if( frameGeneration )
	{
		INITIALIZE_SL_FEATURE_FUNC( slDLSSGGetState, sl::kFeatureDLSS_G );
		INITIALIZE_SL_FEATURE_FUNC( slDLSSGSetOptions, sl::kFeatureDLSS_G );
	}

	INITIALIZE_SL_FUNC( streamlineModule, slSetTag );
	INITIALIZE_SL_FUNC( streamlineModule, slSetConstants );
	INITIALIZE_SL_FUNC( streamlineModule, slEvaluateFeature );
	INITIALIZE_SL_FUNC( streamlineModule, slFreeResources );
	INITIALIZE_SL_FUNC( streamlineModule, slAllocateResources );

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

	if( SL_FAILED( result, m_slDLSSGetOptimalSettings( m_dlssOptions, m_optimalSettings ) ) )
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

		if( SL_FAILED( res, m_slDLSSGSetOptions( m_viewHandle, m_dlssgOptions ) ) )
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

	if( SL_FAILED( res, m_slDLSSSetOptions( m_viewHandle, m_dlssOptions ) ) )
	{
		CCP_LOGERR( "Setting DLSS Options failed with (%d)", res );
		return;
	}

	m_nisOptions.hdrMode = sl::NISHDR::eLinear;
	m_nisOptions.mode = sl::NISMode::eScaler;
	m_nisOptions.sharpness = 0.1f;

	if( SL_FAILED( res, m_slNISSetOptions( m_viewHandle, m_nisOptions ) ) )
	{
		CCP_LOGERR( "Setting NIS Options failed with (%d)", res );
		return;
	}

	auto dims = Tr2BitmapDimensions( m_displayWidth, m_displayHeight, 1, m_params.sourceFormat );
	m_dlssOutput.Create( dims, Tr2GpuUsage::UNORDERED_ACCESS, m_params.renderContext.GetPrimaryRenderContext() );

	CCP_LOGNOTICE( "DLSS Options set successfully" );
	m_setup = true;
}

Tr2DlssUpscalingContext::~Tr2DlssUpscalingContext()
{
	m_params.renderContext.FlushAndSyncDx12();

	m_slFreeResources( sl::kFeatureDLSS, m_viewHandle );
	m_slFreeResources( sl::kFeatureNIS, m_viewHandle );

	if( m_frameGeneration )
	{
		m_slFreeResources( sl::kFeatureDLSS_G, m_viewHandle );
	}
}

sl::Result Tr2DlssUpscalingContext::UpdateDlssG()
{
	if( SL_FAILED( res, m_slDLSSGGetState( m_viewHandle, m_dlssgState, &m_dlssgOptions ) ) )
	{
		CCP_LOGERR( "Getting DLSSG State failed with (%d) - disabling framegeneration", res );
		m_frameGeneration = false;
		switch( m_dlssgState.status )
		{
		case sl::DLSSGStatus::eFailResolutionTooLow:
			CCP_LOGERR( "DLSS - Framegeneration disabled because the resolution is too small (%d, %d). Minimum resolution is %d", m_dlssOptions.outputWidth, m_dlssOptions.outputHeight, m_dlssgState.minWidthOrHeight );
			break;
		case sl::DLSSGStatus::eFailReflexNotDetectedAtRuntime:
			CCP_LOGERR( "DLSS - Framegeneration disabled because Nvidia Reflex was not detected" );
			break;
		case sl::DLSSGStatus::eFailHDRFormatNotSupported:
			CCP_LOGERR( "DLSS - Framegeneration disabled because HDR format is not supported" );
			break;
		case sl::DLSSGStatus::eFailCommonConstantsInvalid:
			CCP_LOGERR( "DLSS - Framegeneration disabled because some common constants are invalid" );
			break;
		case sl::DLSSGStatus::eFailGetCurrentBackBufferIndexNotCalled:
			CCP_LOGERR( "DLSS - Framegeneration disabled because D3D Integration is not using SwapChain::GetCurrentBackBufferIndex API" );
			break;
		default:
			break;
		}
		return res;
	}
	return sl::Result::eOk;
}

bool Tr2DlssUpscalingContext::HasSharpening() const
{
	return true;
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

sl::Result Tr2DlssUpscalingContext::ReadyDLSSResources( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	auto& renderContext = m_params.renderContext;

	auto inputResource = DlssUtils::GenerateTextureResource( dispatchParameters.input );
	auto outputResource = DlssUtils::GenerateTextureResource( &m_dlssOutput );
	auto depthResource = DlssUtils::GenerateTextureResource( dispatchParameters.depth );
	auto velocityResource = DlssUtils::GenerateTextureResource( dispatchParameters.velocity );
	auto exposureResource = DlssUtils::GenerateTextureResource( dispatchParameters.exposure );
	auto opaqueOnlyResource = DlssUtils::GenerateTextureResource( dispatchParameters.opaqueOnly );

	sl::ResourceTag opaqueColorInTag = sl::ResourceTag{ &opaqueOnlyResource, sl::kBufferTypeOpaqueColor, sl::ResourceLifecycle::eValidUntilPresent, nullptr };
	sl::ResourceTag colorInTag = sl::ResourceTag{ &inputResource, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, nullptr };
	sl::ResourceTag colorOutTag = sl::ResourceTag{ &outputResource, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, nullptr };
	sl::ResourceTag depthTag = sl::ResourceTag{ &depthResource, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, nullptr };
	sl::ResourceTag mvecTag = sl::ResourceTag{ &velocityResource, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, nullptr };
	sl::ResourceTag exposureTag = sl::ResourceTag{ &exposureResource, sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilPresent, nullptr };
	
	sl::ResourceTag resources[] = { colorInTag, opaqueColorInTag, colorOutTag, depthTag, mvecTag, exposureTag };

	if( SL_FAILED( res, m_slSetTag( m_viewHandle, resources, 6, renderContext.m_commandList ) ) )
	{
		CCP_LOGERR( "DLSS Failed to tag resources (%d)", res );
		return res;
	}
	return sl::Result::eOk;
}

sl::Result Tr2DlssUpscalingContext::ReadyNISResources( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	auto& renderContext = m_params.renderContext;
	auto inputResource = DlssUtils::GenerateTextureResource( &m_dlssOutput );
	auto outputResource = DlssUtils::GenerateTextureResource( dispatchParameters.output );

	sl::ResourceTag colorInTag = sl::ResourceTag{ &inputResource, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, nullptr };
	sl::ResourceTag colorOutTag = sl::ResourceTag{ &outputResource, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent,  };

	sl::ResourceTag resources[] = { colorInTag, colorOutTag };

	if( SL_FAILED( res, m_slSetTag( m_viewHandle, resources, 2, renderContext.m_commandList ) ) )
	{
		CCP_LOGERR( "NIS Failed to tag resources (%d)", res );
		return res;
	}
	return sl::Result::eOk;
}

void Tr2DlssUpscalingContext::SetCommonConstants( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	m_commonConstants = sl::Constants{};
	m_commonConstants.cameraAspectRatio = dispatchParameters.aspectRatio;
	m_commonConstants.cameraFar = dispatchParameters.backClip;
	m_commonConstants.cameraFOV = dispatchParameters.fieldOfView;
	m_commonConstants.cameraMotionIncluded = sl::eTrue;
	m_commonConstants.cameraNear = dispatchParameters.frontClip;

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
	m_commonConstants.reset = m_reset ? sl::eTrue : sl::eFalse;


	if( SL_FAILED( result, m_slSetConstants( m_commonConstants, *m_frameToken, m_viewHandle ) ) )
	{
		CCP_LOGERR( "Setting Nvidia Streamline common constants failed (%d)", result );
	}
	m_reset = false;
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

	const sl::BaseStructure* handle[] = { &m_viewHandle };

	auto commandBuffer = ( sl::CommandBuffer* ) renderContext.m_commandList;
	if( m_frameGeneration )
	{
		m_slAllocateResources( commandBuffer, sl::kFeatureDLSS_G, m_viewHandle );
		UpdateDlssG();
	}
	m_slAllocateResources( commandBuffer, sl::kFeatureDLSS, m_viewHandle );

	if( SL_FAILED( res, ReadyDLSSResources( dispatchParameters ) ) )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}
	if( SL_FAILED( result, m_slEvaluateFeature( sl::kFeatureDLSS, *m_frameToken, handle, _countof( handle ), commandBuffer ) ) )
	{
		CCP_LOGERR( "Failed to Dispatch DLSS (%d)", result );
	}

	
	if( SL_FAILED( res, ReadyNISResources( dispatchParameters ) ) )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}
	if( SL_FAILED( result, m_slEvaluateFeature( sl::kFeatureNIS, *m_frameToken, handle, _countof( handle ), commandBuffer ) ) )
	{
		CCP_LOGERR( "Failed to Dispatch NIS (%d)", result );
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
	if( texture )
	{
		auto resource = DlssUtils::GenerateTextureResource( texture );
		sl::ResourceTag tag = sl::ResourceTag{ &resource, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, nullptr };

		if( SL_FAILED( res, m_slSetTag( m_viewHandle, &tag, 1, m_params.renderContext.m_commandList ) ) )
		{
			CCP_LOGERR( "Failed to tag HudLess texture (%d)", res );
		}
	}
}



#endif
