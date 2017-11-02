#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "Tr2RenderTargetALDx9.h"
#include "Tr2RenderContextDx9.h"

using namespace Tr2RenderContextEnum;

#include "ALLog.h"

#include "blue/include/TransGaming.h"

#pragma warning( disable: 4189 )	// Scopeguard

Tr2RenderTargetAL::Tr2RenderTargetAL()
	: m_sharedHandle( nullptr )
{
	Destroy();
	memset( &m_deviceLost, 0, sizeof( m_deviceLost ) );
}

void Tr2RenderTargetAL::Destroy()
{
	Tr2BitmapDimensions::Destroy();

	m_format9 = D3DFMT_UNKNOWN;
	m_msaa.samples = 0;
	m_msaa.quality = 0;
	m_flags = EX_NONE;
	
	m_mainRT = nullptr;
	m_mipGeneratedRT = nullptr;
	m_sysMemLocked = nullptr;
	m_msaaRT = nullptr;
	
	m_clearSysMemLockable = true;
	m_isAttached = false;
	
	if( m_sharedHandle )
	{
		::CloseHandle( m_sharedHandle );
		m_sharedHandle = nullptr;
	}
	
	m_backingStore.Destroy();
}

void Tr2RenderTargetAL::PrepareALResource( Tr2PrimaryRenderContextAL& renderContext )
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
				SUCCEEDED( Create(	d.m_width, d.m_height, d.m_mipCount, d.m_format, d.m_msaa, 0, d.m_flags, renderContext ) ) && 
				IsValid() )
			{
				m_deviceLost.m_valid = false;
			}
		}
	}
}

void Tr2RenderTargetAL::ReleaseALResource()
{
	if( !m_isAttached && !m_deviceLost.m_valid )
	{
		m_deviceLost.m_format = m_format;
		m_deviceLost.m_width = m_width;
		m_deviceLost.m_height = m_height;
		m_deviceLost.m_mipCount = m_mipCount;
		m_deviceLost.m_msaa = m_msaa;
		m_deviceLost.m_flags = m_flags;

		m_deviceLost.m_valid = true;
	}

	Destroy();
}

Tr2RenderTargetAL::~Tr2RenderTargetAL()
{
}

bool Tr2RenderTargetAL::IsValid() const
{
	if( m_msaaRT )
	{
		return true;
	}

	return m_mainRT != nullptr;
}

