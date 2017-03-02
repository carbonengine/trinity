#include "StdAfx.h"

#include "Tr2DepthStencilALDx11.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2DepthStencilAL::Tr2DepthStencilAL()
	: m_width( 0 )
	, m_height( 0 )
	, m_format( static_cast<DepthStencilFormat>( 0 ) )
	, m_msaaType( 0 )
	, m_msaaQuality( 0 )
{
	memset( &m_deviceLost, 0, sizeof( m_deviceLost ) );
}

Tr2DepthStencilAL::~Tr2DepthStencilAL()
{
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
	uint32_t msaaType, 
	uint32_t msaaQuality, 
	Tr2PrimaryRenderContextAL& renderContext )
{
	return CreateEx(width, height, dsFormat, msaaType, msaaQuality, 0, renderContext);
}

ALResult Tr2DepthStencilAL::CreateEx( 
	uint32_t width, 
	uint32_t height, 
	DepthStencilFormat dsFormat, 
	uint32_t msaaType, 
	uint32_t msaaQuality, 
	uint32_t flags, 
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
	m_msaaType = msaaType;
	m_msaaQuality = msaaQuality;

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
	
	descDepth.SampleDesc.Count = m_msaaType > 1 ? m_msaaType : 1;
	descDepth.SampleDesc.Quality = m_msaaQuality;
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

	descDSV.ViewDimension = m_msaaType <= 1 ? D3D11_DSV_DIMENSION_TEXTURE2D : D3D11_DSV_DIMENSION_TEXTURE2DMS;
	
	CR_RETURN_HR( renderContext.m_d3dDevice11->CreateDepthStencilView( m_depthStencil, &descDSV, &m_depthStencilView ) );
	
	descDSV.Flags |= D3D11_DSV_READ_ONLY_DEPTH;
	hr = renderContext.m_d3dDevice11->CreateDepthStencilView( m_depthStencil, &descDSV, &m_depthStencilViewReadOnly );
	if( FAILED( hr ) )
	{
		CCP_AL_LOGWARN( "Failed to create m_depthStencilViewReadOnly, no depth testing when texturing" );
	}

	// Always try to create an SRV, if it works then great we're texturable, if it doesn't then too bad.
	
	srvDesc.ViewDimension = msaaType > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	
	if( enableShaderResourceView )
	{
		if( SUCCEEDED( renderContext.m_d3dDevice11->CreateShaderResourceView( m_depthStencil, &srvDesc, &m_backingStore.m_view[0] ) ) )
		{
			m_backingStore.m_format		= pixelFormat;
			m_backingStore.m_usage		= USAGE_IMMUTABLE;	// prevent locking
			m_backingStore.m_type		= TEX_TYPE_2D;
			m_backingStore.m_width		= m_width;
			m_backingStore.m_height		= m_height;
			m_backingStore.m_volumeDepth= 1;
			m_backingStore.m_mipCount	= 1;
			m_backingStore.m_texture	= m_depthStencil;
			m_backingStore.m_isAlias	= true;

			m_backingStore.m_view[1]	= m_backingStore.m_view[0];
		}
	}
	ChangeObjectId();
	
	// If we get here it always worked -- don't return an error code for the failed SRV, that's just optional
	// (depthStencil that should be readable but isn't, returns S_OK -- up to client to look at IsReadable() and
	// change his mind if not what he wants).
	return S_OK;
}
	
void Tr2DepthStencilAL::Destroy()
{
	m_depthStencil = nullptr;
	m_depthStencilView = nullptr;
	m_depthStencilViewReadOnly = nullptr;
	m_backingStore.Destroy();
	m_width = 0;
	m_height = 0;
	m_msaaType = 0;
	m_msaaQuality = 0;	
}

// -------------------------------------------------------------
// Description:
//   Returns true if the depthstencil can be bound as a texture.
// -------------------------------------------------------------

bool Tr2DepthStencilAL::IsReadable() const
{
	return m_backingStore.IsValid() && m_msaaType <= 1;
}

Tr2TextureAL& Tr2DepthStencilAL::GetTexture()
{
	return m_backingStore;
}

const Tr2TextureAL& Tr2DepthStencilAL::GetTexture() const
{
	return m_backingStore;
}

void Tr2DepthStencilAL::ReleaseALResource()
{
	if( !m_deviceLost.m_valid )
	{
		m_deviceLost.m_format		= m_format;
		m_deviceLost.m_width		= m_width;
		m_deviceLost.m_height		= m_height;
		m_deviceLost.m_msaaType		= m_msaaType;
		m_deviceLost.m_msaaQuality	= m_msaaQuality;

		m_deviceLost.m_valid = true;
	}

	Destroy();
}

void Tr2DepthStencilAL::PrepareALResource( Tr2PrimaryRenderContextAL& renderContext )
{
	if( m_deviceLost.m_valid )
	{
		if( IsValid() )
		{
			m_deviceLost.m_valid = false;
		}
		else
		{
			const auto &d = m_deviceLost;

			if( d.m_width  > 0 && 
				d.m_height > 0 &&
				SUCCEEDED( Create(	d.m_width, 
									d.m_height, 
									d.m_format, 
									d.m_msaaType, 
									d.m_msaaQuality, 
									renderContext ) ) && 
				IsValid() )
			{
				m_deviceLost.m_valid = false;
			}
		}
	}
}

uint32_t Tr2DepthStencilAL::GetSharedHandle() const
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
		// TODO: 64bit
		// static_assert( sizeof( HANDLE ) == sizeof( unsigned ), "fix me for 64 bit" );
		return reinterpret_cast<uint32_t>( sharedHandle );
	}
}

#endif
