#include "StdAfx.h"
#include "../include/Tr2StreamlineAL.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12 || TRINITY_PLATFORM == TRINITY_DIRECTX11

#include <filesystem>
#include <sl_security.h>

namespace Tr2StreamlineAL
{
	HMODULE GetStreamlineModule( )
	{
		// we only need to do this once!
		static HMODULE STREAMLINE_MODULE;
		
		if( STREAMLINE_MODULE )
		{
			return STREAMLINE_MODULE;
		}

		wchar_t abs_path[2048];
		auto size = SearchPathW( nullptr, L"sl.interposer.dll", L"", 2048, abs_path, nullptr );
		if( size == 0 )
		{
			CCP_LOGERR( "Unable to find sl.interposer.dll in path for secure load." );
		}
#ifndef NDEBUG
		else 
		{
			STREAMLINE_MODULE = LoadLibraryW( abs_path );
		}
#else
		else if( sl::security::verifyEmbeddedSignature( abs_path ) )
		{
			STREAMLINE_MODULE = LoadLibraryW( abs_path );
		}
#endif

		return STREAMLINE_MODULE;
	}

	void Tr2StreamlineLog( sl::LogType type, const char* msg )
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

	sl::float4x4 F16AsFloat4x4( float f[16] )
	{
		sl::float4x4 m;
		m.setRow( 0, sl::float4( f[0], f[1], f[2], f[3] ) );
		m.setRow( 1, sl::float4( f[4], f[5], f[6], f[7] ) );
		m.setRow( 2, sl::float4( f[8], f[9], f[10], f[11] ) );
		m.setRow( 3, sl::float4( f[12], f[13], f[14], f[15] ) );
		return m;
	}

	sl::Result InitializeStreamline( HMODULE streamlineModule )
	{
		// guards against initializing streamline multiple times...
		static bool SETUP = false;
		static sl::Result SL_INIT_RESULT;

		if( SETUP )
		{
			CCP_LOGNOTICE( "Streamline already initialized with result code %d", SL_INIT_RESULT );
			
			return SL_INIT_RESULT;
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
#if TRINITY_PLATFORM == TRINITY_DIRECTX11
		pref.renderAPI = sl::RenderAPI::eD3D11;
#elif TRINITY_PLATFORM == TRINITY_DIRECTX12
		pref.renderAPI = sl::RenderAPI::eD3D12;
#endif

		std::vector<sl::Feature> features = {
			sl::kFeatureDLSS, // dlss module
			sl::kFeatureNIS // image sharpening module
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
			,sl::kFeatureDLSS_G, // framegeneration is only available on dx12
			sl::kFeatureReflex // dlssg requires reflex
#ifndef NDEBUG
			,sl::kFeatureImGUI // imgui is only availabe with reflex and dlssg
#endif
#endif
		};


		sl::Feature* featuresToEnable = features.data();
		pref.numFeaturesToLoad = (uint32_t)features.size();
		pref.featuresToLoad = featuresToEnable;
		pref.logMessageCallback = Tr2StreamlineLog;

		// the ID of Eve Online
		const uint32_t EVE_ONLINE_APP_ID = 101109911;

		pref.applicationId = EVE_ONLINE_APP_ID;
		pref.engine = sl::EngineType::eCustom;
		pref.flags |= sl::PreferenceFlags::eAllowOTA;
		pref.flags |= sl::PreferenceFlags::eUseManualHooking;
		SL_INIT_RESULT = reinterpret_cast<PFun_slInit*>( GetProcAddress( streamlineModule, "slInit" ) )( pref, sl::kSDKVersion );

		if( SL_INIT_RESULT != sl::Result::eOk )
		{
			CCP_LOGERR( "NVidia Streamline NOT initialized. Error code: %d", SL_INIT_RESULT );
		}

		SETUP = true;
		CCP_LOGNOTICE( "NVidia Streamline successfully initialized" );

		return SL_INIT_RESULT;
	}

	void ReleaseStreamline( HMODULE streamlineModule )
	{
		if( SL_FAILED( res, reinterpret_cast<PFun_slShutdown*>( GetProcAddress( streamlineModule, "slShutdown" ) )() ) )
		{
			CCP_LOGNOTICE( "Could not release streamline %d", res );
		}
	}

	sl::Result CheckForAvailability( HMODULE streamlineModule, sl::Feature feature, sl::AdapterInfo adapterInfo )
	{
		auto pluginName = GetPluginName( feature );

		PFun_slIsFeatureSupported* slIsFeatureSupported = reinterpret_cast<PFun_slIsFeatureSupported*>( GetProcAddress( streamlineModule, "slIsFeatureSupported" ) );

		auto result = slIsFeatureSupported( feature, adapterInfo );
		if( result != sl::Result::eOk )
		{
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
		}
		else
		{
			CCP_LOGNOTICE( "NVidia Streamline plugin '%s' is available", pluginName );
		}
		return result;
	}
}
#endif