ALResult Tr2RenderTargetAL::Create(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format, 
	const Tr2MsaaDesc& msaa,
	Tr2RenderContextEnum::BufferUsage,
	Tr2RenderContextEnum::ExFlag flags,
	Tr2RenderContextAL& renderContext )
{
	Destroy();

	if( renderContext.m_d3dDevice9 == nullptr )
	{
		return E_FAIL;
	}
	
	m_format9 = renderContext.ConvertToD3D9Format( format );
	if( m_format9 == D3DFMT_UNKNOWN )
	{
		CCP_AL_LOGWARN( "Can't create a renderTarget with format %d on DX9", format );
		return E_INVALIDARG;
	}

	const bool shared = flags & EX_CREATE_SHARED;
	const auto msaaType = static_cast<D3DMULTISAMPLE_TYPE>( msaa.samples > 1 ? msaa.samples : 0 );
	
	if( shared )	// from the looks of it, shared previewer was always using an actual RT.  Still supporting shared textures too just in case we change our mind later (below)
	{		
		CComQIPtr<IDirect3DDevice9Ex> exDevice( renderContext.m_d3dDevice9 );
		if( exDevice )
		{
			CR_RETURN_HR( exDevice->CreateRenderTargetEx( 
				width, 
				height, 
				m_format9, 
				msaaType,
				msaa.quality, 
				true, 
				&m_msaaRT, 
				&m_sharedHandle, 
				D3DUSAGE_NONSECURE ) );
		}
		else
		{
			CR_RETURN_HR( renderContext.m_d3dDevice9->CreateRenderTarget( 
				width, 
				height, 
				m_format9, 
				msaaType, 
				msaa.quality, 
				true, 
				&m_msaaRT, 
				&m_sharedHandle ) );				
		}	
	}
	else if( msaaType > 1 )
	{
		CCP_ASSERT( mipLevelCount <= 1 );
		HRESULT hr = renderContext.m_d3dDevice9->CreateRenderTarget( 
			width, 
			height, 
			m_format9, 
			msaaType, 
			msaa.quality, 
			false, 
			&m_msaaRT, 
			nullptr );
		if( FAILED( hr ) )
		{
			CCP_AL_LOGWARN( 
					"Tr2RenderTargetAL::Create: CreateRenderTarget fails %d x %d, fmt %d, msaa %d %d, HR 0x%x", 
					width, height, m_format9, msaa.samples, msaa.quality, hr );
			return hr;
		}
		
		m_mipCount = 1;
	}
	else
	{
		uint32_t usage = D3DUSAGE_RENDERTARGET;
		uint32_t mipCount = 1;
		if( mipLevelCount != 1 )
		{
			usage |= D3DUSAGE_AUTOGENMIPMAP;
			mipCount = 0;
		}

		//TODO do we need these still?
		//trinity.TRIUSAGE_DYNAMIC if trinity.device.UsingEXDevice() else 0, 
        //trinity.TRIPOOL_DEFAULT if trinity.device.UsingEXDevice() else trinity.TRIPOOL_MANAGED )

		HRESULT hr = renderContext.m_d3dDevice9->CreateTexture(	
			width, 
			height, 
			mipCount, 
			usage, 
			m_format9, 
			D3DPOOL_DEFAULT, 
			&m_mainRT, 
			shared ? &m_sharedHandle : nullptr );
		if( FAILED( hr ) )
		{
			CCP_AL_LOGWARN( 
					"Tr2RenderTargetAL::CreateEx: CreateTexture(1) fails %d x %d x %d, usage %d, fmt %d, HR 0x%x", 
					width, height, mipCount, usage, m_format9, hr );
			return hr;
		}
		CR_RETURN_HR( hr );
			
		if( hr == D3DOK_NOAUTOGEN )
		{
			// We need to manually generate mip-levels. Unfortunately, many video cards (nVidia cards in
			// particular) seem to fail miserably when using StretcRect from one mip-level surface to the
			// next so we have to set up a separate render target texture for generating the mip-levels.
			// Using StretchRect between surface levels from different textures seems to work fine, though,
			// but it means we're wasting an extra top-level surface.
			hr = renderContext.m_d3dDevice9->CreateTexture( 
										std::max( width  / 2, 1u ), 
										std::max( height / 2, 1u ), 
										1, 
										D3DUSAGE_RENDERTARGET, 
										m_format9, 
										D3DPOOL_DEFAULT, 
										&m_mipGeneratedRT, 
										shared ? &m_sharedHandle : nullptr );
			if( FAILED( hr ) )
			{
				CCP_AL_LOGWARN( 
					"Tr2RenderTargetAL::CreateEx: CreateTexture(2) fails %d x %d x %d, usage %d, fmt %d, HR 0x%x", 
					std::max( width / 2, 1u ), std::max( height / 2, 1u ), 
					1, D3DUSAGE_RENDERTARGET, m_format9, hr );
				m_mainRT = nullptr;
				return hr;
			}
		}

		m_mipCount		= mipCount;
	}

	m_format = format;
	m_width = width;
	m_height = height;
	m_msaa = msaa;
	m_flags = flags;

	if( !m_msaaRT )
	{
		m_backingStore.m_format		= m_format;
		m_backingStore.m_usage		= USAGE_IMMUTABLE;
		m_backingStore.m_type		= TEX_TYPE_2D;
		m_backingStore.m_width		= m_width;
		m_backingStore.m_height		= m_height;
		m_backingStore.m_volumeDepth= 1;
		m_backingStore.m_mipCount	= m_mipCount;
		m_backingStore.m_texture	= m_mainRT;
		m_backingStore.m_isAlias	= true;
	}
	ChangeObjectId();
	
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Attaches render target to existing surface. This method is platform-specific and
//   should only be used from AL classes. It is used by Tr2RenderContextAL and 
//   Tr2SwapChainAL to attach render target to a back buffer.
// Arguments:
//   surface - Existing render target surface
//   renderContext - Current render context
// Return value:
//   HRESULT value
// --------------------------------------------------------------------------------------
ALResult Tr2RenderTargetAL::Attach( IDirect3DSurface9* surface, Tr2RenderContextAL& /*renderContext*/ )
{
	Destroy();

	if( surface == nullptr )
	{
		return E_INVALIDARG;
	}

	D3DSURFACE_DESC desc;
	CR_RETURN_HR( surface->GetDesc( &desc ) );

	m_format = Tr2RenderContextAL::ConvertFromD3D9Format( desc.Format );
	m_width = desc.Width;
	m_height = desc.Height;
	m_mipCount = 1;
	m_msaa.samples = desc.MultiSampleType;
	m_msaa.quality = desc.MultiSampleQuality;
	m_flags = EX_NONE;
	m_format9 = desc.Format;

	m_msaaRT = surface;

	m_isAttached = true;	// Don't try to auto-recreate on device lost
	ChangeObjectId();

	return S_OK;
}

ALResult Tr2RenderTargetAL::Resolve( Tr2RenderTargetAL& destination, Tr2RenderContextAL& renderContext )
{
	CComPtr<IDirect3DSurface9>	dst;

	if( !IsValid() )
	{
		return S_OK;
	}
	
	if( destination.m_msaaRT )	// can happen when we go to the backbuffer, ie. destination is wrapping an 'orphaned' Surface9
	{
		dst = destination.m_msaaRT;
	}
	else
	{
		if( !destination.IsValid()  )
		{
			return S_OK;
		}
	
		if( !destination.m_mainRT )
		{
			return S_OK;
		}

		destination.m_mainRT->GetSurfaceLevel( 0, &dst );
	}
	if( !dst )
	{
		return S_OK;
	}

	CComPtr<IDirect3DSurface9>	src;
	if( m_msaaRT )
	{
		src = m_msaaRT;
	}
	else
	{
		CCP_ASSERT( m_mainRT );
		if( !m_mainRT )
		{
			return S_OK;
		}
		m_mainRT->GetSurfaceLevel( 0, &src );
	}
	if( !src )
	{
		return S_OK;
	}

		
	D3DSURFACE_DESC srcDesc, dstDesc;
	CR_RETURN_HR( src->GetDesc( &srcDesc ) );
	CR_RETURN_HR( dst->GetDesc( &dstDesc ) );

	
	if( dstDesc.Pool != D3DPOOL_DEFAULT || srcDesc.Pool != D3DPOOL_DEFAULT )
	{
		CCP_AL_LOGERR( "Tr2RenderTargetAL::Resolve: target and source should be in the default pool!" );
		return E_FAIL;
	}

//	RECT srcRect;
	RECT* srcRectPtr = NULL;
	/* unused feature?
	if( m_sourceViewport )
	{
		srcRect.top = m_sourceViewport->y;
		srcRect.left = m_sourceViewport->x;
		srcRect.bottom = m_sourceViewport->y + m_sourceViewport->height;
		srcRect.right = m_sourceViewport->x + m_sourceViewport->width;
		if( srcRect.top < 0 )
		{
			srcRect.top = 0;
		}
		if( srcRect.left < 0 )
		{
			srcRect.left = 0;
		}
		if( srcRect.bottom > (LONG)srcDesc.Height )
		{
			srcRect.bottom = srcDesc.Height;
		}
		if( srcRect.right > (LONG)srcDesc.Width )
		{
			srcRect.right = srcDesc.Width;
		}
		srcRectPtr = &srcRect;
	}
	*/

//	RECT destRect;
	RECT* destRectPtr = NULL;
	/* unused feature?
	if( m_destinationViewport )
	{
		destRect.top = m_destinationViewport->y;
		destRect.left = m_destinationViewport->x;
		destRect.bottom = m_destinationViewport->y + m_destinationViewport->height;
		destRect.right = m_destinationViewport->x + m_destinationViewport->width;
		if( destRect.top < 0 )
		{
			destRect.top = 0;
		}
		if( destRect.left < 0 )
		{
			destRect.left = 0;
		}
		if( destRect.bottom > (LONG)dstDesc.Height )
		{
			destRect.bottom = dstDesc.Height;
		}
		if( destRect.right > (LONG)dstDesc.Width )
		{
			destRect.right = dstDesc.Width;
		}
		destRectPtr = &destRect;
	}
	*/

	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	AL_UPDATE_RESOURCE_FRAME_USAGE( destination );
	HRESULT hr = renderContext.m_d3dDevice9->StretchRect( src, srcRectPtr, dst, destRectPtr, D3DTEXF_POINT );
	
	if( !SUCCEEDED( hr ) )
	{
		CCP_AL_LOGERR( "Tr2RenderTargetAL::Resolve - StretchRect failed!" );
		return hr;
	}

	return S_OK;
}

bool Tr2RenderTargetAL::GetSurfaceForRT( CComPtr<IDirect3DSurface9>	&surf )
{
	if( m_msaaRT )
	{
		surf = m_msaaRT;
	}
	else
	{
		CCP_ASSERT( m_mainRT );
		if( !m_mainRT )
		{
			return false;
		}
		m_mainRT->GetSurfaceLevel( 0, &surf );
	}
	if( !surf )
	{
		return false;
	}
	return true;
}

ALResult Tr2RenderTargetAL::CopySubresourceRegion(	
	uint32_t destX, 
	uint32_t destY, 
	Tr2RenderTargetAL& source, 
	uint32_t* ltrb, 
	Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return S_OK;
	}

	CComPtr<IDirect3DSurface9>	src;
	CComPtr<IDirect3DSurface9>	dst;
	
	if( !source.GetSurfaceForRT( src ) ||
		!this-> GetSurfaceForRT( dst ) )
	{
		return S_OK;
	}
	
	D3DSURFACE_DESC srcDesc, dstDesc;
	CR_RETURN_HR( src->GetDesc( &srcDesc ) );
	CR_RETURN_HR( dst->GetDesc( &dstDesc ) );
		
	if( dstDesc.Pool != D3DPOOL_DEFAULT || srcDesc.Pool != D3DPOOL_DEFAULT )
	{
		CCP_AL_LOGERR( "Tr2RenderTargetAL::CopySubresourceRegion: target and source should be in the default pool!" );
		return E_INVALIDARG;
	}

	RECT srcRect;
	RECT* srcRectPtr = nullptr;
	int32_t srcWidth, srcHeight;
	if( ltrb )
	{
		srcRect.left	= ltrb[0];
		srcRect.top		= ltrb[1];
		srcRect.right	= ltrb[2];
		srcRect.bottom	= ltrb[3];
		
		if( srcRect.bottom > (LONG)srcDesc.Height )
		{
			srcRect.bottom = srcDesc.Height;
		}
		if( srcRect.right > (LONG)srcDesc.Width )
		{
			srcRect.right = srcDesc.Width;
		}
		srcRectPtr = &srcRect;

		srcWidth = srcRect.right - srcRect.left;
		srcHeight = srcRect.bottom - srcRect.top;
	}
	else
	{
		srcWidth = srcDesc.Width;
		srcHeight = srcDesc.Height;
	}
	
	RECT destRect;	
	destRect.left	= destX;
	destRect.top	= destY;
	destRect.right	= destX + srcWidth;
	destRect.bottom	= destY + srcHeight;
		
	if( destRect.bottom > (LONG)dstDesc.Height )
	{
		destRect.bottom = dstDesc.Height;
	}
	if( destRect.right > (LONG)dstDesc.Width )
	{
		destRect.right = dstDesc.Width;
	}
	
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	AL_UPDATE_RESOURCE_FRAME_USAGE( source );
	HRESULT hr = renderContext.m_d3dDevice9->StretchRect( src, srcRectPtr, dst, &destRect, D3DTEXF_POINT );
	
	if( !SUCCEEDED( hr ) )
	{
		CCP_AL_LOGERR( "Tr2RenderTargetAL::CopySubresourceRegion - StretchRect failed!" );
		return hr;
	}

	return S_OK;
}

