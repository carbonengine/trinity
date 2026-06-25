// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX11
#include "Tr2StreamlineALDx11.h"


#include "Tr2TextureALDx11.h"
#include "Tr2BufferAlDx11.h"

#include <filesystem>
#include <sl_security.h>

const uint32_t EVE_ONLINE_APP_ID = 101109911;
extern bool g_skipNvidiaStreamline;

namespace StreamlineUtils
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

sl::CommandBuffer* GetCommandBuffer( Tr2RenderContextAL& renderContext )
{
	return renderContext.m_context;
}
}

namespace Tr2Streamline
{
// Static Streamline interface
// Responsible for checking if it is available, initializing the Streamline framework and keeping tabs on plugin availability
Status s_status = Status::UNCHECKED;
bool s_reset = false;
bool s_reflexEnabled = false;
bool s_framegenEnabled = false;
Tr2DlssPlugin* s_dlssPlugin = nullptr;
Tr2NisPlugin* s_nisPlugin = nullptr;
HMODULE s_streamlineModule;


sl::FrameToken* s_frameToken = 0;
sl::Constants s_commonConstants = {};

std::map<StreamlineUtils::StreamlinePlugin, StreamlineUtils::PluginInfo> s_pluginInfo = {
	{
		StreamlineUtils::SP_DLSS,
		{ "dlss", sl::kFeatureDLSS, StreamlineUtils::SLPA_UNKNOWN, false },
	},
	{
		StreamlineUtils::SP_DLSSG,
		{ "dlss_g", sl::kFeatureDLSS_G, StreamlineUtils::SLPA_UNKNOWN, false },
	},
	{
		StreamlineUtils::SP_REFLEX,
		{ "reflex", sl::kFeatureReflex, StreamlineUtils::SLPA_UNKNOWN, false },
	},
	{
		StreamlineUtils::SP_NIS,
		{ "nis", sl::kFeatureNIS, StreamlineUtils::SLPA_UNKNOWN, false },
	},
	{
		StreamlineUtils::SP_DEBUG,
		{ "imgui", sl::kFeatureImGUI, StreamlineUtils::SLPA_UNKNOWN, false },
	},
};

sl::float2 AsFloat2( float f[2] )
{
	return sl::float2( f[0], f[1] );
}

sl::float3 AsFloat3( float f[3] )
{
	return sl::float3( f[0], f[1], f[2] );
}

sl::float4 AsFloat4( float f[4] )
{
	return sl::float4( f[0], f[1], f[2], f[3] );
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

bool IsAvailable()
{
	if( s_status == UNCHECKED )
	{
		s_status = UNAVAILABLE;

		if( !g_skipNvidiaStreamline )
		{
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
				s_streamlineModule = LoadLibraryW( abs_path );
				available = !!s_streamlineModule;
			}
			CCP_LOGNOTICE( "Nvidia Streamline is %s", available ? "enabled" : "disabled" );
			s_status = available ? AVAILABLE : UNAVAILABLE;
		}
	}
	return s_status != UNAVAILABLE;
}

bool Initialize()
{
	if( !IsAvailable() )
	{
		return false;
	}

	if( s_status != AVAILABLE )
	{
		return s_status == INITIALIZED;
	}

	sl::Preferences pref{};

#ifndef NDEBUG
	pref.showConsole = true; // for debugging, set to false in production
#else
	pref.showConsole = false;
#endif
	auto features = std::vector<sl::Feature>();

	for( auto& pluginInfo : s_pluginInfo )
	{
		features.push_back( pluginInfo.second.streamlineID );
	}
#ifndef NDEBUG
	features.push_back( sl::kFeatureImGUI );
#endif

	sl::Feature* featuresToEnable = features.data();
	pref.numFeaturesToLoad = (uint32_t)features.size();

	pref.logLevel = sl::LogLevel::eDefault;

	pref.featuresToLoad = featuresToEnable;
	pref.logMessageCallback = StreamlineUtils::Log;
	pref.applicationId = EVE_ONLINE_APP_ID;
	pref.engine = sl::EngineType::eCustom;
	pref.flags |= sl::PreferenceFlags::eAllowOTA;
	pref.flags |= sl::PreferenceFlags::eUseManualHooking;

	DECLARE_STATIC_SL_FUNC( slInit );
	if( SL_FAILED( res, s_slInit( pref, sl::kSDKVersion ) ) )
	{
		s_status = UNINITIALIZED;

		if( res == sl::Result::eErrorDriverOutOfDate )
		{
			CCP_LOGWARN( "Could not initialize NVidia Streamline due to drivers being out of date" );
		}

		return false;
	}

	CCP_LOGNOTICE( "NVidia Streamline successfully initialized" );
	s_status = INITIALIZED;
	return true;
}

void CreateProxy( void* native, void** proxy )
{
	proxy = &native;

	if( s_status == UNAVAILABLE )
	{
		return;
	}
	DECLARE_STATIC_SL_FUNC( slUpgradeInterface );

	if( SL_FAILED( res, s_slUpgradeInterface( proxy ) ) )
	{
		CCP_LOGWARN( "Could not upgrade interface to proxy device (%d)", res );
	}
	CCP_ASSERT( proxy != nullptr );
}

void CreateNative( void* proxy, void** native )
{
	if( s_status == UNAVAILABLE )
	{
		return;
	}
	DECLARE_STATIC_SL_FUNC( slGetNativeInterface );

	if( SL_FAILED( res, s_slGetNativeInterface( proxy, native ) ) )
	{
		CCP_LOGWARN( "Could not get native interface from proxy device (%d)", res );
	}
	CCP_ASSERT( native != nullptr );
}

void Attach( void* device )
{
	if( s_status == UNAVAILABLE )
	{
		return;
	}
	DECLARE_STATIC_SL_FUNC( slSetD3DDevice );

	if( SL_FAILED( res, s_slSetD3DDevice( device ) ) )
	{
		CCP_LOGWARN( "Could not attach NVidia Streamline to device (%d)", res );
		return;
	}

	s_status = ATTACHED;
	CCP_LOGNOTICE( "NVidia Streamline successfully attached to device and adapter" );
}

void Shutdown()
{
	if( s_status == ATTACHED || s_status == INITIALIZED )
	{
		// Un-set all tags
		sl::ResourceTag inputs[] = {
			sl::ResourceTag{ nullptr, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent },
			sl::ResourceTag{ nullptr, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent },
			sl::ResourceTag{ nullptr, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent },
			sl::ResourceTag{ nullptr, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent },
			sl::ResourceTag{ nullptr, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent }
		};
		DECLARE_STATIC_SL_FUNC( slSetTag );
		s_slSetTag( 0, inputs, _countof( inputs ), nullptr );
		Toggle( StreamlineUtils::StreamlinePlugin::SP_DEBUG, false );
		Toggle( StreamlineUtils::StreamlinePlugin::SP_REFLEX, false );
		Toggle( StreamlineUtils::StreamlinePlugin::SP_NIS, false );
		Toggle( StreamlineUtils::StreamlinePlugin::SP_DLSS, false );
		Toggle( StreamlineUtils::StreamlinePlugin::SP_DLSSG, false );

		DECLARE_STATIC_SL_FUNC( slShutdown );

		if( SL_FAILED( result, s_slShutdown() ) )
		{
			CCP_LOGWARN( "Could not shutdown NVidia Streamline (%d)", result );
		}
	}
	s_status = UNCHECKED;
	// unload the streamline module
	s_streamlineModule = nullptr;
}

void UpdateFrameToken()
{
	if( s_status == ATTACHED )
	{
		DECLARE_STATIC_SL_FUNC( slGetNewFrameToken );

		if( SL_FAILED( res, s_slGetNewFrameToken( s_frameToken, nullptr ) ) )
		{
			CCP_LOGERR( "Could not get new frame token for Nvidia Streamline (%d)", res );
			return;
		}
		s_reset = false;
	}
}

// functions to create dx stuff from streamline or not
HRESULT SlCreateDXGIFactory( REFIID id, void** factory )
{
	if( !g_skipNvidiaStreamline )
	{
		typedef HRESULT( WINAPI * PFunCreateDXGIFactory )( REFIID, void** );
		static auto slCreateDXGIFactory = reinterpret_cast<PFunCreateDXGIFactory>( GetProcAddress( s_streamlineModule, "CreateDXGIFactory" ) );
		return slCreateDXGIFactory( id, factory );
	}
	return CreateDXGIFactory( id, factory );
}

HRESULT SlCreateDXGIFactory1( REFIID id, void** factory1 )
{
	if( !g_skipNvidiaStreamline )
	{
		typedef HRESULT( WINAPI * PFunCreateDXGIFactory1 )( REFIID, void** );
		static auto slCreateDXGIFactory1 = reinterpret_cast<PFunCreateDXGIFactory1>( GetProcAddress( s_streamlineModule, "CreateDXGIFactory1" ) );
		return slCreateDXGIFactory1( id, factory1 );
	}
	return CreateDXGIFactory1( id, factory1 );
}

bool IsValid( StreamlineUtils::StreamlinePlugin plugin, StreamlineUtils::PluginInfo& info )
{
	auto foundThing = s_pluginInfo.find( plugin );
	if( foundThing == s_pluginInfo.end() )
	{
		return false;
	}
	info = foundThing->second;
	return info.available != StreamlineUtils::SLPA_NO;
}

bool IsAvailable( StreamlineUtils::StreamlinePlugin plugin )
{
	if( s_status != ATTACHED )
	{
		return false;
	}
	// We are using NULL adapter on purpose
	sl::AdapterInfo adapterInfo{};

	StreamlineUtils::PluginInfo info;
	if( !IsValid( plugin, info ) )
	{
		return false;
	}
	if( info.available == StreamlineUtils::SLPA_YES )
	{
		return true;
	}

	uint32_t pluginId = info.streamlineID;

	DECLARE_STATIC_SL_FUNC( slIsFeatureSupported );
	auto result = s_slIsFeatureSupported( pluginId, adapterInfo );
	if( result != sl::Result::eOk )
	{
		CCP_LOGWARN( "NVidia Streamline plugin '%s' is not supported", info.pluginName );
		switch( result )
		{
		case sl::Result::eErrorOSOutOfDate: // inform user to update OS
			CCP_LOGWARN( "OS is out of date, please update OS to use %s", info.pluginName );
			break;
		case sl::Result::eErrorDriverOutOfDate: // inform user to update driver
			CCP_LOGWARN( "Driver is out of date, please update driver to use %s", info.pluginName );
			break;
		case sl::Result::eErrorAdapterNotSupported:
			CCP_LOGWARN( "No adapter found that supports %s", info.pluginName );
		default:
			CCP_LOGWARN( "No adapter found that supports %s", info.pluginName );
		};
		info.available = StreamlineUtils::SLPA_NO;
		s_pluginInfo[plugin] = info;
		return false;
	}

	CCP_LOGNOTICE( "NVidia Streamline plugin '%s' is available", info.pluginName );
	info.available = StreamlineUtils::SLPA_YES;
	s_pluginInfo[plugin] = info;
	return true;
}


void MarkFrameStart()
{
	if( s_reflexEnabled && s_frameToken )
	{
		DECLARE_STATIC_FEATURE_FUNC( slReflexSetMarker, sl::kFeatureReflex );
		if( SL_FAILED( result, s_slReflexSetMarker( sl::ReflexMarker::eRenderSubmitStart, *s_frameToken ) ) )
		{
			CCP_LOGERR( "Reflex failed to set marker eRenderSubmitStart(%d)", result );
		}
	}
}

void MarkFrameEnd()
{
	if( s_reflexEnabled && s_frameToken )
	{
		DECLARE_STATIC_FEATURE_FUNC( slReflexSetMarker, sl::kFeatureReflex );
		if( SL_FAILED( result, s_slReflexSetMarker( sl::ReflexMarker::eRenderSubmitEnd, *s_frameToken ) ) )
		{
			CCP_LOGERR( "Reflex failed to set marker eRenderSubmitEnd(%d)", result );
		}
	}
}

void MarkPresentStart()
{
	if( s_reflexEnabled && s_frameToken )
	{
		DECLARE_STATIC_FEATURE_FUNC( slReflexSetMarker, sl::kFeatureReflex );
		if( SL_FAILED( result, s_slReflexSetMarker( sl::ReflexMarker::ePresentStart, *s_frameToken ) ) )
		{
			CCP_LOGERR( "Reflex failed to set marker ePresentStart (%d)", result );
		}
	}
}

void MarkPresentEnd()
{
	if( s_reflexEnabled && s_frameToken )
	{
		DECLARE_STATIC_FEATURE_FUNC( slReflexSetMarker, sl::kFeatureReflex );
		if( SL_FAILED( result, s_slReflexSetMarker( sl::ReflexMarker::ePresentEnd, *s_frameToken ) ) )
		{
			CCP_LOGERR( "Reflex failed to set marker ePresentEnd (%d)", result );
		}
	}
}
void MarkUpdateStart()
{
	if( s_reflexEnabled && s_frameToken )
	{
		DECLARE_STATIC_FEATURE_FUNC( slReflexSetMarker, sl::kFeatureReflex );
		if( SL_FAILED( result, s_slReflexSetMarker( sl::ReflexMarker::eSimulationStart, *s_frameToken ) ) )
		{
			CCP_LOGERR( "Reflex failed to set marker ePresentEnd (%d)", result );
		}
	}
}
void MarkUpdateEnd()
{
	if( s_reflexEnabled && s_frameToken )
	{
		DECLARE_STATIC_FEATURE_FUNC( slReflexSetMarker, sl::kFeatureReflex );
		if( SL_FAILED( result, s_slReflexSetMarker( sl::ReflexMarker::eSimulationEnd, *s_frameToken ) ) )
		{
			CCP_LOGERR( "Reflex failed to set marker ePresentEnd (%d)", result );
		}
	}
}

bool IsRunning()
{
	return s_dlssPlugin != nullptr;
}

bool GetDlssPlugin( Tr2DlssPlugin& plugin )
{
	if( s_dlssPlugin == nullptr )
	{
		return false;
	}
	plugin = *s_dlssPlugin;
	return true;
}

bool GetNisPlugin( Tr2NisPlugin& plugin )
{
	if( s_nisPlugin == nullptr )
	{
		return false;
	}
	plugin = *s_nisPlugin;
	return true;
}

void Toggle( StreamlineUtils::StreamlinePlugin plugin, bool enable )
{
	StreamlineUtils::PluginInfo info;
	if( !IsAvailable( plugin ) || !IsValid( plugin, info ) )
	{
		return;
	}

	// check if the plugin is availabe
	if( s_pluginInfo[plugin].created == enable )
	{
		// don't need to do anything
		CCP_LOGERR( "Trying to %s Nvidia Streamline plugin '%s' but it is already %s", enable ? "enable" : "disable", info.pluginName, enable ? "enabled" : "disabled" );
		return;
	}

	uint32_t pluginId = info.streamlineID;

	DECLARE_STATIC_SL_FUNC( slSetFeatureLoaded );
	if( SL_FAILED( res, s_slSetFeatureLoaded( pluginId, enable ) ) )
	{
		CCP_LOGERR( "Trying to %s Nvidia Streamline plugin '%s' but it failed (%d)", enable ? "enable" : "disable", info.pluginName, res );
		return;
	}

	CCP_LOGNOTICE( "NVidia Streamline plugin '%s' is now %s", info.pluginName, enable ? "enabled" : "disabled" );

	s_pluginInfo[plugin].created = enable;

	// Setup/delete the plugin
	switch( plugin )
	{
	case StreamlineUtils::SP_DLSS:
		s_dlssPlugin = enable ? new Tr2DlssPlugin( 0, false ) : nullptr;
		break;
	case StreamlineUtils::SP_DLSSG:
		if( !s_dlssPlugin )
		{
			Toggle( StreamlineUtils::StreamlinePlugin::SP_DLSS, true );
		}

		s_dlssPlugin->EnableFrameGeneration( enable );
		PrepareReflex( enable );

		break;
	case StreamlineUtils::SP_NIS:
		s_nisPlugin = enable ? new Tr2NisPlugin( 0 ) : nullptr;
		break;
	default:
		break;
	}
}

void PrepareReflex( bool enable )
{
	if( enable )
	{
		Toggle( StreamlineUtils::StreamlinePlugin::SP_REFLEX, true );
	}

	auto reflexConst = sl::ReflexOptions{};
	reflexConst.mode = enable ? sl::ReflexMode::eLowLatency : sl::ReflexMode::eOff;
	reflexConst.useMarkersToOptimize = true;
	reflexConst.virtualKey = VK_F13;
	reflexConst.frameLimitUs = 0;
	DECLARE_STATIC_FEATURE_FUNC( slReflexSetOptions, sl::kFeatureReflex );
	if( SL_FAILED( result, s_slReflexSetOptions( reflexConst ) ) )
	{
		CCP_LOGERR( "Reflex failed to set options (%d)", result );
	}

	if( !enable )
	{
		Toggle( StreamlineUtils::StreamlinePlugin::SP_REFLEX, false );
	}
	s_reflexEnabled = enable;
}

void SetCommonConstants( StreamlineUtils::CommonConstants constants )
{
	s_commonConstants.cameraAspectRatio = constants.cameraAspectRatio;
	s_commonConstants.cameraFar = constants.cameraFar;
	s_commonConstants.cameraFOV = constants.cameraFOV;
	s_commonConstants.cameraFwd = AsFloat3( constants.cameraForward );
	s_commonConstants.cameraMotionIncluded = sl::eTrue;
	s_commonConstants.cameraNear = constants.cameraNear;
	s_commonConstants.cameraPos = AsFloat3( constants.cameraPos );
	s_commonConstants.cameraRight = AsFloat3( constants.cameraRight );
	s_commonConstants.cameraUp = AsFloat3( constants.cameraUp );
	s_commonConstants.cameraViewToClip = AsFloat4x4( constants.projection );
	s_commonConstants.clipToCameraView = AsFloat4x4( constants.invprojection );
	s_commonConstants.clipToPrevClip = AsFloat4x4( constants.clipToPrevClip );
	s_commonConstants.depthInverted = sl::eTrue;
	s_commonConstants.jitterOffset = AsFloat2( constants.jitterOffset );
	s_commonConstants.motionVectors3D = sl::eFalse;
	s_commonConstants.motionVectorsDilated = sl::eFalse;
	s_commonConstants.motionVectorsJittered = sl::eFalse;
	s_commonConstants.mvecScale = AsFloat2( constants.motionScale );
	s_commonConstants.orthographicProjection = sl::eFalse;
	s_commonConstants.prevClipToClip = AsFloat4x4( constants.prevClipToClip );
	// unused things
	s_commonConstants.cameraPinholeOffset = sl::float2( 0, 0 );
	s_commonConstants.reset = s_reset ? sl::eTrue : sl::eFalse;

	DECLARE_STATIC_SL_FUNC( slSetConstants );
	if( SL_FAILED( result, s_slSetConstants( s_commonConstants, *s_frameToken, 0 ) ) )
	{
		CCP_LOGERR( "Setting Nvidia Streamline common constants failed (%d)", result );
	}
}
}
Tr2DlssPlugin::Tr2DlssPlugin()
{
}

