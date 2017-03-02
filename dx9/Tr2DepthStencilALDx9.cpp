#include "StdAfx.h"
#include "Tr2DepthStencilALDx9.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2DepthStencilAL::Tr2DepthStencilAL()
	: m_width( 0 )
	, m_height( 0 )
	, m_format( static_cast<DepthStencilFormat>( 0 ) )
	, m_msaaType( 0 )
	, m_msaaQuality( 0 )
	, m_sharedHandle( nullptr )
{
	memset( &m_deviceLost, 0, sizeof( m_deviceLost ) );
}

Tr2DepthStencilAL::~Tr2DepthStencilAL()
{
}

ALResult Tr2DepthStencilAL::Create( 
	uint32_t width, 
	uint32_t height, 
	Tr2RenderContextEnum::DepthStencilFormat format, 
	uint32_t msaaType, 
	uint32_t msaaQuality, 
	Tr2RenderContextAL& renderContext )
{
	return CreateEx( width, height, format, msaaType, msaaQuality, 0, renderContext );
}

ALResult Tr2DepthStencilAL::CreateEx(	
	uint32_t width, 
	uint32_t height, 
	DepthStencilFormat dsFormat, 
	uint32_t msaaType, 
	uint32_t msaaQuality, 
	uint32_t flags, 
	Tr2RenderContextAL& renderContext )
{
	Destroy();

	if( !renderContext.m_d3dDevice9 )
	{
		return E_FAIL;
	}

	m_width  = width;
	m_height = height;
	m_format = dsFormat;
	m_msaaType = msaaType;
	m_msaaQuality = msaaQuality;

	if( dsFormat == DSFMT_READABLE )
	{
		return CreateReadableDepth( renderContext );
	}
	
	D3DFORMAT format;

#define CASE_(x)	case DSFMT_ ## x : format = D3DFMT_ ## x; break;

	if( dsFormat == DSFMT_AUTO )
	{
		dsFormat = renderContext.m_depthStencilFormat;
	}

	switch( dsFormat )
	{
	CASE_(D24S8)
	CASE_(D24X8)
	CASE_(D24FS8)
	
	//CASE_(D32F)
	case DSFMT_D32F: format = D3DFMT_D32F_LOCKABLE; break;	// no D32F available
	//CASE_(READABLE)
	case DSFMT_READABLE: format = (D3DFORMAT)MAKEFOURCC( 'I', 'N', 'T', 'Z' ); break;

	CASE_(D32)
	CASE_(D16_LOCKABLE)
	CASE_(D15S1)
	   CASE_(D24X4S4)
	CASE_(D16)
	CASE_(D32F_LOCKABLE)

	default:
		CCP_AL_LOGERR( "Unsupported depth stencil format %d", dsFormat );
		return E_INVALIDARG;
	}

	HRESULT hr;

	const bool shared = flags & EX_CREATE_SHARED;
	const auto sample = static_cast<D3DMULTISAMPLE_TYPE>( m_msaaType > 1 ? m_msaaType : 0 );
	
	if( !shared )
	{
		hr = renderContext.m_d3dDevice9->CreateDepthStencilSurface( width, height, format, sample, m_msaaQuality, /*Discard*/ TRUE, &m_depthStencil, nullptr );
	}
	else
	{
		CComQIPtr<IDirect3DDevice9Ex> exDevice( renderContext.m_d3dDevice9 );
		if( exDevice )
		{
			hr = exDevice->CreateDepthStencilSurfaceEx( width, height, format, sample, m_msaaQuality, /*Discard*/ TRUE, &m_depthStencil, &m_sharedHandle, D3DUSAGE_NONSECURE );			
		}
		else
		{
			hr = renderContext.m_d3dDevice9->CreateDepthStencilSurface( width, height, format, sample, m_msaaQuality, /*Discard*/ TRUE, &m_depthStencil, &m_sharedHandle );
		}
	}
	if( SUCCEEDED( hr ) )
	{
		ChangeObjectId();
	}

	return hr;
}

ALResult Tr2DepthStencilAL::CreateReadableDepth( Tr2RenderContextAL& renderContext )
{
	static const D3DFORMAT format = D3DFORMAT( MAKEFOURCC( 'I', 'N', 'T', 'Z' ) );
	
	HRESULT hr = renderContext.m_d3dDevice9->CreateTexture(	m_width, m_height, 1, D3DUSAGE_DEPTHSTENCIL, format, D3DPOOL_DEFAULT, &m_depthStencilREADABLE, nullptr );

	if( SUCCEEDED( hr ) )
	{
		m_backingStore.m_format		= PIXEL_FORMAT_R32_SINT;
		m_backingStore.m_usage		= USAGE_IMMUTABLE;	// prevent locking
		m_backingStore.m_type		= TEX_TYPE_2D;
		m_backingStore.m_width		= m_width;
		m_backingStore.m_height		= m_height;
		m_backingStore.m_volumeDepth= 1;
		m_backingStore.m_mipCount	= 1;
		m_backingStore.m_texture	= m_depthStencilREADABLE;
		m_backingStore.m_isAlias	= true;
		ChangeObjectId();
	}

	return hr;
}
	
bool Tr2DepthStencilAL::IsValid() const
{
	return m_depthStencil != nullptr || m_depthStencilREADABLE != nullptr;
}	

void Tr2DepthStencilAL::Destroy()
{
	m_depthStencil		= nullptr;
	m_depthStencilREADABLE	= nullptr;
	m_backingStore		.Destroy();

	m_width = 0;
	m_height = 0;
	m_msaaType = 0;
	m_msaaQuality = 0;
	if( m_sharedHandle )
	{
		::CloseHandle( m_sharedHandle );
		m_sharedHandle = nullptr;
	}
}

bool Tr2DepthStencilAL::IsReadable() const
{
	return m_format == DSFMT_READABLE && m_msaaType <= 1;
}

Tr2TextureAL& Tr2DepthStencilAL::GetTexture()
{
	//CCP_ASSERT( IsReadable() );
	// Doesn't work, might be accessed before it's created to set up a variable store.

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

			if( d.m_width > 0 && d.m_height > 0 &&
				SUCCEEDED( Create(	d.m_width, d.m_height, d.m_format, d.m_msaaType, d.m_msaaQuality, renderContext ) ) && 
				IsValid() )
			{
				m_deviceLost.m_valid = false;
			}
		}
	}
}

uint32_t Tr2DepthStencilAL::GetSharedHandle() const
{
	// TODO: 64bit
	// static_assert( sizeof( HANDLE ) == sizeof( unsigned ), "fix me for 64 bit" );
	return reinterpret_cast<uint32_t>( m_sharedHandle );
}

#endif
