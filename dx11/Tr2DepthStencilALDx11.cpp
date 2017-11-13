#include "StdAfx.h"

#include "Tr2DepthStencilALDx11.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "ALLog.h"
#include "Tr2PrimaryRenderContextDx11.h"

using namespace Tr2RenderContextEnum;

Tr2DepthStencilAL::Tr2DepthStencilAL()
	: m_width( 0 )
	, m_height( 0 )
	, m_format( static_cast<DepthStencilFormat>( 0 ) )
	, m_exFlag( EX_NONE )
{
}

Tr2DepthStencilAL::~Tr2DepthStencilAL()
{
}

bool Tr2DepthStencilAL::IsValid() const 
{ 
	return m_depthStencil != nullptr && m_depthStencilView != nullptr; 
}

DXGI_FORMAT	Tr2DepthStencilAL::ConvertDepthStencilFormatToDxgi( 
				Tr2RenderContextEnum::DepthStencilFormat dsFormat )
{
	// Note: when changing this, also change Create
	switch( dsFormat )
	{
	case DSFMT_D24S8:
	case DSFMT_D24X8:
	case DSFMT_AUTO:
	case DSFMT_READABLE:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
		
	case DSFMT_D24FS8:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;		// no DX11 DS24F?
		
	case DSFMT_D32:
		return DXGI_FORMAT_D32_FLOAT;
		
	case DSFMT_D32F:
		return DXGI_FORMAT_D32_FLOAT;
	}

	return DXGI_FORMAT_D24_UNORM_S8_UINT;
}

ALResult Tr2DepthStencilAL::Create( 
	uint32_t width, 
	uint32_t height, 
	DepthStencilFormat dsFormat, 
	const Tr2MsaaDesc& msaa,
	Tr2RenderContextEnum::ExFlag flags,
	Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();

	const bool shared = flags & EX_CREATE_SHARED;

	if( !renderContext.m_d3dDevice11 )
	{
		return E_FAIL;
	}

	m_width = width;
	m_height = height;
	m_format = dsFormat;
	m_msaa = msaa;
	m_exFlag = flags;

	D3D11_TEXTURE2D_DESC descDepth;
	memset( &descDepth, 0, sizeof( descDepth ) );

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	memset( &descDSV, 0, sizeof( descDSV ) );

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	memset( &srvDesc, 0, sizeof( srvDesc ) );

	PixelFormat pixelFormat;

	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	
	// Note: when changing this, also change ConvertDepthStencilFormatToDxgi.
	switch( dsFormat )
	{
	case DSFMT_D24S8:
	case DSFMT_D24X8:
	case DSFMT_AUTO:
	case DSFMT_READABLE:
		descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
		descDSV  .Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		srvDesc  .Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		pixelFormat		 = PIXEL_FORMAT_D24_UNORM_S8_UINT;
		break;

	case DSFMT_D24FS8:
		descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
		descDSV  .Format = DXGI_FORMAT_D24_UNORM_S8_UINT;		// no DX11 DS24F?
		srvDesc  .Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		pixelFormat		 = PIXEL_FORMAT_D24_UNORM_S8_UINT;
		break;

	case DSFMT_D32:
		descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
		descDSV  .Format = DXGI_FORMAT_D32_FLOAT;
		srvDesc  .Format = DXGI_FORMAT_R32_FLOAT;		// no D32 UNORM in DX11?
		pixelFormat	     = PIXEL_FORMAT_R32_FLOAT;
		break;

	case DSFMT_D32F:
		descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
		descDSV  .Format = DXGI_FORMAT_D32_FLOAT;
		srvDesc  .Format = DXGI_FORMAT_R32_FLOAT;
		pixelFormat	     = PIXEL_FORMAT_R32_FLOAT;
		break;

	default:
		CCP_AL_LOGERR( "Unsupported depth stencil format %d", dsFormat );
		return E_INVALIDARG;
	}

	bool enableShaderResourceView = true;
	
	descDepth.SampleDesc.Count = m_msaa.samples > 1 ? m_msaa.samples : 1;
	descDepth.SampleDesc.Quality = m_msaa.quality;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	if( shared )
	{
		descDepth.SampleDesc.Count = 1;
		descDepth.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
	}	

	HRESULT hr = renderContext.m_d3dDevice11->CreateTexture2D( &descDepth, nullptr, &m_depthStencil );
	if( FAILED( hr ) )
	{
		// Try again without the SRV part.
		descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		enableShaderResourceView = false;
		CR_RETURN_HR( renderContext.m_d3dDevice11->CreateTexture2D( &descDepth, nullptr, &m_depthStencil ) );		
	}

	descDSV.ViewDimension = m_msaa.samples <= 1 ? D3D11_DSV_DIMENSION_TEXTURE2D : D3D11_DSV_DIMENSION_TEXTURE2DMS;
	
	CR_RETURN_HR( renderContext.m_d3dDevice11->CreateDepthStencilView( m_depthStencil, &descDSV, &m_depthStencilView ) );
	
	descDSV.Flags |= D3D11_DSV_READ_ONLY_DEPTH;
	hr = renderContext.m_d3dDevice11->CreateDepthStencilView( m_depthStencil, &descDSV, &m_depthStencilViewReadOnly );
	if( FAILED( hr ) )
	{
		CCP_AL_LOGWARN( "Failed to create m_depthStencilViewReadOnly, no depth testing when texturing" );
	}

	// Always try to create an SRV, if it works then great we're texturable, if it doesn't then too bad.
	
	srvDesc.ViewDimension = msaa.samples > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	
	if( enableShaderResourceView )
	{
		if( SUCCEEDED( renderContext.m_d3dDevice11->CreateShaderResourceView( m_depthStencil, &srvDesc, &m_backingStore.m_view[0] ) ) )
		{
			m_backingStore.m_format = pixelFormat;
			m_backingStore.m_usage = USAGE_IMMUTABLE;	// prevent locking
			m_backingStore.m_type = TEX_TYPE_2D;
			m_backingStore.m_width = m_width;
			m_backingStore.m_height = m_height;
			m_backingStore.m_volumeDepth= 1;
			m_backingStore.m_mipCount = 1;
			m_backingStore.m_texture = m_depthStencil;
			m_backingStore.m_isAlias = true;

			m_backingStore.m_view[1] = m_backingStore.m_view[0];
		}
	}

	m_memory.Set( Tr2MemoryCounterAL::TEXTURE, width, height, dsFormat, msaa );

	ChangeObjectId();
	
	return S_OK;
}
	
