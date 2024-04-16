////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

#include "Tr2DlssUpscaling.h"
#include "../Tr2AdapterStructures.h"
#include "../Tr2VideoAdapterInfoALDx11.h" 
#include "../Tr2RenderContextDx11.h"
#include "../Tr2TextureALDx11.h"

#include <filesystem>
#include <sl_security.h>


#define DECLARE_STATIC_SL_FUNC( func ) \
	static PFun_##func* s_##func = reinterpret_cast<PFun_##func*>( GetProcAddress( m_streamlineModule, #func ) )

#define DECLARE_STATIC_FEATURE_FUNC( func, feature )                                   \
	static PFun_##func* s_##func{};                                                    \
	if( !s_##func )                                                                    \
	{                                                                                  \
		DECLARE_STATIC_SL_FUNC( slGetFeatureFunction );                                \
		sl::Result res = s_slGetFeatureFunction( feature, #func, (void*&)s_##func );   \
		if( res != sl::Result::eOk )                                                   \
			CCP_LOGERR( "Unable to find function %s for feature %s", #func, feature ); \
	}


namespace DlssUtils
{
void Log( sl::LogType type, const char* msg )
{
	switch( type )
	{
	case sl::LogType::eInfo:
		CCP_LOGNOTICE( msg );
		break;

	case sl::LogType::eWarn:
		CCP_LOGWARN( msg );
		break;

	case sl::LogType::eError:
		CCP_LOGERR( msg );
		break;
	default:
		break;
	}
}

const char* GetPluginName( sl::Feature feature )
{
	switch( feature )
	{
	case sl::kFeatureDLSS:
		return "DLSS";
	case sl::kFeatureDLSS_G:
		return "DLSSG";
	case sl::kFeatureNIS:
		return "NIS";
	case sl::kFeatureImGUI:
		return "ImGUI";
	case sl::kFeatureReflex:
		return "Reflex";
	default:
		return "N/A";
	}
}

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

sl::float4x4 AsFloat4x4( float f[16] )
{
	sl::float4x4 m;
	m.setRow( 0, sl::float4( f[0], f[1], f[2], f[3] ) );
	m.setRow( 1, sl::float4( f[4], f[5], f[6], f[7] ) );
	m.setRow( 2, sl::float4( f[8], f[9], f[10], f[11] ) );
	m.setRow( 3, sl::float4( f[12], f[13], f[14], f[15] ) );
	return m;
}
}

Tr2DlssUpscalingTechnique::Tr2DlssUpscalingTechnique( Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) :
	Tr2UpscalingTechniqueDx11( setting, frameGeneration, adapter ),
	m_contextIndex(0)
{
}

Tr2DlssUpscalingTechnique::~Tr2DlssUpscalingTechnique()
{
}

bool Tr2DlssUpscalingTechnique::InitializeStreamline()
{
	// first find the dll
	wchar_t abs_path[2048];
	auto size = SearchPathW( nullptr, L"sl.interposer.dll", L"", 2048, abs_path, nullptr );
	bool available = false;
	if( size == 0 )
	{
		CCP_LOGERR( "Unable to find sl.interposer.dll in path for secure load." );
	}
	else
	{
		available = sl::security::verifyEmbeddedSignature( abs_path );
		m_streamlineModule = LoadLibraryW( abs_path );
		available = !!m_streamlineModule;
	}
	CCP_LOGNOTICE( "Nvidia Streamline is %s", available ? "enabled" : "disabled" );

	if( !available )
	{
		return false;
	}

	// now set it up
	sl::Preferences pref{};

#ifndef NDEBUG
	pref.showConsole = true; // for debugging, set to false in production
	pref.logLevel = sl::LogLevel::eVerbose;
#else
	pref.showConsole = false;
	pref.logLevel = sl::LogLevel::eOff;
#endif
	std::vector<sl::Feature> features = {
		sl::kFeatureDLSS,
		sl::kFeatureNIS
	};

#ifndef NDEBUG
	features.push_back( sl::kFeatureImGUI );
#endif

	pref.renderAPI = sl::RenderAPI::eD3D11;

	sl::Feature* featuresToEnable = features.data();
	pref.numFeaturesToLoad = (uint32_t)features.size();
	pref.featuresToLoad = featuresToEnable;
	pref.logMessageCallback = DlssUtils::Log;

	// the ID of Eve Online
	const uint32_t EVE_ONLINE_APP_ID = 101109911;

	pref.applicationId = EVE_ONLINE_APP_ID;
	pref.engine = sl::EngineType::eCustom;
	pref.flags |= sl::PreferenceFlags::eAllowOTA;
	pref.flags |= sl::PreferenceFlags::eUseManualHooking;

	DECLARE_STATIC_SL_FUNC( slInit );
	if( SL_FAILED( res, s_slInit( pref, sl::kSDKVersion ) ) )
	{
		if( res == sl::Result::eErrorDriverOutOfDate )
		{
			CCP_LOGWARN( "Could not initialize NVidia Streamline due to drivers being out of date" );
		}

		return false;
	}

	CCP_LOGNOTICE( "NVidia Streamline successfully initialized" );
	return true;
}

bool Tr2DlssUpscalingTechnique::IsPluginAvailable( sl::Feature feature, uint32_t adapter )
{
	Tr2AdapterInfo videoAdapterInfo = {};
	Tr2VideoAdapterInfo::GetAdapterInfo( adapter, videoAdapterInfo );

	sl::AdapterInfo adapterInfo{};
	adapterInfo.deviceLUID = videoAdapterInfo.luid;
	adapterInfo.deviceLUIDSizeInBytes = sizeof( LUID );

	DECLARE_STATIC_SL_FUNC( slIsFeatureSupported );
	auto result = s_slIsFeatureSupported( feature, adapterInfo );
	if( result != sl::Result::eOk )
	{
		auto pluginName = DlssUtils::GetPluginName( feature );
		switch( result )
		{
		case sl::Result::eErrorOSOutOfDate: // inform user to update OS
			CCP_LOGWARN( "OS is out of date, please update OS to use %s", pluginName );
			break;
		case sl::Result::eErrorDriverOutOfDate: // inform user to update driver
			CCP_LOGWARN( "Driver is out of date, please update driver to use %s", pluginName );
			break;
		case sl::Result::eErrorAdapterNotSupported:
			CCP_LOGWARN( "No adapter found that supports %s", pluginName );
			break;
		case sl::Result::eErrorMissingOrInvalidAPI:
			CCP_LOGWARN( "Graphics api not supported for %s", pluginName );
			break;
		default:
			CCP_LOGWARN( "NVidia Streamline plugin '%s' is not supported", pluginName );
			CCP_LOGWARN( "Streamline error %d", result );
		};
		return false;
	}

	CCP_LOGNOTICE( "NVidia Streamline plugin '%s' is available", DlssUtils::GetPluginName( feature ) );
	return true;
}

bool Tr2DlssUpscalingTechnique::TogglePlugin( sl::Feature feature, bool enable )
{
	DECLARE_STATIC_SL_FUNC( slSetFeatureLoaded );
	if( SL_FAILED( res, s_slSetFeatureLoaded( feature, enable ) ) )
	{
		CCP_LOGERR( "Trying to %s Nvidia Streamline plugin '%s' but it failed (%d)", enable ? "enable" : "disable", DlssUtils::GetPluginName( feature ), res );
		return false;
	}
	return false;
}


void Tr2DlssUpscalingTechnique::MarkFrameEvent( Tr2RenderContextEnum::FrameEvent& frameEvent )
{
	Tr2UpscalingTechniqueDx11::MarkFrameEvent( frameEvent );
	if( frameEvent == Tr2RenderContextEnum::FRAME_EVENT_RENDERING_STARTED )
	{
		DECLARE_STATIC_SL_FUNC( slGetNewFrameToken );
		if( SL_FAILED( res, s_slGetNewFrameToken( m_frameToken, nullptr ) ) )
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
	if( !InitializeStreamline() )
	{
		return Tr2UpscalingAL::Result::TECHNIQUE_NOT_SUPPORTED;
	}
	return Tr2UpscalingAL::Result::OK;
}

void Tr2DlssUpscalingTechnique::Destroy( Tr2RenderContextAL& renderContext )
{
	m_contexts.clear();

	TogglePlugin( sl::kFeatureDLSS, false );
	TogglePlugin( sl::kFeatureNIS, false );
}

void Tr2DlssUpscalingTechnique::AttachToDevice(CComPtr<ID3D11Device>& device) 
{
	DECLARE_STATIC_SL_FUNC( slSetD3DDevice );

	if( SL_FAILED( res, s_slSetD3DDevice( device ) ) )
	{
		CCP_LOGWARN( "Could not attach NVidia Streamline to device (%d)", res );
		return;
	}
	TogglePlugin( sl::kFeatureDLSS, true );
	TogglePlugin( sl::kFeatureNIS, true );
	CCP_LOGNOTICE( "NVidia Streamline successfully attached to device and adapter" );
}

Tr2UpscalingContext* Tr2DlssUpscalingTechnique::CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight )
{
	return new Tr2DlssUpscalingContext( displayWidth, displayHeight, m_setting, m_frameGeneration, m_contextIndex++, m_streamlineModule, m_frameToken );
}

Tr2DlssUpscalingContext::Tr2DlssUpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t contextNumber, HMODULE streamlineModule, sl::FrameToken* frameToken ) :
	Tr2UpscalingContext( displayWidth, displayHeight, setting, frameGeneration ),
	m_viewHandle( sl::ViewportHandle( contextNumber ) ),
	m_streamlineModule( streamlineModule ),
	m_frameToken( frameToken )
{
}

Tr2DlssUpscalingContext::~Tr2DlssUpscalingContext()
{
	DECLARE_STATIC_SL_FUNC( slFreeResources );
	s_slFreeResources( sl::kFeatureDLSS, m_viewHandle );
	s_slFreeResources( sl::kFeatureNIS, m_viewHandle );
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

	DECLARE_STATIC_FEATURE_FUNC( slDLSSGetOptimalSettings, sl::kFeatureDLSS )

	if( SL_FAILED( result, s_slDLSSGetOptimalSettings( m_options, m_optimalSettings ) ) )
	{
		CCP_LOGERR( "Getting Optimal Settings for DLSS failed (%d)", result );
		return Tr2UpscalingAL::Result::INCORRECT_INPUT;
	}
	m_renderWidth = m_optimalSettings.optimalRenderWidth;
	m_renderHeight = m_optimalSettings.optimalRenderHeight;

	m_upscaling = (float)m_options.outputHeight / (float)m_renderHeight;

	m_jitterSequence = Tr2UpscalingAL::GenerateHaltonSequence( 8 * (uint32_t)powf( m_upscaling, 2.0f ), 2, 3 );

	m_options.colorBuffersHDR = sl::eTrue;
	m_options.mode = m_dlssMode;
	m_options.useAutoExposure = sl::eFalse;

	DECLARE_STATIC_FEATURE_FUNC( slDLSSSetOptions, sl::kFeatureDLSS )
	if( SL_FAILED( res, s_slDLSSSetOptions( m_viewHandle, m_options ) ) )
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
	m_jitterY = -m_jitterSequence[m_jitterIndex].second;

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
	DECLARE_STATIC_SL_FUNC( slSetTag );
	if( SL_FAILED( res, s_slSetTag( m_viewHandle, resources, 6, renderContext.m_context ) ) )
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
	m_commonConstants.cameraViewToClip = DlssUtils::AsFloat4x4( dispatchParameters.projection );
	m_commonConstants.clipToCameraView = DlssUtils::AsFloat4x4( dispatchParameters.invProjection );
	m_commonConstants.clipToPrevClip = DlssUtils::AsFloat4x4( dispatchParameters.clipToPrevClip );
	m_commonConstants.depthInverted = sl::eTrue;
	m_commonConstants.jitterOffset = sl::float2( m_jitterX, m_jitterY );
	m_commonConstants.motionVectors3D = sl::eFalse;
	m_commonConstants.motionVectorsDilated = sl::eFalse;
	m_commonConstants.motionVectorsJittered = sl::eFalse;
	m_commonConstants.mvecScale = sl::float2( 1, 1 );
	m_commonConstants.orthographicProjection = sl::eFalse;
	m_commonConstants.prevClipToClip = DlssUtils::AsFloat4x4( dispatchParameters.prevClipToClip );
	// unused things
	m_commonConstants.cameraPinholeOffset = sl::float2( 0, 0 );
	m_commonConstants.reset = m_reset ? sl::eTrue : sl::eFalse;

	DECLARE_STATIC_SL_FUNC( slSetConstants );
	if( SL_FAILED( result, s_slSetConstants( m_commonConstants, *m_frameToken, m_viewHandle ) ) )
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

	DECLARE_STATIC_SL_FUNC( slEvaluateFeature );

	if( SL_FAILED( result, s_slEvaluateFeature( sl::kFeatureDLSS, *m_frameToken, handle, _countof( handle ), renderContext.m_context ) ) )
	{
		CCP_LOGERR( "DLSS Failed to Dispatch (%d)", result );
	}

	return Tr2UpscalingAL::OK;
}


#endif