#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

extern bool g_wantsEXDevice;

#include "Tr2VideoAdapterInfoALDx9.h"
#include "Tr2AdapterStructures.h"
#include "Tr2RenderContextDx9.h"

extern bool g_usingEXDevice;

using namespace Tr2RenderContextEnum;

namespace
{
CComPtr<IDirect3D9> s_direct3D;

ALResult InitializeDirect3D()
{
	s_direct3D = nullptr;

	//
	// First see if we the extended device is supported
	//
	g_usingEXDevice = false;

	// Define a function pointer to the Direct3DCreate9Ex function.
	typedef HRESULT ( WINAPI*LPDIRECT3DCREATE9EX )( uint32_t,
													void** );
	LPDIRECT3DCREATE9EX Direct3DCreate9ExPtr = NULL;

	if ( g_wantsEXDevice )
	{
		HMODULE libHandle = NULL;

		// Manually load the d3d9.dll library.
		libHandle = LoadLibrary( "d3d9.dll" );
		if ( libHandle != NULL )
		{
			// Obtain the address of the Direct3DCreate9Ex function. 
			Direct3DCreate9ExPtr = ( LPDIRECT3DCREATE9EX )GetProcAddress( libHandle, "Direct3DCreate9Ex" );

			if ( Direct3DCreate9ExPtr != NULL )
			{
				// Direct3DCreate9Ex is supported.
				g_usingEXDevice = true;
			}

			// Free the library.
			FreeLibrary( libHandle );
		}
		else
		{
			return E_FAIL;
		}
	}

	// Create the Direct3D object
	if ( g_usingEXDevice )
	{
		// Direct3DCreate9Ex is supported.
		IDirect3D9Ex* d3d = NULL;
		( *Direct3DCreate9ExPtr )( D3D_SDK_VERSION, ( void** )&d3d );
		if ( !d3d )
		{
			CCP_LOGERR( "Failed to create the extended WDDM Direct3D object" );
			g_usingEXDevice = false;
		}
		else
		{
			CCP_LOG( "Trinity is using the extended WDDM device" );
			s_direct3D.Attach( ( IDirect3D9* )d3d );
		}

	}

	if ( !s_direct3D )
	{
		IDirect3D9* d3d = NULL;
		d3d = Direct3DCreate9( D3D_SDK_VERSION );
		if ( !d3d )
		{
			CCP_LOGERR( "Failed to create the Direct3D object" );
			g_usingEXDevice = false;
		}
		CCP_LOG( "Trinity is using the regular device" );
		s_direct3D.Attach( d3d );
	}

	if ( !s_direct3D )
	{
		return E_FAIL;
	}
	return S_OK;
}

}