void Tr2DepthStencilAL::Destroy()
{
	m_memory.Reset();
	m_depthStencil = nullptr;
	m_depthStencilView = nullptr;
	m_depthStencilViewReadOnly = nullptr;
	m_backingStore.Destroy();
	m_width = 0;
	m_height = 0;
	m_msaa.samples = 0;
	m_msaa.quality = 0;	
	m_exFlag = EX_NONE;
}

Tr2TextureAL& Tr2DepthStencilAL::GetTexture()
{
	return m_backingStore;
}

const Tr2TextureAL& Tr2DepthStencilAL::GetTexture() const
{
	return m_backingStore;
}

uintptr_t Tr2DepthStencilAL::GetSharedHandle() const
{
	IDXGIResource* pOtherResource( NULL );
	if( m_depthStencil == NULL )
	{
		CCP_AL_LOGERR( "GetSharedHandle: depthStencil is NULL" );
		return 0;
	}
	HRESULT HR = m_depthStencil->QueryInterface( __uuidof(IDXGIResource), (void**)&pOtherResource );
	if( FAILED( HR  ) )
	{
		CCP_AL_LOGERR( "GetSharedHandle: QueryInterface failed: 0x%x", HR );
		return 0;
	}
	else
	{
		HANDLE sharedHandle;
		HR = pOtherResource->GetSharedHandle( &sharedHandle );
		if( FAILED( HR ) )
		{
			CCP_AL_LOGERR( "GetSharedHandle: GetSharedHandle failed: 0x%x", HR );
			pOtherResource->Release();
			return 0;
		}
		pOtherResource->Release();
		return reinterpret_cast<uintptr_t>( sharedHandle );
	}
}

uint32_t Tr2DepthStencilAL::GetWidth()  const
{
	return m_width;
}

uint32_t Tr2DepthStencilAL::GetHeight() const
{
	return m_height;
}

const Tr2MsaaDesc& Tr2DepthStencilAL::GetMsaaDesc() const
{
	return m_msaa;
}

Tr2RenderContextEnum::DepthStencilFormat Tr2DepthStencilAL::GetFormat() const
{
	return m_format;
}

bool Tr2DepthStencilAL::operator==( const Tr2DepthStencilAL& other ) const 
{ 
	return m_depthStencil == other.m_depthStencil; 
}

Tr2ALMemoryType Tr2DepthStencilAL::GetMemoryClass() const 
{ 
	return AL_MEMORY_MANAGED; 
}

#endif