ALResult Tr2RenderTargetAL::GenerateMipMaps( Tr2RenderContextAL& renderContext )
{
	if( m_mipCount == 1 || !IsValid() )
	{
		return S_OK;
	}
	if( !m_mipGeneratedRT )
	{
		m_mainRT->GenerateMipSubLevels();
		return S_OK;
	}


	// Some (most nVidia, it seems) video cards can't do a StretchRect from one mip-level
	// surface to the next within the same texture. We therefore have to copy (with filtering)
	// from the m_mainRT texture to the m_mipGeneratedRT, then copy those filtered results
	// back to use as source for the next level down.

	const uint32_t levelCount = m_mainRT->GetLevelCount();

	CComPtr<IDirect3DSurface9> srcLevel;
	CComPtr<IDirect3DSurface9> dstLevel;
	CComPtr<IDirect3DSurface9> scratch;

	RECT srcRect = { 0, 0, m_width, m_height };
	RECT dstRect = { 0, 0, std::max( m_width / 2, 1u ), std::max( m_height / 2, 1u ) };

	CR_RETURN_HR( m_mipGeneratedRT->GetSurfaceLevel( 0, &scratch ) );
	CR_RETURN_HR( m_mainRT->GetSurfaceLevel( 0, &srcLevel ) );

	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );

	for( uint32_t levelIx = 1; levelIx < levelCount; ++levelIx )
	{
		CR( renderContext.m_d3dDevice9->StretchRect( srcLevel, &srcRect, scratch, &dstRect, D3DTEXF_LINEAR ) );
		CR_RETURN_HR( m_mainRT->GetSurfaceLevel( levelIx, &dstLevel ) );
		CR( renderContext.m_d3dDevice9->StretchRect( scratch, &dstRect, dstLevel, &dstRect, D3DTEXF_NONE ) );
		srcLevel = dstLevel;

		srcRect = dstRect;
		dstRect.right = std::max( dstRect.right / 2, 1l );
		dstRect.bottom = std::max( dstRect.bottom / 2, 1l );
	}

	return S_OK;
}

