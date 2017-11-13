#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "Tr2DepthStencilALDx9.h"
#include "Tr2RenderContextDx9.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2DepthStencilAL::Tr2DepthStencilAL()
	: m_width( 0 )
	, m_height( 0 )
	, m_format( static_cast<DepthStencilFormat>( 0 ) )
	, m_exFlags( EX_NONE )
	, m_sharedHandle( nullptr )
{
}

Tr2DepthStencilAL::~Tr2DepthStencilAL()
{
}

ALResult Tr2DepthStencilAL::Create(	
	uint32_t width, 
	uint32_t height, 
	DepthStencilFormat dsFormat, 
	const Tr2MsaaDesc& msaa,
	Tr2RenderContextEnum::ExFlag flags,
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
	m_msaa = msaa;
	m_exFlags = flags;

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
	const auto sample = static_cast<D3DMULTISAMPLE_TYPE>( m_msaa.samples > 1 ? m_msaa.samples : 0 );
	
	if( !shared )
	{
		hr = renderContext.m_d3dDevice9->CreateDepthStencilSurface( width, height, format, sample, m_msaa.quality, /*Discard*/ TRUE, &m_depthStencil, nullptr );
	}
	else
	{
		CComQIPtr<IDirect3DDevice9Ex> exDevice( renderContext.m_d3dDevice9 );
		if( exDevice )
		{
			hr = exDevice->CreateDepthStencilSurfaceEx( width, height, format, sample, m_msaa.quality, /*Discard*/ TRUE, &m_depthStencil, &m_sharedHandle, D3DUSAGE_NONSECURE );			
		}
		else
		{
			hr = renderContext.m_d3dDevice9->CreateDepthStencilSurface( width, height, format, sample, m_msaa.quality, /*Discard*/ TRUE, &m_depthStencil, &m_sharedHandle );
		}
	}
	if( SUCCEEDED( hr ) )
	{
		m_memory.Set( Tr2MemoryCounterAL::TEXTURE, width, height, dsFormat, msaa );

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

		m_memory.Set( Tr2MemoryCounterAL::TEXTURE, m_width, m_height, m_format );

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
	m_memory.Reset();

	m_depthStencil		= nullptr;
	m_depthStencilREADABLE	= nullptr;
	m_backingStore		.Destroy();

	m_width = 0;
	m_height = 0;
	m_msaa.samples = 0;
	m_msaa.quality = 0;
	m_exFlags = EX_NONE;
	if( m_sharedHandle )
	{
		::CloseHandle( m_sharedHandle );
		m_sharedHandle = nullptr;
	}
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
	return reinterpret_cast<uintptr_t>( m_sharedHandle );
}

uint32_t Tr2DepthStencilAL::GetWidth() const
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
	return m_depthStencil == other.m_depthStencil && m_depthStencilREADABLE == other.m_depthStencilREADABLE;
}

Tr2ALMemoryType Tr2DepthStencilAL::GetMemoryClass() const 
{ 
	return AL_MEMORY_VIDEO; 
}

#endif