Tr2DlssPlugin::Tr2DlssPlugin( uint32_t id, bool framegeneration ) :
	m_id( id ),
	m_framegeneration( framegeneration ),
	m_dlssOptions( {} ),
	m_dlssgOptions( {} )
{
	m_view = sl::ViewportHandle( id );
}

Tr2DlssPlugin::~Tr2DlssPlugin()
{
}

DlssUtils::DlssOptimalSetting Tr2DlssPlugin::GetOptimalSettings( DlssUtils::DlssMode mode, uint32_t outputWidth, uint32_t outputHeight )
{
	sl::DLSSMode dlssMode;

	switch( mode )
	{
	case DlssUtils::dmUltraPerformance:
		dlssMode = sl::DLSSMode::eUltraPerformance;
		break;
	case DlssUtils::dmPerformance:
		dlssMode = sl::DLSSMode::eMaxPerformance;
		break;
	case DlssUtils::dmBalanced:
		dlssMode = sl::DLSSMode::eBalanced;
		break;
	case DlssUtils::dmQuality:
		dlssMode = sl::DLSSMode::eMaxQuality;
		break;
	case DlssUtils::dmUltraQuality:
		dlssMode = sl::DLSSMode::eDLAA;
		break;
	default:
		break;
	}
	sl::DLSSOptions options = {};
	options.outputWidth = outputWidth;
	options.outputHeight = outputHeight;
	options.mode = dlssMode;
	sl::DLSSOptimalSettings s = {};
	DECLARE_STATIC_FEATURE_FUNC( slDLSSGetOptimalSettings, sl::kFeatureDLSS )

	if( SL_FAILED( result, s_slDLSSGetOptimalSettings( options, s ) ) )
	{
		CCP_LOGERR( "Getting Optimal Settings for DLSS failed (%d)", result );
	}

	return DlssUtils::DlssOptimalSetting{
		s.optimalSharpness, s.optimalRenderWidth, s.optimalRenderHeight
	};
}

