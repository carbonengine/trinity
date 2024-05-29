////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#define DLSS_DEBUG false

#include "Tr2DlssUpscaling.h"
#include "../Tr2TextureALDx12.h"
#include "../Tr2RenderContextDx12.h"
#include "../Tr2PrimaryRenderContextDx12.h"
#include "../Tr2VideoAdapterInfoALDx12.h"
#include "../Utilities.h"
#include "include/Tr2StreamlineAL.h"

namespace DlssUtils
{

	sl::Resource GenerateTextureResource( Tr2TextureAL* texture )
	{
		if( texture && texture->IsValid() )
		{
			return { sl::ResourceType::eTex2d, texture->TrinityALImpl_GetObject()->GetResourceDx12(), nullptr, nullptr, (uint32_t)texture->TrinityALImpl_GetObject()->GetResourceState() };
		}
		else
		{
			return { sl::ResourceType::eTex2d, nullptr, nullptr, nullptr, 0 };
		}
	}
}

Tr2DlssUpscalingTechnique::Tr2DlssUpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	TrinityALImpl::Tr2UpscalingTechniqueDx12( technique, setting, frameGeneration ),
	m_adapter( adapter ),
	m_contextIndex( 0 ), 
	m_frameToken( 0 ), 
	m_attachedToDevice( false ),
	m_slGetFeatureFunction( nullptr ),
	m_slReflexSetOptions( nullptr ),
	m_slSetFeatureLoaded( nullptr ),
	m_slPCLSetMarker( nullptr ),
	m_slGetNewFrameToken( nullptr ),
	m_supportsFrameGeneration( false )
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
	Tr2StreamlineAL::ReleaseStreamline( m_streamlineModule );
}

bool Tr2DlssUpscalingTechnique::IsAvailable( Tr2RenderContextAL& renderContext ) const 
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


void Tr2DlssUpscalingTechnique::MarkFrameEvent( Tr2RenderContextAL& renderContext, Tr2RenderContextEnum::FrameEvent& frameEvent )
{
	Tr2UpscalingTechniqueDx12::MarkFrameEvent( renderContext, frameEvent );
	
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

void Tr2DlssUpscalingTechnique::Destroy( Tr2RenderContextAL& renderContext )
{
	TogglePlugin( sl::kFeatureDLSS, false );
	TogglePlugin( sl::kFeatureDLSS_G, false );
	TogglePlugin( sl::kFeatureNIS, false );
#if DLSS_DEBUG
	TogglePlugin( sl::kFeatureImGUI, false );
#endif
	TogglePlugin( sl::kFeaturePCL, false );

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
}

Tr2UpscalingContextAL* Tr2DlssUpscalingTechnique::CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat )
{
	return new Tr2DlssUpscalingContext( displayWidth, displayHeight, m_setting, m_frameGeneration, sourceFormat, depthFormat, ++m_contextIndex, m_streamlineModule, m_frameToken );
}

Tr2DlssUpscalingContext::Tr2DlssUpscalingContext( 
	uint32_t displayWidth, 
	uint32_t displayHeight, 
	Tr2UpscalingAL::Setting setting, 
	bool frameGeneration,
	Tr2RenderContextEnum::PixelFormat sourceFormat,
	Tr2RenderContextEnum::DepthStencilFormat depthFormat, 
	uint32_t contextNumber, 
	HMODULE streamlineModule, 
	sl::FrameToken* frameToken ) :
	Tr2UpscalingContextAL( displayWidth, displayHeight, setting, frameGeneration, sourceFormat, depthFormat ),
	m_viewHandle( sl::ViewportHandle( contextNumber ) ),
	m_setup( false ),
	m_dlssMode( sl::DLSSMode::eOff ),
	m_options( {} ),
	m_dlssgOptions( {} ),
	m_optimalSettings( {} ),
	m_streamlineModule( streamlineModule ),
	m_frameToken( frameToken ),
	m_slDLSSGetOptimalSettings( nullptr ),
	m_slDLSSSetOptions( nullptr ),
	m_slDLSSGSetOptions( nullptr ),
	m_slSetConstants( nullptr ),
	m_slEvaluateFeature( nullptr ),
	m_slFreeResources( nullptr ),
	m_slDLSSGGetState( nullptr ),
	m_slSetTag( nullptr )
{
	DECLARE_SL_FUNC( m_streamlineModule, slGetFeatureFunction );

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
}