ALResult Tr2RenderTargetAL::Bind( uint32_t slot, Tr2RenderContextAL& renderContext ) const
{
	if( !renderContext.m_d3dDevice9 )
	{
		return E_FAIL;
	}

	if( this == &nullRT && slot > 0 )
	{
		return renderContext.m_d3dDevice9->SetRenderTarget( slot, nullptr );
	}

	if( m_msaaRT )
	{
		return renderContext.m_d3dDevice9->SetRenderTarget( slot, m_msaaRT );
	}

	if( !m_mainRT )
	{
		return E_FAIL;
	}

	CComPtr<IDirect3DSurface9>			m_boundRT;	
	HRESULT hr = m_mainRT->GetSurfaceLevel( 0, &m_boundRT );
	if( SUCCEEDED( hr ) )
	{
		hr = renderContext.m_d3dDevice9->SetRenderTarget( slot, m_boundRT );
	}
	return hr;
}

Tr2TextureAL& Tr2RenderTargetAL::GetTexture()
{ 
	return m_backingStore; 
}

const Tr2TextureAL& Tr2RenderTargetAL::GetTexture() const
{ 
	return m_backingStore; 
}

ALResult Tr2RenderTargetAL::GetRenderTargetData( uint32_t mipLevel, CComPtr<IDirect3DSurface9> &sysMem, Tr2RenderContextAL& renderContext )
{
	const uint32_t width  = GetMipWidth( mipLevel );
	const uint32_t height = GetMipHeight( mipLevel );

	auto& srcRT = m_mainRT;
	auto& dev = renderContext.m_d3dDevice9;

	const bool isAutoGen = m_mipCount == 0 && !m_mipGeneratedRT;


	if( sysMem )
	{
		D3DSURFACE_DESC desc;
		if( FAILED( sysMem->GetDesc( &desc ) ) || desc.Width != width || desc.Height != height || desc.Format != m_format9 )
		{
			sysMem = nullptr;
		}
	}

	if( !sysMem )
	{
		CR_RETURN_HR( dev->CreateOffscreenPlainSurface( width, height, m_format9, D3DPOOL_SYSTEMMEM, &sysMem, nullptr ) );
		if( !sysMem )
		{
			return E_FAIL;
		}
	}

	if( m_msaaRT )
	{
		CCP_AL_LOG("Tr2RenderTargetAL::Lock getting render target data ");
		CR_RETURN_HR( dev->GetRenderTargetData( m_msaaRT, sysMem ) );
	}
	else if( isAutoGen && mipLevel > 0 )
	{
		// Need to go the long route of first rendering the renderTarget into a new, non-mipmapped renderTarget, and then
		// getting the data out of that second RT... :|

		CComPtr<IDirect3DTexture9>	nonMipRT;
		CComPtr<IDirect3DSurface9>	nonMipSurface;
		{
			CComPtr<IDirect3DSurface9>	oldRT, oldDS;

			CR_RETURN_HR( dev->CreateTexture( width, height, 1, D3DUSAGE_RENDERTARGET, m_format9, D3DPOOL_DEFAULT, &nonMipRT, nullptr ) ) ;
			if( !nonMipRT )
			{
				return E_FAIL;
			}
			CR_RETURN_HR( nonMipRT->GetSurfaceLevel( 0, &nonMipSurface ) );

			renderContext.m_frameDelayedDX9Objects.push_back( CComPtr<IUnknown>( nonMipRT ) );	// workaround - if FXAA is on, bad things happen if we destroy the RT asap. So wait until end of frame.

			CR_RETURN_HR( renderContext.InternalBlit( nonMipSurface, m_backingStore.m_texture, width, height ) );
		}

		CR_RETURN_HR( dev->GetRenderTargetData( nonMipSurface, sysMem ) );
	}
	else
	{
		CComPtr<IDirect3DSurface9> RT;
		CR_RETURN_HR( m_mainRT->GetSurfaceLevel( mipLevel, &RT ) );
		CR_RETURN_HR( dev->GetRenderTargetData( RT, sysMem ) );
	}
	return S_OK;
}

