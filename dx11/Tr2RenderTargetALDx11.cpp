#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2RenderTargetALDx11.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2RenderTargetAL::Tr2RenderTargetAL()
{
	Destroy();
	memset( &m_deviceLost, 0, sizeof( m_deviceLost ) );
}

void Tr2RenderTargetAL::Destroy()
{
	Tr2BitmapDimensions::Destroy();
	
	m_msaaType		= 1;
	m_msaaQuality	= 0;
	
	m_texture		= nullptr;
	m_RTV			= nullptr;
	m_RTVsRgb = nullptr;

	m_isAttached	= false;

	m_backingStore.Destroy();
}

Tr2RenderTargetAL::~Tr2RenderTargetAL()
{
}

// -------------------------------------------------------------
// Description:
//	Create a non-MSAA renderTarget. It will not need a Resolve step; but if it can be bound as a texture
//  or not will depende on the format.
// -------------------------------------------------------------
ALResult Tr2RenderTargetAL::Create(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format,					
	Tr2PrimaryRenderContextAL &renderContext )
{
	return Create( width, height, mipLevelCount, format, 1, 0, renderContext );
}

// -------------------------------------------------------------
// Description:
//  Create a renderTarget that may have MSAA enabled (msaaType > 1).
//  MipLevelCount should always be zero or one at the current time.
//  Zero will one day generate mipmaps, but is not yet implemented.
// -------------------------------------------------------------
ALResult Tr2RenderTargetAL::Create(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format, 
	uint32_t msaaType, 
	uint32_t msaaQuality,
	Tr2PrimaryRenderContextAL &renderContext )
{
	return CreateEx( width, height, mipLevelCount, format, msaaType, msaaQuality, 0, 0, renderContext );
}

ALResult Tr2RenderTargetAL::CreateEx(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format, 
	uint32_t msaaType, 
	uint32_t msaaQuality,
	Tr2RenderContextEnum::BufferUsage usage,
	uint32_t flags,
	Tr2PrimaryRenderContextAL &renderContext )
{
	Destroy();

	if( !renderContext.m_d3dDevice11 )
	{
		return E_FAIL;
	}

	const bool shared = flags & EX_CREATE_SHARED;

	if( msaaType > 1 )
	{
		mipLevelCount = 1;
	}

	D3D11_TEXTURE2D_DESC desc;
	memset( &desc, 0, sizeof (desc) );
	desc.Width		= width;
	desc.Height		= height;
	desc.MipLevels	= mipLevelCount;
	desc.ArraySize  = 1;
	desc.Format     = static_cast<DXGI_FORMAT>( MakeTypeless( format ) );
	desc.BindFlags  = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;	//TODO test if this actualyl works with msaa.. else drop the SRV bit
	if( msaaType <= 1 && msaaQuality <= 1 && ( usage & USAGE_UNORDERED_ACCESS ) )
	{
		desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}
	if( !mipLevelCount )
	{
		desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}
	desc.SampleDesc.Count   = msaaType;
	desc.SampleDesc.Quality = msaaQuality;
	if( shared )
	{
		desc.SampleDesc.Count = 1;
		desc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
	}	
	
	HRESULT HR = renderContext.m_d3dDevice11->CreateTexture2D( &desc, nullptr, &m_texture );

	if( FAILED( HR ) )
	{
		return HR;
	}


	const bool isTexturable = true;//msaaType <= 1;	// else not texturable yet.. may change later when we allow subsample access in shaders.

	CComPtr<ID3D11ShaderResourceView>	view[2];

	if( isTexturable )
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		memset( &srvDesc, 0, sizeof( srvDesc ) );

		srvDesc.Format				= static_cast<DXGI_FORMAT>( format );
		srvDesc.ViewDimension		= msaaType > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;

		/*
		ID3D11Device::CreateShaderResourceView: The Dimensions of the View are invalid due to at least one of
		the following conditions. MostDetailedMip (value = 0) must be between 0 and MipLevels-1 of the Texture
		Resource, 9, inclusively. With the current MostDetailedMip, MipLevels (value = 0) must be between 1 and 10, 
		inclusively, or -1 to default to all mips from MostDetailedMip, in order that the View fit on the Texture. 
		[ STATE_CREATION ERROR #128: CREATESHADERRESOURCEVIEW_INVALIDDIMENSIONS ]
		*/
		srvDesc.Texture2D.MipLevels	= /*mipLevelCount*/ ~0u;
	
	
		for( unsigned loop = 0; loop != 2; ++loop )
		{
			HR = renderContext.m_d3dDevice11->CreateShaderResourceView( m_texture, &srvDesc, &view[loop] );
			if( FAILED( HR ) )
			{
				CCP_AL_LOGERR( "Create: CreateShaderResourceView failed: 0x%x", HR );
				return HR;
			}
			srvDesc.Format = static_cast<DXGI_FORMAT>( MakeSrgb( format ) );
		}
	}
	
	
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	memset( &rtvDesc, 0, sizeof( rtvDesc ) );

	rtvDesc.Format	= static_cast<DXGI_FORMAT>( format );
	rtvDesc.ViewDimension = msaaType > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	
	HR = renderContext.m_d3dDevice11->CreateRenderTargetView( m_texture, &rtvDesc, &m_RTV );
	if( FAILED( HR ) )
	{
		CCP_AL_LOGERR( "Create: CreateRenderTargetView failed: 0x%x", HR );
		return HR;
	}

	rtvDesc.Format	= static_cast<DXGI_FORMAT>( MakeSrgb( format ) );
	HR = renderContext.m_d3dDevice11->CreateRenderTargetView( m_texture, &rtvDesc, &m_RTVsRgb );
	if( FAILED( HR ) )
	{
		CCP_AL_LOGERR( "Create: CreateRenderTargetView failed: 0x%x", HR );
		return HR;
	}

	CComPtr<ID3D11UnorderedAccessView> uav;
	if( msaaType <= 1 && msaaQuality <= 1 && ( usage & USAGE_UNORDERED_ACCESS ) )
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV;
		descUAV.Format = static_cast<DXGI_FORMAT>( format );
		descUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		descUAV.Texture2D.MipSlice = 0;
		CR_RETURN_HR( renderContext.m_d3dDevice11->CreateUnorderedAccessView( m_texture, &descUAV, &uav ) );
	}
	
	m_format		= format;
	m_width			= width;
	m_height		= height;
	m_volumeDepth	= 1;
	m_type			= TEX_TYPE_2D;
	m_mipCount		= mipLevelCount;

	m_msaaType		= msaaType;
	m_msaaQuality	= msaaQuality;
	
	if( isTexturable )
	{
		m_backingStore.m_format		= m_format;
		m_backingStore.m_usage		= USAGE_IMMUTABLE;
		m_backingStore.m_type		= TEX_TYPE_2D;
		m_backingStore.m_width		= m_width;
		m_backingStore.m_height		= m_height;
		m_backingStore.m_volumeDepth= 1;
		m_backingStore.m_mipCount	= m_mipCount;
		m_backingStore.m_texture	= m_texture;
		m_backingStore.m_isAlias	= true;
		m_backingStore.m_view[0].Attach( view[0].Detach() );
		m_backingStore.m_view[1].Attach( view[1].Detach() );
		m_backingStore.m_uav = uav;
	}
	ChangeObjectId();
	
	return HR;
}