ALResult Tr2VideoAdapterInfo::GetAdapterCount( unsigned& count )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	count = s_direct3D->GetAdapterCount();
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterInfo( unsigned adapterIndex,
											  Tr2AdapterInfo& info )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	D3DADAPTER_IDENTIFIER9 id;
	CR_RETURN_HR( s_direct3D->GetAdapterIdentifier( adapterIndex, 0, &id ) );
	info.driver = id.Driver;
	info.description = CA2W( id.Description );
	info.deviceName = id.DeviceName;
	info.driverVersion = id.DriverVersion.QuadPart;
	info.vendorID = id.VendorId;
	info.deviceID = id.DeviceId;
	info.subSystemID = id.SubSysId;
	info.revision = id.Revision;
	info.deviceIdentifier = *reinterpret_cast<AdapterGuid*>( &id.DeviceIdentifier );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMonitor( unsigned adapterIndex,
												 void*& monitor )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	monitor = s_direct3D->GetAdapterMonitor( adapterIndex );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterDisplayMode( unsigned adapterIndex,
													 Tr2DisplayModeInfo& mode )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	D3DDISPLAYMODE id;
	CR_RETURN_HR( s_direct3D->GetAdapterDisplayMode( adapterIndex, &id ) );
	mode.width = id.Width;
	mode.height = id.Height;
	mode.refreshRateNumerator = 1;
	mode.refreshRateDenominator = id.RefreshRate;
	mode.format = Tr2RenderContextAL::ConvertFromD3D9Format( id.Format );
	mode.scanlineOrdering = SCANLINE_ORDER_UNSPECIFIED;
	mode.scaling = DISPLAY_SCALING_UNSPECIFIED;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterModeCount( unsigned adapterIndex,
												   Tr2RenderContextEnum::PixelFormat backBufferFormat,
												   unsigned& count )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}

	D3DFORMAT format = Tr2RenderContextAL::ConvertToD3D9Format( backBufferFormat );

	static const D3DFORMAT allowed[] =
	{
		D3DFMT_X8R8G8B8,
		D3DFMT_A8R8G8B8,
		D3DFMT_A2R10G10B10,
		D3DFMT_X1R5G5B5,
		D3DFMT_A1R5G5B5,
		D3DFMT_R5G6B5
	};
	const int nAllowed = sizeof( allowed ) / sizeof( allowed[0] );
	int i;

	for ( i = 0; i < nAllowed; i++ )
	{
		if ( format == allowed[i] )
		{
			break;
		}
	}
	if ( i == nAllowed )
	{
		return E_FAIL;
	}

	count = s_direct3D->GetAdapterModeCount( adapterIndex, format );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMode( unsigned adapterIndex,
											  Tr2RenderContextEnum::PixelFormat backBufferFormat,
											  unsigned modeIndex,
											  Tr2DisplayModeInfo& mode )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	D3DDISPLAYMODE id;
	CR_RETURN_HR( s_direct3D->EnumAdapterModes( adapterIndex, Tr2RenderContextAL::ConvertToD3D9Format( backBufferFormat ), modeIndex, &id ) );
	mode.width = id.Width;
	mode.height = id.Height;
	mode.refreshRateNumerator = 1;
	mode.refreshRateDenominator = id.RefreshRate;
	mode.format = Tr2RenderContextAL::ConvertFromD3D9Format( id.Format );
	mode.scanlineOrdering = SCANLINE_ORDER_UNSPECIFIED;
	mode.scaling = DISPLAY_SCALING_UNSPECIFIED;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterShaderVersion( unsigned adapterIndex,
													   unsigned& version )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	D3DCAPS9 caps;
	CR_RETURN_HR( s_direct3D->GetDeviceCaps( adapterIndex, D3DDEVTYPE_HAL, &caps ) );
	version = std::min( caps.VertexShaderVersion, caps.PixelShaderVersion );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMaxTextureWidth( unsigned adapterIndex,
														 unsigned& maxWidth )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	D3DCAPS9 caps;
	CR_RETURN_HR( s_direct3D->GetDeviceCaps( adapterIndex, D3DDEVTYPE_HAL, &caps ) );
	maxWidth = caps.MaxTextureWidth;
	return S_OK;
}

bool Tr2VideoAdapterInfo::SupportsBackBufferFormat( unsigned adapterIndex,
													Tr2RenderContextEnum::PixelFormat backBufferFormat,
													bool windowed )
{
	if ( !s_direct3D && !SUCCEEDED( InitializeDirect3D() ) )
	{
		return false;
	}
	D3DFORMAT bbFormat = Tr2RenderContextAL::ConvertToD3D9Format( backBufferFormat );
	D3DFORMAT displayFormat;
	switch ( bbFormat )
	{
	case D3DFMT_A8R8G8B8:
		displayFormat = D3DFMT_X8R8G8B8;
		break;
	case D3DFMT_A1R5G5B5:
		displayFormat = D3DFMT_X1R5G5B5;
		break;
	default:
		displayFormat = bbFormat;
	}
	return SUCCEEDED( s_direct3D->CheckDeviceType( adapterIndex,
		D3DDEVTYPE_HAL,
		displayFormat,
		bbFormat,
		windowed ? TRUE : FALSE ) );
}

bool Tr2VideoAdapterInfo::SupportsRenderTargetFormat( unsigned adapterIndex,
													  Tr2RenderContextEnum::PixelFormat backBufferFormat,
													  Tr2RenderContextEnum::PixelFormat format,
													  bool withAutoGenMipmap )
{
	if ( !s_direct3D && !SUCCEEDED( InitializeDirect3D() ) )
	{
		return false;
	}
	return SUCCEEDED( s_direct3D->CheckDeviceFormat( adapterIndex,
		D3DDEVTYPE_HAL,
		Tr2RenderContextAL::ConvertToD3D9Format( backBufferFormat ),
		D3DUSAGE_RENDERTARGET | ( withAutoGenMipmap ? D3DUSAGE_AUTOGENMIPMAP : 0 ),
		D3DRTYPE_SURFACE,
		Tr2RenderContextAL::ConvertToD3D9Format( format ) ) );
}

