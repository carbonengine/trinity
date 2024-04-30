////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

#include "Tr2DlssUpscaling.h"
#include "include/Tr2StreamlineAL.h"

#include "../Tr2AdapterStructures.h"
#include "../Tr2VideoAdapterInfoALDx11.h" 
#include "../Tr2RenderContextDx11.h"
#include "../Tr2TextureALDx11.h"

namespace DlssUtils
{
	sl::Resource GenerateTextureResource( Tr2TextureAL* texture )
	{
		if( texture && texture->IsValid() )
		{
			return { sl::ResourceType::eTex2d, texture->TrinityALImpl_GetObject()->GetResourceDx11(), nullptr, nullptr, 0 };
		}
		else
		{
			return { sl::ResourceType::eTex2d, nullptr, nullptr, nullptr, 0 };
		}
	}
}

Tr2DlssUpscalingTechnique::Tr2DlssUpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	Tr2UpscalingTechniqueDx11( technique, setting, frameGeneration, adapter ),
	m_contextIndex(0),
	m_attachedToDevice( false )
{
	m_streamlineSetup = false;
	m_streamlineModule = Tr2StreamlineAL::GetStreamlineModule();

	if( !m_streamlineModule )
	{
		CCP_LOGERR( "Could not find the streamline module. Streamline will not be initialized" );

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

	Tr2AdapterInfo videoAdapterInfo;
	Tr2VideoAdapterInfo::GetAdapterInfo( adapter, videoAdapterInfo );

	sl::AdapterInfo adapterInfo{};
	adapterInfo.deviceLUID = videoAdapterInfo.luid;
	adapterInfo.deviceLUIDSizeInBytes = sizeof( LUID );

	m_isAvailable = true;

	std::vector<sl::Feature> features = {
		sl::kFeatureDLSS,
		sl::kFeatureNIS
	};
	for( auto& feature : features )
	{
		m_isAvailable &= Tr2StreamlineAL::CheckForAvailability( m_streamlineModule, feature, adapterInfo ) == sl::Result::eOk;
	}

	m_streamlineSetup = true;
	SanitizeState();
}

Tr2DlssUpscalingTechnique::~Tr2DlssUpscalingTechnique()
{
}

bool Tr2DlssUpscalingTechnique::IsAvailable( Tr2RenderContextAL& renderContext, uint32_t adapter ) const
{
	return m_isAvailable;
}

std::vector<Tr2UpscalingAL::Setting> Tr2DlssUpscalingTechnique::GetAvailableSettings() const
{
	return {
		Tr2UpscalingAL::QUALITY, 
		Tr2UpscalingAL::BALANCED, 
		Tr2UpscalingAL::PERFORMANCE,
		Tr2UpscalingAL::ULTRA_PERFORMANCE
	};
}

bool Tr2DlssUpscalingTechnique::TogglePlugin( sl::Feature feature, bool enable )
{
	if( SL_FAILED( res, m_slSetFeatureLoaded( feature, enable ) ) )
	{
		CCP_LOGERR( "Trying to %s Nvidia Streamline plugin '%s' but it failed (%d)", enable ? "enable" : "disable", Tr2StreamlineAL::GetPluginName( feature ), res );
		return false;
	}
	return false;
}

void Tr2DlssUpscalingTechnique::MarkFrameEvent( Tr2RenderContextAL& renderContext, Tr2RenderContextEnum::FrameEvent& frameEvent )
{
	Tr2UpscalingTechniqueDx11::MarkFrameEvent( renderContext, frameEvent );
	if( frameEvent == Tr2RenderContextEnum::FRAME_EVENT_RENDERING_STARTED )
	{
		if( SL_FAILED( res, m_slGetNewFrameToken( m_frameToken, nullptr ) ) )
		{
			CCP_LOGERR( "Could not get new frame token for Nvidia Streamline (%d)", res );
			return;
		}
		for( auto& context : m_contexts )
		{
			( (Tr2DlssUpscalingContext*)( context.second.get() ) )->SetFrameToken( m_frameToken );
		}
	}
}

Tr2UpscalingAL::Result Tr2DlssUpscalingTechnique::Setup()
{
	if( !m_isAvailable || !m_streamlineSetup )
	{
		return Tr2UpscalingAL::Result::TECHNIQUE_NOT_SUPPORTED;
	}
	return Tr2UpscalingAL::Result::OK;
}

void Tr2DlssUpscalingTechnique::Destroy( Tr2RenderContextAL& renderContext )
{
	if( m_attachedToDevice )
	{
		TogglePlugin( sl::kFeatureDLSS, false );
		TogglePlugin( sl::kFeatureNIS, false );
	}
}

void Tr2DlssUpscalingTechnique::AttachToDevice(CComPtr<ID3D11Device>& device) 
{
	if( m_attachedToDevice )
	{
		TogglePlugin( sl::kFeatureDLSS, false );
		TogglePlugin( sl::kFeatureNIS, false );
	}
	DECLARE_SL_FUNC( m_streamlineModule, slSetD3DDevice );
	if( SL_FAILED( res, m_slSetD3DDevice( device ) ) )
	{
		CCP_LOGWARN( "Could not attach NVidia Streamline to device. Error code %d", res );
		return;
	}
	m_attachedToDevice = true;
	TogglePlugin( sl::kFeatureDLSS, true );
	TogglePlugin( sl::kFeatureNIS, true );
	CCP_LOGNOTICE( "NVidia Streamline successfully attached to device and adapter" );
}

Tr2UpscalingContextAL* Tr2DlssUpscalingTechnique::CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat )
{
	return new Tr2DlssUpscalingContext( displayWidth, displayHeight, m_setting, m_frameGeneration, sourceFormat, depthFormat, m_contextIndex++, m_streamlineModule, m_frameToken );
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
	m_streamlineModule( streamlineModule ),
	m_frameToken( frameToken )
{
	DECLARE_SL_FUNC( m_streamlineModule, slGetFeatureFunction );

	INITIALIZE_SL_FEATURE_FUNC( slDLSSGetOptimalSettings, sl::kFeatureDLSS );
	INITIALIZE_SL_FEATURE_FUNC( slDLSSSetOptions, sl::kFeatureDLSS );

	INITIALIZE_SL_FUNC( streamlineModule, slSetTag );
	INITIALIZE_SL_FUNC( streamlineModule, slSetConstants );
	INITIALIZE_SL_FUNC( streamlineModule, slEvaluateFeature );
	INITIALIZE_SL_FUNC( streamlineModule, slFreeResources );
}

Tr2DlssUpscalingContext::~Tr2DlssUpscalingContext()
{
	DECLARE_SL_FUNC( m_streamlineModule, slFreeResources );
	m_slFreeResources( sl::kFeatureDLSS, m_viewHandle );
	m_slFreeResources( sl::kFeatureNIS, m_viewHandle );
}

Tr2UpscalingAL::Result Tr2DlssUpscalingContext::Setup( Tr2RenderContextAL& renderContext )
{
	m_dlssMode = sl::DLSSMode::eOff;
	switch( m_setting )
	{
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
	m_renderWidth = m_optimalSettings.optimalRenderWidth == 0 ? m_displayWidth : m_optimalSettings.optimalRenderWidth;
	m_renderHeight = m_optimalSettings.optimalRenderHeight == 0 ? m_displayHeight : m_optimalSettings.optimalRenderHeight;

	m_upscaling = (float)m_options.outputHeight / (float)m_renderHeight;

	m_jitterSequence = Tr2UpscalingAL::GenerateHaltonSequence( 8 * (uint32_t)powf( m_upscaling, 2.0f ), 2, 3 );

	m_options.colorBuffersHDR = sl::eTrue;
	m_options.mode = m_dlssMode;
	m_options.useAutoExposure = sl::eFalse;

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

void Tr2DlssUpscalingContext::SetFrameToken( sl::FrameToken* frameToken )
{
	m_frameToken = frameToken;
}

void Tr2DlssUpscalingContext::UpdateJitter()
{
	m_jitterX = m_jitterSequence[m_jitterIndex].first;
	m_jitterY = m_jitterSequence[m_jitterIndex].second;

	m_jitterIndex = ++m_jitterIndex % m_jitterSequence.size();
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
	
	if( SL_FAILED( res, m_slSetTag( m_viewHandle, resources, 6, renderContext.m_context ) ) )
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
	m_commonConstants.mvecScale = sl::float2( 1, 1 );
	m_commonConstants.orthographicProjection = sl::eFalse;
	m_commonConstants.prevClipToClip = Tr2StreamlineAL::F16AsFloat4x4( dispatchParameters.prevClipToClip );
	// unused things
	m_commonConstants.cameraPinholeOffset = sl::float2( 0, 0 );
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

	if( !AreDisplayParametersValid( dispatchParameters ) )
	{
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}

	if( SL_FAILED( res, ReadyResources( renderContext, dispatchParameters ) ) )
	{
		CCP_LOGERR( "DLSS Failed to tag resources (%d)", res );
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}

	SetCommonConstants( dispatchParameters );

	const sl::BaseStructure* handle[] = { &m_viewHandle };

	if( SL_FAILED( result, m_slEvaluateFeature( sl::kFeatureDLSS, *m_frameToken, handle, _countof( handle ), renderContext.m_context ) ) )
	{
		CCP_LOGERR( "DLSS Failed to Dispatch (%d)", result );
	}

	return Tr2UpscalingAL::OK;
}


#endif