Tr2DlssUpscalingContext::~Tr2DlssUpscalingContext()
{

}

void Tr2DlssUpscalingContext::Destroy( Tr2RenderContextAL& renderContext ) {
	renderContext.FlushAndSyncDx12();

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
			CCP_LOGERR( "DLSS - Framegeneration disabled because the resolution is too small (%d, %d). Minimum resolution is %d", m_options.outputWidth, m_options.outputHeight, m_dlssgState.minWidthOrHeight );
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
		}
		return res;
	}
	return sl::Result::eOk;
}

Tr2UpscalingAL::Result Tr2DlssUpscalingContext::Setup( Tr2RenderContextAL& renderContext )
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

	m_options.outputWidth = m_displayWidth;
	m_options.outputHeight = m_displayHeight;
	m_options.mode = m_dlssMode;

	if( SL_FAILED( result, m_slDLSSGetOptimalSettings( m_options, m_optimalSettings ) ) )
	{
		CCP_LOGERR( "Getting Optimal Settings for DLSS failed (%d)", result );
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}
	m_renderWidth = m_optimalSettings.optimalRenderWidth;
	m_renderHeight = m_optimalSettings.optimalRenderHeight;

	if( m_renderWidth == 0 || m_renderHeight == 0 )
	{
		m_renderWidth = m_displayWidth;
		m_renderHeight = m_displayHeight;
	}

	m_upscaling = (float)m_options.outputHeight / (float)m_renderHeight;

	m_jitterSequence = Tr2UpscalingAL::GenerateHaltonSequence( 8 * (uint32_t)powf( m_upscaling, 2.0f ), 2, 3 );

	if( m_frameGeneration )
	{
		UpdateDlssG();
	}

	if( m_frameGeneration )
	{
		m_dlssgOptions.mode = sl::DLSSGMode::eOn;

		m_dlssgOptions.colorWidth = m_options.outputWidth;
		m_dlssgOptions.colorHeight = m_options.outputHeight;

		m_dlssgOptions.colorBufferFormat = m_sourceFormat;
		m_dlssgOptions.depthBufferFormat = m_depthFormat;
		m_dlssgOptions.mvecBufferFormat = Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_FLOAT;

		m_dlssgOptions.mvecDepthWidth = m_renderWidth;
		m_dlssgOptions.mvecDepthHeight = m_renderHeight;

		if( SL_FAILED( res, m_slDLSSGSetOptions( m_viewHandle, m_dlssgOptions ) ) )
		{
			CCP_LOGERR( "Setting DLSSG Options failed with (%d)", res );
			return Tr2UpscalingAL::Result::INCORRECT_INPUT;
		}
		else
		{
			CCP_LOGNOTICE( "DLSSG Options set successfully" );
		}
	}

	m_options.colorBuffersHDR = sl::eTrue;
	m_options.mode = m_dlssMode;
	m_options.useAutoExposure = sl::eFalse;
	m_options.alphaUpscalingEnabled = sl::eFalse;

	if( SL_FAILED( res, m_slDLSSSetOptions( m_viewHandle, m_options ) ) )
	{
		CCP_LOGERR( "Setting DLSS Options failed with (%d)", res );
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}
	else
	{
		CCP_LOGNOTICE( "DLSS Options set successfully" );
	}
	
	m_setup = true;
	return Tr2UpscalingAL::Result::OK;
}

bool Tr2DlssUpscalingContext::IsTemporal() const
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