ALResult Tr2RenderTargetAL::Lock(	
	uint32_t mipLevel, 
	uint32_t* ltrb, 
	void*& data, 
	uint32_t& pitch, 
	Tr2RenderContextAL& renderContext )
{
	CCP_ASSERT( !m_sysMemLocked );	// already locked?
	if( !renderContext.m_d3dDevice9 || m_sysMemLocked || ( m_msaaRT && m_msaa.samples >= 2 ) )
	{
		return E_FAIL;
	}

	m_sysMemLocked = nullptr;
	CR_RETURN_HR( GetRenderTargetData( mipLevel, m_sysMemLocked, renderContext ) );
				
	D3DLOCKED_RECT locked;
	if( ltrb )
	{
		RECT R = { ltrb[0], ltrb[1], ltrb[2], ltrb[3] };
		CR_RETURN_HR( m_sysMemLocked->LockRect( &locked, &R, D3DLOCK_READONLY ) );
	}
	else
	{
		CR_RETURN_HR( m_sysMemLocked->LockRect( &locked, NULL, D3DLOCK_READONLY ) );
	}
		
	data  = locked.pBits;
	pitch = locked.Pitch;

	return data ? S_OK : E_FAIL;
}

ALResult Tr2RenderTargetAL::Unlock( Tr2RenderContextAL& renderContext )
{
	CCP_ASSERT( m_sysMemLocked );	// properly locked?
	if( !renderContext.m_d3dDevice9 || !m_sysMemLocked || !m_sysMemLocked )
	{
		return E_FAIL;
	}

	CR_RETURN_HR( m_sysMemLocked->UnlockRect() );
	if( m_clearSysMemLockable )
	{
		m_sysMemLocked = nullptr;
	}

	return S_OK;
}


void Tr2RenderTargetAL::SetHintLockOften()
{
	m_clearSysMemLockable = false;
}

uintptr_t Tr2RenderTargetAL::GetSharedHandle() const
{
	return reinterpret_cast<uintptr_t>( m_sharedHandle );
}

bool Tr2RenderTargetAL::operator==( const Tr2RenderTargetAL& other ) const
{ 
	return m_mainRT == other.m_mainRT && m_msaaRT == other.m_msaaRT; 
}

Tr2ALMemoryType Tr2RenderTargetAL::GetMemoryClass() const 
{ 
	return AL_MEMORY_VIDEO; 
}

Tr2MsaaDesc Tr2RenderTargetAL::GetMsaaDesc() const
{
	return m_msaa;
}

#endif // ( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
