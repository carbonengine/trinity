#include "StdAfx.h"
#include "../include/Tr2StreamlineAL.h"
#include "Tr2AdapterStructures.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12 || TRINITY_PLATFORM == TRINITY_DIRECTX11

#include <filesystem>
#include <sl_security.h>

extern bool g_upscalingDebug;

namespace Tr2StreamlineAL
{
	static HMODULE STREAMLINE_MODULE = nullptr;
	static bool STREAMLINE_INITIALIZED = false;
	static sl::Result STREAMLINE_INITIALIZATION_RESULT = sl::Result::eOk;

	
	static bool STREAMLINE_DEVICE_SET = false;
	static bool STREAMLINE_DLSS_SUPPORTED = false;
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	static bool STREAMLINE_FRAME_GENERATION_SUPPORTED = false;
#endif


	#define INITIALIZE_FUNCTION( func ) \
		FUNCTIONS.m_##func = reinterpret_cast<PFun_##func*>( GetProcAddress( STREAMLINE_MODULE, #func ) )

	#define INITIALIZE_FEATURE_FUNCTION( feature, func )                                    \
		if( SL_FAILED( res, FUNCTIONS.m_slGetFeatureFunction( feature, #func, (void*&)FEATURE_FUNCTIONS.m_##func ) ) ) \
			CCP_LOGERR( "Unable to find function %s for feature %s Error code: %d", #func, #feature, res ); 
	

	//These two structs are automatically zero-initialized
	static struct
	{
		//initialization
		PFun_slInit* m_slInit;
		PFun_slShutdown* m_slShutdown;

		//wrapping
		PFun_slSetD3DDevice* m_slSetD3DDevice;
		PFun_slUpgradeInterface* m_slUpgradeInterface;

		//feature support
		PFun_slIsFeatureSupported* m_slIsFeatureSupported;
		PFun_slSetFeatureLoaded* m_slSetFeatureLoaded;
		PFun_slGetFeatureFunction* m_slGetFeatureFunction;

		//frame tokens
		PFun_slGetNewFrameToken* m_slGetNewFrameToken;

		//resource allocation
		PFun_slAllocateResources* m_slAllocateResources;
		PFun_slFreeResources* m_slFreeResources;

		//dispatching
		PFun_slSetTag* m_slSetTag;
		PFun_slSetConstants* m_slSetConstants;
		PFun_slEvaluateFeature* m_slEvaluateFeature;

	} FUNCTIONS;

	static struct
	{

		//DLSS
		PFun_slDLSSGetOptimalSettings* m_slDLSSGetOptimalSettings;
		PFun_slDLSSSetOptions* m_slDLSSSetOptions;

		//NIS
		PFun_slNISSetOptions* m_slNISSetOptions;

		
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
		//DLSSG
		PFun_slDLSSGSetOptions* m_slDLSSGSetOptions;
		PFun_slDLSSGGetState* m_slDLSSGGetState;

		//Reflex
		PFun_slReflexSetOptions* m_slReflexSetOptions;

		//PCL
		PFun_slPCLSetMarker* m_slPCLSetMarker;
#endif

	} FEATURE_FUNCTIONS;

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

	sl::float4x4 F16AsFloat4x4( const float f[16] )
	{
		sl::float4x4 m;
		m.setRow( 0, sl::float4( f[0], f[1], f[2], f[3] ) );
		m.setRow( 1, sl::float4( f[4], f[5], f[6], f[7] ) );
		m.setRow( 2, sl::float4( f[8], f[9], f[10], f[11] ) );
		m.setRow( 3, sl::float4( f[12], f[13], f[14], f[15] ) );
		return m;
	}

	sl::Result InitializeStreamline()
	{
		if( STREAMLINE_INITIALIZED )
		{
			CCP_LOGERR( "Streamline already initialized with result code %d", STREAMLINE_INITIALIZATION_RESULT );

			return STREAMLINE_INITIALIZATION_RESULT;
		}


		// load the DLL
		{
			wchar_t abs_path[4096];
			auto size = SearchPathW( nullptr, L"sl.interposer.dll", L"", 4096, abs_path, nullptr );
			if( size == 0 )
			{
				CCP_LOGERR( "Unable to find sl.interposer.dll in path for secure load." );

				return sl::Result::eErrorIO; //Most fitting error...
			}
			else if( g_upscalingDebug )
			{
				STREAMLINE_MODULE = LoadLibraryW( abs_path );
			}
			else if( sl::security::verifyEmbeddedSignature( abs_path ) )
			{
				STREAMLINE_MODULE = LoadLibraryW( abs_path );
			}
#
			CCP_LOGNOTICE( "NVidia Streamline library loaded" );
		}

		// initialize function pointers
		{
			INITIALIZE_FUNCTION( slInit );
			INITIALIZE_FUNCTION( slShutdown );

			INITIALIZE_FUNCTION( slSetD3DDevice );
			INITIALIZE_FUNCTION( slUpgradeInterface );

			INITIALIZE_FUNCTION( slIsFeatureSupported );
			INITIALIZE_FUNCTION( slSetFeatureLoaded );
			INITIALIZE_FUNCTION( slGetFeatureFunction );
			

			INITIALIZE_FUNCTION( slGetNewFrameToken );

			INITIALIZE_FUNCTION( slAllocateResources );
			INITIALIZE_FUNCTION( slFreeResources );

			INITIALIZE_FUNCTION( slSetTag );
			INITIALIZE_FUNCTION( slSetConstants );
			INITIALIZE_FUNCTION( slEvaluateFeature );
		}



		// now set it up

		{
			sl::Preferences pref{};

#if TRINITY_PLATFORM == TRINITY_DIRECTX11
			pref.renderAPI = sl::RenderAPI::eD3D11;
#elif TRINITY_PLATFORM == TRINITY_DIRECTX12
			pref.renderAPI = sl::RenderAPI::eD3D12;
#endif


			std::vector<sl::Feature> features;

			features.push_back( sl::kFeatureDLSS ); // dlss module
			features.push_back( sl::kFeatureNIS ); // image sharpening module

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
			features.push_back( sl::kFeatureDLSS_G ); // frame generation is only available on dx12
			features.push_back( sl::kFeatureReflex ); // dlssg requires reflex
			features.push_back( sl::kFeaturePCL );
#endif

			if( g_upscalingDebug )
			{
				pref.showConsole = true; // for debugging, set to false in production
				pref.logLevel = sl::LogLevel::eVerbose;
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
				features.push_back( sl::kFeatureImGUI );
#endif
			}
			else
			{
				pref.showConsole = false;
				pref.logLevel = sl::LogLevel::eOff;
			}

			sl::Feature* featuresToEnable = features.data();
			pref.numFeaturesToLoad = (uint32_t)features.size();
			pref.featuresToLoad = featuresToEnable;
			pref.logMessageCallback = Tr2StreamlineLog;

			// the ID of Eve Online
			const uint32_t EVE_ONLINE_APP_ID = 101109911;

			pref.applicationId = EVE_ONLINE_APP_ID;
			pref.engine = sl::EngineType::eCustom;
			pref.flags |= sl::PreferenceFlags::eUseManualHooking;
			STREAMLINE_INITIALIZATION_RESULT = FUNCTIONS.m_slInit( pref, sl::kSDKVersion );

			if( STREAMLINE_INITIALIZATION_RESULT != sl::Result::eOk )
			{
				CCP_LOGERR( "NVidia Streamline NOT initialized. Error code: %d", STREAMLINE_INITIALIZATION_RESULT );
				ReleaseStreamline();
				return STREAMLINE_INITIALIZATION_RESULT;
			}
		}


		STREAMLINE_INITIALIZED = true;
		CCP_LOGNOTICE( "NVidia Streamline successfully initialized" );

		return STREAMLINE_INITIALIZATION_RESULT;
	}

	void ReleaseStreamline( )
	{
		if( STREAMLINE_MODULE )
		{
			if (STREAMLINE_INITIALIZED)
			{
				if( SL_FAILED( res, FUNCTIONS.m_slShutdown() ) )
				{
					CCP_LOGERR( "Could not release streamline %d", res );
				}
				else
				{
					CCP_LOGNOTICE( "NVidia Streamline successfully released" );
				}
			}

			memset( &FUNCTIONS, 0, sizeof( FUNCTIONS ) );
			memset( &FEATURE_FUNCTIONS, 0, sizeof( FEATURE_FUNCTIONS ) );

			FreeLibrary( STREAMLINE_MODULE );

			STREAMLINE_INITIALIZED = false;
			STREAMLINE_MODULE = nullptr;
			STREAMLINE_DEVICE_SET = false;
			STREAMLINE_DLSS_SUPPORTED = false;
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
			STREAMLINE_FRAME_GENERATION_SUPPORTED = false;
#endif
		}
	}


	sl::CommandBuffer* GetCommandBuffer( Tr2RenderContextAL& renderContext )
	{
#if TRINITY_PLATFORM == TRINITY_DIRECTX11
		return renderContext.m_context;
#elif TRINITY_PLATFORM == TRINITY_DIRECTX12
		return renderContext.m_commandList;
#endif
	}

	bool CheckFeature( sl::AdapterInfo adapterInfo, sl::Feature feature )
	{

		auto pluginName = GetPluginName( feature );

		auto result = FUNCTIONS.m_slIsFeatureSupported( feature, adapterInfo );
		if( result != sl::Result::eOk )
		{
			CCP_LOGNOTICE( "NVidia Streamline plugin '%s' is available", pluginName );
			switch( result )
			{
			case sl::Result::eErrorOSOutOfDate: // inform user to update OS
				CCP_LOGWARN( "OS is out of date, please update OS to use %s", pluginName );
				break;
			case sl::Result::eErrorDriverOutOfDate: // inform user to update driver
				CCP_LOGWARN( "Driver is out of date, please update driver to use %s", pluginName );
				break;
			case sl::Result::eErrorAdapterNotSupported:
				CCP_LOGWARN( "Current adapter doesn't support %s", pluginName );
				break;
			case sl::Result::eErrorMissingOrInvalidAPI:
				CCP_LOGWARN( "Graphics API not supported for %s", pluginName );
				break;
			default:
				CCP_LOGWARN( "NVidia Streamline plugin '%s' is not supported, Streamline error: %d", pluginName, result );
			};

			return false;
		}
		else
		{
			CCP_LOGNOTICE( "NVidia Streamline plugin '%s' is available", pluginName );
		}
		return true;
	}

	sl::AdapterInfo GetAdapterInfo( uint32_t adapter )
	{
		Tr2AdapterInfo videoAdapterInfo;
		Tr2VideoAdapterInfo::GetAdapterInfo( adapter, videoAdapterInfo );

		sl::AdapterInfo adapterInfo{};
		adapterInfo.deviceLUID = videoAdapterInfo.luid;
		adapterInfo.deviceLUIDSizeInBytes = sizeof( LUID );
		return adapterInfo;
	}

	sl::Result SetDevice( void* d3dDevice, uint32_t adapter )
	{
		if (STREAMLINE_DEVICE_SET)
		{
			CCP_LOGERR( "Streamline D3D device already set!" );
			return sl::Result::eOk;
		}

		sl::Result result = FUNCTIONS.m_slSetD3DDevice( d3dDevice );

		if (result != sl::Result::eOk)
		{
			CCP_LOGERR( "Failed to set Streamline D3D device!" );
			return result;
		}

		STREAMLINE_DEVICE_SET = true;
		
		sl::AdapterInfo info = GetAdapterInfo( adapter );

		bool dlssSupported = true;
		dlssSupported &= CheckFeature( info, sl::kFeatureDLSS );
		dlssSupported &= CheckFeature( info, sl::kFeatureNIS );
		if (dlssSupported)
		{
			INITIALIZE_FEATURE_FUNCTION( sl::kFeatureDLSS, slDLSSGetOptimalSettings );
			INITIALIZE_FEATURE_FUNCTION( sl::kFeatureDLSS, slDLSSSetOptions );

			INITIALIZE_FEATURE_FUNCTION( sl::kFeatureNIS, slNISSetOptions );
			STREAMLINE_DLSS_SUPPORTED = true;
		}

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

		bool frameGenerationSupport = dlssSupported;
		frameGenerationSupport &= CheckFeature( info, sl::kFeatureDLSS_G );
		frameGenerationSupport &= CheckFeature( info, sl::kFeatureReflex );
		frameGenerationSupport &= CheckFeature( info, sl::kFeaturePCL );

		if( frameGenerationSupport ) {
			INITIALIZE_FEATURE_FUNCTION( sl::kFeatureDLSS_G, slDLSSGSetOptions );
			INITIALIZE_FEATURE_FUNCTION( sl::kFeatureDLSS_G, slDLSSGGetState );

			INITIALIZE_FEATURE_FUNCTION( sl::kFeatureReflex, slReflexSetOptions );

			INITIALIZE_FEATURE_FUNCTION( sl::kFeaturePCL, slPCLSetMarker );
			STREAMLINE_FRAME_GENERATION_SUPPORTED = true;
		}
#endif


		return result;
	}

	sl::Result UpgradeInterface( void** nativeInterface )
	{
		return FUNCTIONS.m_slUpgradeInterface( nativeInterface );
	}

	sl::Result SetFeatureLoaded( sl::Feature feature, bool enable )
	{
		return FUNCTIONS.m_slSetFeatureLoaded( feature, enable );
	}

	sl::Result GetNewFrameToken( sl::FrameToken*& m_frameToken )
	{
		return FUNCTIONS.m_slGetNewFrameToken( m_frameToken, nullptr );
	}

	sl::Result AllocateResources( Tr2RenderContextAL& renderContext, sl::Feature feature, const sl::ViewportHandle& viewport )
	{
		return FUNCTIONS.m_slAllocateResources( GetCommandBuffer(renderContext), feature, viewport );
	}

	sl::Result FreeResources( sl::Feature feature, const sl::ViewportHandle& viewport )
	{
		return FUNCTIONS.m_slFreeResources( feature, viewport );
	}


	sl::Result SetTags( Tr2RenderContextAL& renderContext, const sl::ViewportHandle& viewport, const sl::ResourceTag* tags, uint32_t numTags )
	{
		return FUNCTIONS.m_slSetTag( viewport, tags, numTags, GetCommandBuffer( renderContext ) );
	}

	sl::Result SetConstants( const sl::Constants& values, const sl::FrameToken& frame, const sl::ViewportHandle& viewport )
	{
		return FUNCTIONS.m_slSetConstants( values, frame, viewport );
	}

	sl::Result EvaluateFeature( Tr2RenderContextAL& renderContext, sl::Feature feature, const sl::FrameToken& frame, const sl::BaseStructure** inputs, uint32_t numInputs )
	{
		return FUNCTIONS.m_slEvaluateFeature( feature, frame, inputs, numInputs, GetCommandBuffer( renderContext ) );
	}




	sl::Result GetDLSSOptimalSettings( const sl::DLSSOptions& options, sl::DLSSOptimalSettings& settings )
	{
		return FEATURE_FUNCTIONS.m_slDLSSGetOptimalSettings( options, settings );
	}

	sl::Result SetDLSSOptions( const sl::ViewportHandle& viewport, const sl::DLSSOptions& options )
	{
		return FEATURE_FUNCTIONS.m_slDLSSSetOptions( viewport, options );
	}

	sl::Result SetNISOptions( const sl::ViewportHandle& viewport, const sl::NISOptions& options )
	{
		return FEATURE_FUNCTIONS.m_slNISSetOptions( viewport, options );
	}

	
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	
	sl::Result SetDLSSGOptions( const sl::ViewportHandle& viewport, const sl::DLSSGOptions& options )
	{
		return FEATURE_FUNCTIONS.m_slDLSSGSetOptions(viewport, options);
	}

	sl::Result GetDLSSGState( const sl::ViewportHandle& viewport, sl::DLSSGState& state, const sl::DLSSGOptions* options )
	{
		return FEATURE_FUNCTIONS.m_slDLSSGGetState( viewport, state, options );
	}


	sl::Result SetReflexOptions( const sl::ReflexOptions& options )
	{
		return FEATURE_FUNCTIONS.m_slReflexSetOptions( options );
	}


	void SetPCLMarker( Tr2RenderContextEnum::FrameEvent& frameEvent, sl::FrameToken* m_frameToken )
	{

		if( !m_frameToken )
		{
			return;
		}

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
			break;
		case Tr2RenderContextEnum::FRAME_EVENT_RENDERING_FINISHED:
			slEvent = sl::PCLMarker::eRenderSubmitEnd;
			break;
		}

		if( SL_FAILED( result, FEATURE_FUNCTIONS.m_slPCLSetMarker( slEvent, *m_frameToken ) ) )
		{
			CCP_LOGERR( "Reflex failed to set marker %d (%d)", slEvent, result );
		}
	}

#endif

	bool IsDLSSAvailable()
	{
		if( !STREAMLINE_DEVICE_SET )
		{
			CCP_LOGERR( "Tr2StreamlineAL::IsDLSSAvailable() called before D3D device was set!" );
		}
		return STREAMLINE_DLSS_SUPPORTED;
	}

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	bool IsFrameGenerationAvailable()
	{
		if (!STREAMLINE_DEVICE_SET)
		{
			CCP_LOGERR( "Tr2StreamlineAL::IsFrameGenerationAvailable() called before D3D device was set!" );
		}
		return STREAMLINE_FRAME_GENERATION_SUPPORTED;
	}
#endif


}
#endif