sl::Result Tr2DlssUpscalingContext::ReadyResources( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	auto inputResource = DlssUtils::GenerateTextureResource( dispatchParameters.input );
	auto outputResource = DlssUtils::GenerateTextureResource( dispatchParameters.output );
	auto depthResource = DlssUtils::GenerateTextureResource( dispatchParameters.depth );
	auto velocityResource = DlssUtils::GenerateTextureResource( dispatchParameters.velocity );
	auto exposureResource = DlssUtils::GenerateTextureResource( dispatchParameters.exposure );
	auto opaqueOnlyResource = DlssUtils::GenerateTextureResource( dispatchParameters.opaqueOnly );

	sl::Extent renderExtent = {};
	renderExtent.height = m_renderHeight;
	renderExtent.width = m_renderWidth;

	sl::Extent displayExtent = {};
	displayExtent.height = m_displayHeight;
	displayExtent.width = m_displayWidth;

	sl::Extent exposureExtent = {};
	exposureExtent.height = 1;
	exposureExtent.width = 1;

	sl::ResourceTag opaqueColorInTag = sl::ResourceTag{ &opaqueOnlyResource, sl::kBufferTypeOpaqueColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag colorInTag = sl::ResourceTag{ &inputResource, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag colorOutTag = sl::ResourceTag{ &outputResource, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &displayExtent };
	sl::ResourceTag depthTag = sl::ResourceTag{ &depthResource, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag mvecTag = sl::ResourceTag{ &velocityResource, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag exposureTag = sl::ResourceTag{ &exposureResource, sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilPresent, &exposureExtent };

	sl::ResourceTag resources[] = { colorInTag, opaqueColorInTag, colorOutTag, depthTag, mvecTag, exposureTag };

	if( SL_FAILED( res, m_slSetTag( m_viewHandle, resources, 6, renderContext.m_commandList ) ) )
	{
		CCP_LOGERR( "DLSS Failed to tag resources (%d)", res );
		return res;
	}
	return sl::Result::eOk;
}

void Tr2DlssUpscalingContext::SetCommonConstants( Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	m_commonConstants.cameraAspectRatio = dispatchParameters.aspectRatio;
	m_commonConstants.cameraFar = dispatchParameters.backClip;
	m_commonConstants.cameraFOV = dispatchParameters.fieldOfView;
	m_commonConstants.cameraFwd = sl::float3( dispatchParameters.view[13], dispatchParameters.view[14], dispatchParameters.view[15] );
	m_commonConstants.cameraMotionIncluded = sl::eTrue;
	m_commonConstants.cameraNear = dispatchParameters.frontClip;
	m_commonConstants.cameraPos = sl::float3( dispatchParameters.view[2], dispatchParameters.view[6], dispatchParameters.view[10] );
	m_commonConstants.cameraRight = sl::float3( dispatchParameters.view[0], dispatchParameters.view[4], dispatchParameters.view[8] );
	m_commonConstants.cameraUp = sl::float3( dispatchParameters.view[1], dispatchParameters.view[5], dispatchParameters.view[9] );
	m_commonConstants.cameraViewToClip = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.projection );
	m_commonConstants.clipToCameraView = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.invProjection );
	m_commonConstants.clipToPrevClip = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.clipToPrevClip );
	m_commonConstants.depthInverted = sl::eTrue;
	m_commonConstants.jitterOffset = sl::float2( m_jitterX, m_jitterY );
	m_commonConstants.motionVectors3D = sl::eFalse;
	m_commonConstants.motionVectorsDilated = sl::eFalse;
	m_commonConstants.motionVectorsJittered = sl::eFalse;
	m_commonConstants.mvecScale = sl::float2( 1, -1 );
	m_commonConstants.orthographicProjection = sl::eFalse;
	m_commonConstants.prevClipToClip = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.prevClipToClip );
	m_commonConstants.reset = m_reset ? sl::eTrue : sl::eFalse;

	if( SL_FAILED( result, m_slSetConstants( m_commonConstants, *m_frameToken, m_viewHandle ) ) )
	{
		CCP_LOGERR( "Setting Nvidia Streamline common constants failed (%d)", result );
	}
	m_reset = false;
}

Tr2UpscalingAL::Result Tr2DlssUpscalingContext::Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters )
{
	if( !m_setup )
	{
		return Tr2UpscalingAL::Result::CONTEXT_SETUP_FAILED;
	}

	if( !AreDispatchParametersValid( dispatchParameters ) )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}

	renderContext.FlushBarriersDx12();
	if( SL_FAILED( res, ReadyResources( renderContext, dispatchParameters ) ) )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}

	SetCommonConstants( dispatchParameters );

	const sl::BaseStructure* handle[] = { &m_viewHandle };

	auto commandBuffer = ( sl::CommandBuffer* ) renderContext.m_commandList;
	if( m_frameGeneration )
	{
		m_slAllocateResources( commandBuffer, sl::kFeatureDLSS_G, m_viewHandle );
		UpdateDlssG();
	}
	m_slAllocateResources( commandBuffer, sl::kFeatureNIS, m_viewHandle );
	m_slAllocateResources( commandBuffer, sl::kFeatureDLSS, m_viewHandle );

	if( SL_FAILED( result, m_slEvaluateFeature( sl::kFeatureDLSS, *m_frameToken, handle, _countof( handle ), commandBuffer ) ) )
	{
		CCP_LOGERR( "DLSS Failed to Dispatch (%d)", result );
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


#endif
