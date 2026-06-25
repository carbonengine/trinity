// Copyright © 2023 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>
#include <sl_dlss_g.h>
#include <sl_reflex.h>
#include <sl_nis.h>

#include "../include/Tr2TextureAL.h"
#include "Tr2TextureALDx12.h"
#include "Tr2RenderContextDx12.h"
#include "Tr2ResourceSetALDx12.h"
#include "Tr2GpuTimerALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Tr2RenderContextDx12.h"

#define DECLARE_STATIC_SL_FUNC( func ) \
	static PFun_##func* s_##func = reinterpret_cast<PFun_##func*>( GetProcAddress( Tr2Streamline::GetStreamlineModule(), #func ) )

#define DECLARE_STATIC_FEATURE_FUNC( func, feature )                                   \
	static PFun_##func* s_##func{};                                                    \
	if( !s_##func )                                                                    \
	{                                                                                  \
		DECLARE_STATIC_SL_FUNC( slGetFeatureFunction );                                \
		sl::Result res = s_slGetFeatureFunction( feature, #func, (void*&)s_##func );   \
		if( res != sl::Result::eOk )                                                   \
			CCP_LOGERR( "Unable to find function %s for feature %s", #func, feature ); \
	}


namespace StreamlineUtils
{
void Log( sl::LogType type, const char* msg );
sl::Resource GenerateTextureResource( Tr2TextureAL* texture );
sl::CommandBuffer* GetCommandBuffer( Tr2RenderContextAL& renderContext );

enum PluginAvailability
{
	SLPA_YES,
	SLPA_NO,
	SLPA_UNKNOWN
};

struct PluginInfo
{
	std::string pluginName;
	uint32_t streamlineID;
	PluginAvailability available;
	bool created;
};

enum StreamlinePlugin
{
	SP_DLSS,
	SP_DLSSG,
	SP_REFLEX,
	SP_NIS,
	SP_DEBUG,
	SP_COUNT
};

struct CommonConstants
{
	float projection[16];
	float invprojection[16];
	float clipToPrevClip[16];
	float prevClipToClip[16];
	float jitterOffset[2];
	float motionScale[2];
	float cameraPos[3];
	float cameraUp[3];
	float cameraRight[3];
	float cameraForward[3];
	float cameraNear;
	float cameraFar;
	float cameraFOV;
	float cameraAspectRatio;
	bool reset;
};
}

namespace DlssUtils
{
enum DlssMode
{
	dmUltraPerformance,
	dmPerformance,
	dmBalanced,
	dmQuality,
	dmUltraQuality,
};

struct DlssOptions
{
	DlssMode mode;
	uint32_t outputWidth;
	uint32_t outputHeight;
	uint32_t renderWidth;
	uint32_t renderHeight;
	Tr2RenderContextEnum::PixelFormat renderPixelFormat;
	Tr2RenderContextEnum::PixelFormat outputPixelFormat;
	Tr2RenderContextEnum::PixelFormat motionVectorPixelFormat;
	Tr2RenderContextEnum::PixelFormat depthPixelFormat;
	bool hdr;
};

struct DlssOptimalSetting
{
	float sharpness;
	uint32_t renderWidth;
	uint32_t renderHeight;
};

struct DlssResources
{
	Tr2TextureAL* input;
	Tr2TextureAL* depth;
	Tr2TextureAL* velocity;
	Tr2TextureAL* output;
	Tr2TextureAL* opaqueOnly;
	Tr2TextureAL* exposure;
	Tr2TextureAL* reactive;
};
}

namespace NisUtils
{

struct NisResources
{
	Tr2TextureAL* input;
	Tr2TextureAL* output;
};

struct NisOptions
{
	float sharpness;
	bool hdr;
};
}

class Tr2DlssPlugin
{
public:
	Tr2DlssPlugin();
	Tr2DlssPlugin( uint32_t id, bool framegeneration );
	~Tr2DlssPlugin();

	DlssUtils::DlssOptimalSetting GetOptimalSettings( DlssUtils::DlssMode mode, uint32_t outputWidth, uint32_t outputHeight );
	void SetSettings( DlssUtils::DlssOptions options );
	void SetResources( DlssUtils::DlssOptions options, DlssUtils::DlssResources resources, Tr2RenderContextAL& renderContext );
	void SetHudLessResource( Tr2TextureAL* hudless, Tr2RenderContextAL& renderContext );
	void SetUiAndAlphaResource( Tr2TextureAL* uiAlpha, Tr2RenderContextAL& renderContext );
	void Dispatch( Tr2RenderContextAL& renderContext );

	uint64_t GetEstimatedVRAMUsageInBytes();
	uint32_t GetMinWidthOrHeight();
	uint32_t GetNumFramesActuallyPresented();
	void EnableFrameGeneration( bool enable );

protected:
	uint32_t m_id;
	bool m_framegeneration;

	void UpdateState();
	sl::ViewportHandle m_view;
	sl::DLSSGOptions m_dlssgOptions;
	sl::DLSSGState m_dlssgState;
	sl::DLSSOptions m_dlssOptions;
};


class Tr2NisPlugin
{
public:
	Tr2NisPlugin();
	Tr2NisPlugin( uint32_t id );
	~Tr2NisPlugin();

	void SetSettings( NisUtils::NisOptions options );
	void SetResources( NisUtils::NisResources resources, Tr2RenderContextAL& renderContext );
	void Dispatch( Tr2RenderContextAL& renderContext );

private:
	uint32_t m_id;
	sl::ViewportHandle m_view;
};

namespace Tr2Streamline
{
enum Status
{
	UNCHECKED,
	AVAILABLE,
	UNAVAILABLE,
	INITIALIZED,
	UNINITIALIZED,
	ATTACHED
};

// static things
bool IsAvailable();
bool IsAvailable( StreamlineUtils::StreamlinePlugin plugin );
bool IsInitialized();

void MarkFrameStart();
void MarkFrameEnd();
void MarkPresentStart();
void MarkPresentEnd();
void MarkUpdateStart();
void MarkUpdateEnd();
void SetSwapchainCreator( std::function<void()> creator );
void SetSwapchainDestroyer( std::function<void()> destroyer );

void UpdateFrameToken();

HMODULE GetStreamlineModule();

template <typename T>
void CheckForProxy( T thing, const std::string& message )
{
	// Use special GUID to to obtain the underlying native interface if SL proxy is used, returns null otherwise
	IID riid;
	IIDFromString( L"{ADEC44E2-61F0-45C3-AD9F-1B37379284FF}", &riid );
	// This can be ID3D12Device, IDXGIFactory, ID3D12GraphicsCommandList or any other interface which has SL proxy
	ATL::CComPtr<T> native = nullptr;
	thing->QueryInterface( riid, reinterpret_cast<void**>( &native.p ) );

	if( native )
	{
		CCP_LOG( "%s - is proxy!", message );
		native.Release();
	}
	else
	{

		CCP_LOG( "%s - is not a proxy!", message );
	}
};


bool Initialize();
void Shutdown();

template <typename T>
ATL::CComPtr<T> CreateProxy( ATL::CComPtr<T> native )
{
	if( !IsAvailable() )
	{
		return native;
	}
	DECLARE_STATIC_SL_FUNC( slUpgradeInterface );
	ATL::CComPtr<T> proxy;
	if( SL_FAILED( res, s_slUpgradeInterface( (void**)&proxy.p ) ) )
	{
		CCP_LOGWARN( "Could not upgrade interface to proxy device (%d)", res );
	}
	CCP_ASSERT( proxy != nullptr );
	return proxy;
};

template <typename T>
ATL::CComPtr<T> CreateNative( ATL::CComPtr<T> proxy )
{
	if( !IsAvailable() )
	{
		return proxy;
	}
	DECLARE_STATIC_SL_FUNC( slGetNativeInterface );
	ATL::CComPtr<T> native;
	if( SL_FAILED( res, s_slGetNativeInterface( proxy.p, (void**)&native.p ) ) )
	{
		CCP_LOGWARN( "Could not get native interface from proxy device (%d)", res );
	}
	CCP_ASSERT( native != nullptr );
	return native;
};


void Attach( void* d3dDevice );

HRESULT SlCreateDXGIFactory( REFIID id, void** factory );
HRESULT SlCreateDXGIFactory1( REFIID id, void** factory1 );
HRESULT SlCreateDXGIFactory2( UINT flags, REFIID id, void** factory2 );
HRESULT SlD3D12CreateDevice( IUnknown* adapter, D3D_FEATURE_LEVEL featureLevel, REFIID id, void** device );

bool IsRunning();
bool GetDlssPlugin( Tr2DlssPlugin& plugin );
bool GetNisPlugin( Tr2NisPlugin& plugin );
void Toggle( StreamlineUtils::StreamlinePlugin plugin, bool enable );
void SetCommonConstants( StreamlineUtils::CommonConstants constants );

void FreeResources();

void PrepareReflex( bool enable );
bool IsValid( StreamlineUtils::StreamlinePlugin plugin, StreamlineUtils::PluginInfo& info );

};
#endif