void Tr2DlssPlugin::UpdateState()
{
	DECLARE_STATIC_FEATURE_FUNC( slDLSSGGetState, sl::kFeatureDLSS_G )
	if( SL_FAILED( res, s_slDLSSGGetState( m_view, m_dlssgState, &m_dlssgOptions ) ) )
	{
		CCP_LOGERR( "Getting DLSSG State failed with (%d) - disabling framegeneration", res );
		m_framegeneration = false;
	}
}

void Tr2DlssPlugin::EnableFrameGeneration( bool enable )
{
	m_framegeneration = enable;
}

void Tr2DlssPlugin::SetSettings( DlssUtils::DlssOptions options )
{
	sl::DLSSMode dlssMode;

	switch( options.mode )
	{
	case DlssUtils::dmUltraPerformance:
		dlssMode = sl::DLSSMode::eUltraPerformance;
		break;
	case DlssUtils::dmPerformance:
		dlssMode = sl::DLSSMode::eMaxPerformance;
		break;
	case DlssUtils::dmBalanced:
		dlssMode = sl::DLSSMode::eBalanced;
		break;
	case DlssUtils::dmQuality:
		dlssMode = sl::DLSSMode::eMaxQuality;
		break;
	case DlssUtils::dmUltraQuality:
		dlssMode = sl::DLSSMode::eDLAA;
		break;
	default:
		break;
	}

	if( m_framegeneration )
	{
		UpdateState();

		// check if DLSS-G can be run
		if( m_dlssgState.status != sl::DLSSGStatus::eOk )
		{
			switch( m_dlssgState.status )
			{
			case sl::DLSSGStatus::eFailResolutionTooLow:
				CCP_LOGERR( "DLSS - Framegeneration disabled because the resolution is too small (%d, %d). Minimum resolution is %d", options.outputWidth, options.outputHeight, m_dlssgState.minWidthOrHeight );
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
			m_framegeneration = false;
		}
	}

	if( m_framegeneration )
	{
		m_dlssgOptions.mode = sl::DLSSGMode::eOn;

		m_dlssgOptions.colorWidth = options.outputWidth;
		m_dlssgOptions.colorHeight = options.outputHeight;
		m_dlssgOptions.colorBufferFormat = options.outputPixelFormat;

		m_dlssgOptions.depthBufferFormat = options.depthPixelFormat;
		m_dlssgOptions.mvecBufferFormat = options.motionVectorPixelFormat;
		m_dlssgOptions.mvecDepthWidth = options.renderWidth;
		m_dlssgOptions.mvecDepthHeight = options.renderWidth;

		DECLARE_STATIC_FEATURE_FUNC( slDLSSGSetOptions, sl::kFeatureDLSS_G )
		if( SL_FAILED( res, s_slDLSSGSetOptions( m_id, m_dlssgOptions ) ) )
		{
			CCP_LOGERR( "Setting DLSSG Options failed with (%d)", res );
		}
		else
		{
			CCP_LOGNOTICE( "DLSSG Options set successfully" );
		}
	}

	m_dlssOptions.outputWidth = options.outputWidth;
	m_dlssOptions.outputHeight = options.outputHeight;
	m_dlssOptions.colorBuffersHDR = options.hdr ? sl::eTrue : sl::eFalse;
	m_dlssOptions.mode = dlssMode;
	m_dlssOptions.useAutoExposure = sl::eFalse;

	DECLARE_STATIC_FEATURE_FUNC( slDLSSSetOptions, sl::kFeatureDLSS )
	if( SL_FAILED( res, s_slDLSSSetOptions( m_id, m_dlssOptions ) ) )
	{
		CCP_LOGERR( "Setting DLSS Options failed with (%d)", res );
	}
	else
	{
		CCP_LOGNOTICE( "DLSS Options set successfully" );
	}
}

void Tr2DlssPlugin::SetResources( DlssUtils::DlssOptions options, DlssUtils::DlssResources resources, Tr2RenderContextAL& renderContext )
{
	auto inputResource = StreamlineUtils::GenerateTextureResource( resources.input );
	auto outputResource = StreamlineUtils::GenerateTextureResource( resources.output );
	auto depthResource = StreamlineUtils::GenerateTextureResource( resources.depth );
	auto velocityResource = StreamlineUtils::GenerateTextureResource( resources.velocity );
	auto exposureResource = StreamlineUtils::GenerateTextureResource( resources.exposure );
	auto opaqueOnlyResource = StreamlineUtils::GenerateTextureResource( resources.opaqueOnly );

	sl::Extent renderExtent = {};
	renderExtent.height = options.renderHeight;
	renderExtent.width = options.renderWidth;

	sl::Extent displayExtent = {};
	displayExtent.height = resources.output->GetHeight();
	displayExtent.width = resources.output->GetWidth();

	sl::Extent exposureExtent = {};
	exposureExtent.height = 1;
	exposureExtent.width = 1;

	sl::ResourceTag opaqueColorInTag = sl::ResourceTag{ &opaqueOnlyResource, sl::kBufferTypeOpaqueColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag colorInTag = sl::ResourceTag{ &inputResource, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag colorOutTag = sl::ResourceTag{ &outputResource, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &displayExtent };
	sl::ResourceTag depthTag = sl::ResourceTag{ &depthResource, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag mvecTag = sl::ResourceTag{ &velocityResource, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag exposureTag = sl::ResourceTag{ &exposureResource, sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilPresent, &exposureExtent };

	sl::ResourceTag inputs[] = { colorInTag, opaqueColorInTag, colorOutTag, depthTag, mvecTag, exposureTag };

	DECLARE_STATIC_SL_FUNC( slSetTag );
	if( SL_FAILED( res, s_slSetTag( m_view, inputs, _countof( inputs ), StreamlineUtils::GetCommandBuffer( renderContext ) ) ) )
	{
		CCP_LOGERR( "DLSS Failed to tag resources (%d)", res );
	}
}

void Tr2DlssPlugin::SetHudLessResource( Tr2TextureAL* hudless, Tr2RenderContextAL& renderContext )
{
	if( !m_framegeneration )
		return;
	auto hudlessResource = StreamlineUtils::GenerateTextureResource( hudless );

	sl::Extent displayExtent = {};
	displayExtent.height = hudless->GetHeight();
	displayExtent.width = hudless->GetWidth();

	sl::ResourceTag hudlessResourceTag = sl::ResourceTag{ &hudlessResource, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, &displayExtent };

	sl::ResourceTag inputs[] = { hudlessResourceTag };

	DECLARE_STATIC_SL_FUNC( slSetTag );
	if( SL_FAILED( res, s_slSetTag( m_view, inputs, _countof( inputs ), StreamlineUtils::GetCommandBuffer( renderContext ) ) ) )
	{
		CCP_LOGERR( "DLSS-G Failed to tag hudless resource (%d)", res );
	}
}

void Tr2DlssPlugin::SetUiAndAlphaResource( Tr2TextureAL* uiAlpha, Tr2RenderContextAL& renderContext )
{
	if( !m_framegeneration )
		return;

	auto uiAndAlphaResource = StreamlineUtils::GenerateTextureResource( uiAlpha );

	sl::Extent displayExtent = {};
	displayExtent.height = uiAlpha->GetHeight();
	displayExtent.width = uiAlpha->GetWidth();

	sl::ResourceTag uiAndAlphaResourceTag = sl::ResourceTag{ &uiAndAlphaResource, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, &displayExtent };

	sl::ResourceTag inputs[] = { uiAndAlphaResourceTag };

	DECLARE_STATIC_SL_FUNC( slSetTag );
	if( SL_FAILED( res, s_slSetTag( m_view, inputs, _countof( inputs ), StreamlineUtils::GetCommandBuffer( renderContext ) ) ) )
	{
		CCP_LOGERR( "DLSS-G Failed to tag ui and alpha resource (%d)", res );
	}
}

void Tr2DlssPlugin::Dispatch( Tr2RenderContextAL& renderContext )
{
	const sl::BaseStructure* inputs[] = { &m_view };
	auto cb = StreamlineUtils::GetCommandBuffer( renderContext );

	DECLARE_STATIC_SL_FUNC( slEvaluateFeature );

	if( m_framegeneration )
	{
		UpdateState();
	}
	if( SL_FAILED( result, s_slEvaluateFeature( sl::kFeatureDLSS, *Tr2Streamline::s_frameToken, inputs, _countof( inputs ), cb ) ) )
	{
		CCP_LOGERR( "DLSS Failed to Dispatch (%d)", result );
	}
}

uint64_t Tr2DlssPlugin::GetEstimatedVRAMUsageInBytes()
{
	if( m_framegeneration )
	{
		return m_dlssgState.estimatedVRAMUsageInBytes;
	}
	return 0;
}

uint32_t Tr2DlssPlugin::GetMinWidthOrHeight()
{
	if( m_framegeneration )
	{
		return m_dlssgState.minWidthOrHeight;
	}
	return 0;
}

uint32_t Tr2DlssPlugin::GetNumFramesActuallyPresented()
{
	if( m_framegeneration )
	{
		return m_dlssgState.numFramesActuallyPresented;
	}
	return 0;
}

Tr2NisPlugin::Tr2NisPlugin()
{
}

Tr2NisPlugin::Tr2NisPlugin( uint32_t id ) :
	m_id( id )
{
	m_view = sl::ViewportHandle( id );
}

Tr2NisPlugin::~Tr2NisPlugin()
{
}

void Tr2NisPlugin::SetSettings( NisUtils::NisOptions options )
{
	sl::NISOptions sloptions = {};
	sloptions.mode = sl::NISMode::eScaler;
	sloptions.hdrMode = sl::NISHDR::eNone;
	sloptions.sharpness = options.sharpness;

	DECLARE_STATIC_FEATURE_FUNC( slNISSetOptions, sl::kFeatureNIS )
	if( SL_FAILED( res, s_slNISSetOptions( m_id, sloptions ) ) )
	{
		CCP_LOGERR( "Setting NIS Options failed with (%d)", res );
	}
	else
	{
		CCP_LOGNOTICE( "NIS Options set successfully" );
	}
}


void Tr2NisPlugin::SetResources( NisUtils::NisResources resources, Tr2RenderContextAL& renderContext )
{
	auto inputResource = StreamlineUtils::GenerateTextureResource( resources.input );
	auto outputResource = StreamlineUtils::GenerateTextureResource( resources.output );

	sl::Extent displayExtent = {};
	displayExtent.height = resources.output->GetHeight();
	displayExtent.width = resources.output->GetWidth();

	sl::ResourceTag colorInTag = sl::ResourceTag{ &inputResource, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &displayExtent };
	sl::ResourceTag colorOutTag = sl::ResourceTag{ &outputResource, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &displayExtent };

	sl::ResourceTag inputs[] = { colorInTag, colorOutTag };

	DECLARE_STATIC_SL_FUNC( slSetTag );
	if( SL_FAILED( res, s_slSetTag( m_view, inputs, _countof( inputs ), StreamlineUtils::GetCommandBuffer( renderContext ) ) ) )
	{
		CCP_LOGERR( "NIS Failed to tag resources (%d)", res );
	}
}

void Tr2NisPlugin::Dispatch( Tr2RenderContextAL& renderContext )
{
	const sl::BaseStructure* inputs[] = { &m_view };
	auto cb = StreamlineUtils::GetCommandBuffer( renderContext );

	DECLARE_STATIC_SL_FUNC( slEvaluateFeature );
	if( SL_FAILED( result, s_slEvaluateFeature( sl::kFeatureNIS, *Tr2Streamline::s_frameToken, inputs, _countof( inputs ), cb ) ) )
	{
		CCP_LOGERR( "NIS Failed to Dispatch (%d)", result );
	}
}

#endif