bool Tr2VideoAdapterInfo::SupportsDepthStencilFormat( unsigned adapterIndex,
													  Tr2RenderContextEnum::PixelFormat backBufferFormat,
													  Tr2RenderContextEnum::DepthStencilFormat format )
{
	if ( !s_direct3D && !SUCCEEDED( InitializeDirect3D() ) )
	{
		return false;
	}
	D3DFORMAT format9 = Tr2RenderContextEnum::ConvertToD3D9DepthStencilFormat( format );
	return SUCCEEDED( s_direct3D->CheckDeviceFormat( adapterIndex,
		D3DDEVTYPE_HAL,
		Tr2RenderContextAL::ConvertToD3D9Format( backBufferFormat ),
		D3DUSAGE_DEPTHSTENCIL,
		D3DRTYPE_SURFACE,
		format9 ) );
}

bool Tr2VideoAdapterInfo::SupportsVertexTextureFormat( unsigned adapterIndex,
													   Tr2RenderContextEnum::PixelFormat backBufferFormat,
													   Tr2RenderContextEnum::PixelFormat format )
{
	if ( !s_direct3D && !SUCCEEDED( InitializeDirect3D() ) )
	{
		return false;
	}
	return SUCCEEDED( s_direct3D->CheckDeviceFormat( adapterIndex,
		D3DDEVTYPE_HAL,
		Tr2RenderContextAL::ConvertToD3D9Format( backBufferFormat ),
		D3DUSAGE_QUERY_VERTEXTEXTURE,
		D3DRTYPE_SURFACE,
		Tr2RenderContextAL::ConvertToD3D9Format( format ) ) );
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
													 Tr2RenderContextEnum::PixelFormat format,
													 bool windowed,
													 unsigned msaaType,
													 unsigned& msaaQuality )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	const auto
	sample = static_cast<D3DMULTISAMPLE_TYPE>( msaaType > 1 ? msaaType : 0 );
	DWORD levels = 0;
	HRESULT hr = s_direct3D->CheckDeviceMultiSampleType( adapterIndex, D3DDEVTYPE_HAL,
		Tr2RenderContextAL::ConvertToD3D9Format( format ), windowed ? TRUE : FALSE, sample, &levels );
	if ( hr == D3DERR_NOTAVAILABLE )
	{
		levels = 0;
	}
	else if ( FAILED( hr ) )
	{
		return hr;
	}
	msaaQuality = levels;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
													 Tr2RenderContextEnum::DepthStencilFormat format,
													 bool windowed,
													 unsigned msaaType,
													 unsigned& msaaQuality )
{
	if ( !s_direct3D )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	D3DFORMAT format9 = Tr2RenderContextEnum::ConvertToD3D9DepthStencilFormat( format );
	const auto
	sample = static_cast<D3DMULTISAMPLE_TYPE>( msaaType > 1 ? msaaType : 0 );
	DWORD levels = 0;
	HRESULT hr = s_direct3D->CheckDeviceMultiSampleType( adapterIndex, D3DDEVTYPE_HAL, format9,
		windowed ? TRUE : FALSE, sample, &levels );
	if ( hr == D3DERR_NOTAVAILABLE )
	{
		levels = 0;
	}
	else if ( FAILED( hr ) )
	{
		return hr;
	}
	msaaQuality = levels;
	return S_OK;
}

bool Tr2VideoAdapterInfo::AreAdaptersDifferent( unsigned adapter1,
												unsigned adapter2 )
{
	return adapter1 != adapter2;
}

ALResult Tr2VideoAdapterInfo::RefreshData()
{
	return InitializeDirect3D();
}

IDirect3D9* Tr2VideoAdapterInfo::GetDirect3D()
{
	if ( !s_direct3D && !SUCCEEDED( InitializeDirect3D() ) )
	{
		return nullptr;
	}
	return s_direct3D;
}

#endif