// --------------------------------------------------------------------------------------
// Description:
//   Attaches render target to existing texture. This method is platform-specific and
//   should only be used from AL classes. It is used by Tr2RenderContextAL and 
//   Tr2SwapChainAL to attach render target to a back buffer.
// Arguments:
//   texture - Existing render target texture
//   renderContext - Current render context
// Return value:
//   HRESULT value
// --------------------------------------------------------------------------------------
ALResult Tr2RenderTargetAL::Attach( ID3D11Texture2D* texture, Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();

	if( texture == nullptr || !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	D3D11_TEXTURE2D_DESC desc;
	texture->GetDesc( &desc );

	m_format		= static_cast<PixelFormat>( desc.Format );
	m_width			= desc.Width;
	m_height		= desc.Height;
	m_mipCount		= desc.MipLevels;
	m_msaaType		= desc.SampleDesc.Count;
	m_msaaQuality	= desc.SampleDesc.Quality;
	m_texture		= texture;

	D3D11_RENDER_TARGET_VIEW_DESC rtvd;
	memset (&rtvd, 0, sizeof (rtvd));
	rtvd.Format			= desc.Format;
	rtvd.ViewDimension	= desc.SampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
	CR_RETURN_HR( renderContext.m_d3dDevice11->CreateRenderTargetView( texture, &rtvd, &m_RTV ) );

	rtvd.Format	= static_cast<DXGI_FORMAT>( MakeSrgb( m_format ) );
	CR_RETURN_HR( renderContext.m_d3dDevice11->CreateRenderTargetView( texture, &rtvd, &m_RTVsRgb ) );

	auto& bb = m_backingStore;
	bb.m_format		= m_format;
	bb.m_usage		= USAGE_IMMUTABLE;
	bb.m_type		= TEX_TYPE_2D;
	bb.m_width		= m_width;
	bb.m_height		= m_height;
	bb.m_volumeDepth= 1;
	bb.m_mipCount	= m_mipCount;
	bb.m_isAlias	= true;

	bb.m_texture = texture;

	m_isAttached = true;	// Don't try to auto-recreate on device lost
	ChangeObjectId();

	return S_OK;
}

// -------------------------------------------------------------
// Description:
//  Resolve this renderTarget into destination.
//  The current renderTarget does not need to have MSAA enabled; if not, the resolve will just be
//  a straight copy (possibly asynchronous on the GPU).
//  This renderTarget and destination must have exactly the same dimensions and pixelformat.
//  For pixel format conversions, you must use a full screen quad instead (and the renderTarget that
//  you're converting from must therefor be texturable).
// -------------------------------------------------------------
ALResult Tr2RenderTargetAL::Resolve( Tr2RenderTargetAL& destination, Tr2RenderContextAL& renderContext )
{
	if( m_msaaType <= 1 )
	{
		return destination.GetTexture().CopySubresourceRegion( Tr2TextureSubresource(), GetTexture(), Tr2TextureSubresource(), renderContext );
	}

	CCP_ASSERT( destination.m_msaaType <= 1 );
	CCP_ASSERT( GetWidth() == destination.GetWidth() );
	CCP_ASSERT( GetHeight() == destination.GetHeight() );
	CCP_ASSERT( GetFormat() == destination.GetFormat() );
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	AL_UPDATE_RESOURCE_FRAME_USAGE( destination );
	renderContext.m_context->ResolveSubresource( destination.m_texture, 0, m_texture, 0, static_cast<DXGI_FORMAT>( m_format ) );

	return S_OK;
}

// -------------------------------------------------------------
// Description:
//  Copy part of the renderTarget into another with exactly the same format and MSAA qualities.
//  This can be used to extract part of a renderTarget into something smaller prior to resolving.
//  You cannot use this call to resolve -- an extra resolve is necessary on the destination into
//  a third plain target.
// -------------------------------------------------------------
ALResult Tr2RenderTargetAL::CopySubresourceRegion( 
	uint32_t destX, 
	uint32_t destY, 
	Tr2RenderTargetAL& source, 
	uint32_t* ltrb, 
	Tr2RenderContextAL& renderContext )
{
	CCP_ASSERT( IsValid() && source.IsValid() );
	CCP_ASSERT( std::max( m_msaaType, 1u ) == std::max( source.m_msaaType, 1u ) );
	CCP_ASSERT( GetFormat() == source.GetFormat() );
	CCP_ASSERT( m_msaaQuality == source.m_msaaQuality );
	return Tr2TextureAL::CopySubresourceRegion( m_texture, destX, destY, source.m_texture, ltrb, renderContext );
}

// -------------------------------------------------------------
// Description:
//  Generate mipmaps. May or may not actually do anything useful at the moment.
// -------------------------------------------------------------
ALResult Tr2RenderTargetAL::GenerateMipMaps( Tr2RenderContextAL& renderContext )
{
	if( m_mipCount != 1 && m_backingStore.m_view[0] )
	{
		AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
		renderContext.m_context->GenerateMips( m_backingStore.m_view[0] );
	}
	return S_OK;
}

// -------------------------------------------------------------
// Description:
//  Return the texture that backs up the renderTarget. It may not be IsValid() -- as is the case
//  when the renderTarget uses MSAA.  In that case, trying to bind it anyway is an error.
// -------------------------------------------------------------
Tr2TextureAL& Tr2RenderTargetAL::GetTexture()
{ 
	return m_backingStore; 
}

// -------------------------------------------------------------
// Description:
//  Return the texture that backs up the renderTarget. It may not be IsValid() -- as is the case
//  when the renderTarget uses MSAA.  In that case, trying to bind it anyway is an error.
// -------------------------------------------------------------
const Tr2TextureAL& Tr2RenderTargetAL::GetTexture() const
{ 
	return m_backingStore; 
}

ALResult Tr2RenderTargetAL::GetLockedRenderTarget( uint32_t mipLevel, uint32_t* ltrb, Tr2LockedRenderTargetAL& lockedRT, Tr2RenderContextAL& renderContext )
{
	if( !m_backingStore.m_texture || !renderContext.IsValid() || ( m_mipCount > 0 && mipLevel >= m_mipCount ) )
	{
		return E_FAIL;
	}
		
	if( !renderContext.m_secondaryDevice11 )
	{
		return E_FAIL;
	}
	
	const uint32_t width  = ltrb ? ltrb[2] - ltrb[0] : m_width;
	const uint32_t height = ltrb ? ltrb[3] - ltrb[1] : m_height;

	D3D11_TEXTURE2D_DESC desc;
	memset( &desc, 0, sizeof (desc) );
	desc.Width		= width >> mipLevel;
	desc.Height		= height >> mipLevel;
	if( IsCompressedFormat( GetFormat() ) )
	{
		desc.Width  = std::max( desc.Width , 4u );
		desc.Height = std::max( desc.Height, 4u );
	}
	else
	{
		desc.Width  = std::max( desc.Width , 1u );
		desc.Height = std::max( desc.Height, 1u );
	}
	desc.MipLevels	= 1;
	desc.ArraySize  = 1;
	desc.Format     = static_cast<DXGI_FORMAT>( MakeTypeless( m_format ) );
		
	desc.Usage			= D3D11_USAGE_STAGING;
	desc.CPUAccessFlags	= D3D11_CPU_ACCESS_READ;
	
	desc.SampleDesc.Count = 1;

	if( lockedRT.m_staging )
	{
		D3D11_TEXTURE2D_DESC desc2;
		lockedRT.m_staging->GetDesc( &desc2 );
		if( desc2.Width != desc.Width || desc2.Height != desc.Height || desc2.Format != desc.Format )
		{
			lockedRT.m_staging = nullptr;
		}
	}

	if( !lockedRT.m_staging )
	{
		CR_RETURN_HR( renderContext.m_secondaryDevice11->CreateTexture2D( &desc, nullptr, &lockedRT.m_staging ) );
		if( !lockedRT.m_staging )
		{
			return E_FAIL;
		}
	}
	if( ltrb )
	{
		D3D11_BOX box = { ltrb[0], ltrb[1], 0, ltrb[2], ltrb[3], 1 };
		renderContext.m_context->CopySubresourceRegion( lockedRT.m_staging, 0, 0, 0, 0, m_texture, D3D10CalcSubresource( mipLevel, 0, GetTrueMipCount() ), &box );
	}
	else
	{
		renderContext.m_context->CopySubresourceRegion( lockedRT.m_staging, 0, 0, 0, 0, m_texture, D3D10CalcSubresource( mipLevel, 0, GetTrueMipCount() ), nullptr );
	}
	return S_OK;
}

// -------------------------------------------------------------
// Description:
//  Try to lock the underlying texture.  The lock is always LOCK_READONLY.
//  Avoid this function, use Resolve and full screen quads instead where possible.
//  This function may be extremely expensive.
// -------------------------------------------------------------
ALResult Tr2RenderTargetAL::Lock(	
	uint32_t mipLevel, 
	uint32_t* ltrb, 
	void*& data, 
	uint32_t& pitch, 
	Tr2RenderContextAL& renderContext )
{
	return m_backingStore.Lock( mipLevel, ltrb, data, pitch, LOCK_READONLY, renderContext );
}


ALResult Tr2RenderTargetAL::Unlock( Tr2RenderContextAL& renderContext )
{
	return m_backingStore.Unlock( renderContext );
}

// -------------------------------------------------------------
// Description:
//  Give the renderTarget a hint that it may be locked frequently, and it might want to hold
//  on to any staging resources needed to transfer data.
// -------------------------------------------------------------
void Tr2RenderTargetAL::SetHintLockOften()
{
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

			if( d.m_width  > 0 && 
				d.m_height > 0 &&
				SUCCEEDED( Create(	d.m_width, 
									d.m_height, 
									d.m_mipCount, 
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

void Tr2RenderTargetAL::ReleaseALResource()
{
	if( !m_isAttached && !m_deviceLost.m_valid )
	{
		m_deviceLost.m_format		= m_format;
		m_deviceLost.m_width		= m_width;
		m_deviceLost.m_height		= m_height;
		m_deviceLost.m_mipCount		= m_mipCount;
		m_deviceLost.m_msaaType		= m_msaaType;
		m_deviceLost.m_msaaQuality	= m_msaaQuality;

		m_deviceLost.m_valid = true;
	}

	Destroy();
}

uint32_t Tr2RenderTargetAL::GetSharedHandle() const
{
	IDXGIResource* pOtherResource( NULL );
	if( m_texture == NULL )
	{
		CCP_AL_LOGERR( "GetSharedHandle: texture is NULL" );
		return 0;
	}
	HRESULT HR = m_texture->QueryInterface( __uuidof(IDXGIResource), (void**)&pOtherResource );
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

#endif // ( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
