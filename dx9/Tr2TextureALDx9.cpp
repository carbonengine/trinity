#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "Tr2TextureALDx9.h"
#include "BcDecompress.h"
#include "ALLog.h"
#include "Tr2RenderContextDx9.h"

extern bool g_preloadTextureToDeviceOnPrepare;
extern bool g_usingEXDevice;
extern bool g_useManagedDX9Buffers;

#pragma warning( disable : 4189 )


namespace
{

	ALResult Create2D( 
		CComPtr<IDirect3DTexture9>& tex, 
		HANDLE* shared, 
		const Tr2BitmapDimensions& desc, 
		D3DFORMAT format, 
		D3DPOOL pool, 
		uint32_t usage, 
		Tr2SubresourceData* initialData, 
		Tr2PrimaryRenderContextAL& renderContext )
	{
		FORWARD_HR( renderContext.m_d3dDevice9->CreateTexture(
			desc.GetWidth(),
			desc.GetHeight(),
			desc.GetMipCount(),
			usage,
			format,
			pool,
			&tex,
			shared ) );

		bool needsStagingTexture = false;
		if( pool == D3DPOOL_DEFAULT && ( usage & D3DUSAGE_DYNAMIC ) == 0 )
		{
			needsStagingTexture = true;
		}

		// Create a staging texture in system memory, load data into it.
		// Then we use UpdateTexture - this way works with default pool textures without
		// having to set the dynamic usage flag.
		CComPtr<IDirect3DTexture9> stagingTexture;

		if( needsStagingTexture )
		{
			CR_RETURN_HR( renderContext.m_d3dDevice9->CreateTexture(
				desc.GetWidth(),
				desc.GetHeight(),
				desc.GetTrueMipCount(),
				0,
				format,
				D3DPOOL_SYSTEMMEM,
				&stagingTexture,
				nullptr ) );
		}
		else
		{
			stagingTexture = tex;
		}

		if( initialData )
		{
			for( uint32_t i = 0; i < desc.GetTrueMipCount(); ++i )
			{
				auto p = static_cast<const uint8_t*>( initialData[i].m_sysMem );
				if( !p )
				{
					return E_INVALIDARG;
				}

				D3DLOCKED_RECT l;
				auto hr = stagingTexture->LockRect( i, &l, 0, 0 );
				if( FAILED( hr ) || !l.pBits )
				{
					return hr;
				}
				if( format == MAKEFOURCC( 'A', 'T', 'I', '1' ) )
				{
					// dx9 incorrectly reports pitch for BC4 format as if it's uncompressed
					l.Pitch *= 2;
				}

				const uint32_t numRows = desc.GetMipNumRows( i );
				const uint32_t mipLevelSize = std::min( desc.GetMipSize( i ), initialData[i].m_sysMemSlicePitch );

				if( l.Pitch * numRows == mipLevelSize )
				{
					memcpy( l.pBits, p, mipLevelSize );
				}
				else
				{
					for( uint32_t j = 0; j != numRows; ++j )
					{
						memcpy(
							static_cast<uint8_t*>( l.pBits ) + j * l.Pitch,
							p + j * initialData[i].m_sysMemPitch,
							std::min<uint32_t>( l.Pitch, initialData[i].m_sysMemPitch ) );
					}
				}

				stagingTexture->UnlockRect( i );
			}

			if( needsStagingTexture )
			{
				CR_RETURN_HR( tex->AddDirtyRect( nullptr ) );
				CR_RETURN_HR( renderContext.m_d3dDevice9->UpdateTexture( stagingTexture, tex ) );
			}
		}
		return S_OK;
	}

	ALResult CreateCube( 
		CComPtr<IDirect3DCubeTexture9>& tex, 
		HANDLE* shared, 
		const Tr2BitmapDimensions& desc, 
		D3DFORMAT format, 
		D3DPOOL pool, 
		uint32_t usage, 
		Tr2SubresourceData* initialData, 
		Tr2PrimaryRenderContextAL& renderContext )
	{
		bool needsStagingTexture = false;
		if( initialData && pool == D3DPOOL_DEFAULT && ( usage & D3DUSAGE_DYNAMIC ) == 0 )
		{
			needsStagingTexture = true;
		}

		CR_RETURN_HR( renderContext.m_d3dDevice9->CreateCubeTexture( desc.GetWidth(), desc.GetMipCount(), usage, format, pool, &tex, shared ) );

		CComPtr<IDirect3DTexture9> stagingTexture;

		if( needsStagingTexture )
		{
			CR_RETURN_HR( renderContext.m_d3dDevice9->CreateTexture(
				desc.GetWidth(),
				desc.GetHeight(),
				desc.GetTrueMipCount(),
				0,
				format,
				D3DPOOL_SYSTEMMEM,
				&stagingTexture,
				nullptr ) );
		}


		if( initialData )
		{
			for( uint32_t i = 0; i != desc.GetTrueMipCount(); ++i )
			{
				for( uint32_t face = 0; face != 6; ++face )
				{
					const Tr2SubresourceData& srd = initialData[face * desc.GetTrueMipCount() + i];

					D3DLOCKED_RECT l =
					{
						0
					};
					if( stagingTexture )
					{
						CR_RETURN_HR( stagingTexture->LockRect( i, &l, 0, D3DLOCK_DISCARD ) );
					}
					else
					{
						CR_RETURN_HR( tex->LockRect( (D3DCUBEMAP_FACES)face, i, &l, 0, 0 ) );
					}

					if( !l.pBits )
					{
						return E_FAIL;
					}
					memcpy( l.pBits, srd.m_sysMem, srd.m_sysMemSlicePitch );

					if( stagingTexture )
					{
						CR_RETURN_HR( stagingTexture->UnlockRect( i ) );

						CComPtr<IDirect3DSurface9> srcSurface, destSurface;

						CR_RETURN_HR( stagingTexture->GetSurfaceLevel( i, &srcSurface ) );
						CR_RETURN_HR( tex->GetCubeMapSurface( (D3DCUBEMAP_FACES)face, i, &destSurface ) );
						CR_RETURN_HR( renderContext.m_d3dDevice9->UpdateSurface( srcSurface, nullptr, destSurface, nullptr ) )
							;
					}
					else
					{
						CR_RETURN_HR( tex->UnlockRect( (D3DCUBEMAP_FACES)face, i ) );
					}
				}
			}
		}
		return S_OK;
	}

	ALResult Create3D( 
		CComPtr<IDirect3DVolumeTexture9>& tex, 
		HANDLE* shared, 
		const Tr2BitmapDimensions& desc, 
		D3DFORMAT& format, 
		D3DPOOL pool, 
		uint32_t usage, 
		Tr2SubresourceData* initialData, 
		Tr2PrimaryRenderContextAL& renderContext )
	{
		bool needsDecompression = false;
		auto hr = renderContext.m_d3dDevice9->CreateVolumeTexture(
			desc.GetWidth(),
			desc.GetHeight(),
			desc.GetDepth(),
			desc.GetTrueMipCount(),
			usage,
			format,
			pool,
			&tex,
			shared );
		if( FAILED( hr ) )
		{
			if( Tr2RenderContextEnum::IsCompressedFormat( desc.GetFormat() ) )
			{
				format = renderContext.ConvertToD3D9Format( Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM );
				CR_RETURN_HR( renderContext.m_d3dDevice9->CreateVolumeTexture(
					desc.GetWidth(),
					desc.GetHeight(),
					desc.GetDepth(),
					desc.GetTrueMipCount(),
					usage,
					format,
					pool,
					&tex,
					shared ) );
				needsDecompression = true;
			}
			else
			{
				return hr;
			}
		}

		bool needsStagingTexture = false;
		if( initialData && pool == D3DPOOL_DEFAULT && ( usage & D3DUSAGE_DYNAMIC ) == 0 )
		{
			needsStagingTexture = true;
		}

		CComPtr<IDirect3DVolumeTexture9> stagingTexture;

		if( needsStagingTexture )
		{
			CR_RETURN_HR( renderContext.m_d3dDevice9->CreateVolumeTexture(
				desc.GetWidth(),
				desc.GetHeight(),
				desc.GetDepth(),
				desc.GetTrueMipCount(),
				0,
				format,
				D3DPOOL_SYSTEMMEM,
				&stagingTexture,
				nullptr ) );
		}
		else
		{
			stagingTexture = tex;
		}

		std::unique_ptr<uint8_t[]> decompressed;

		for( uint32_t i = 0; i != desc.GetTrueMipCount(); ++i )
		{
			D3DLOCKED_BOX l =
			{
				0
			};
			CR_RETURN_HR( stagingTexture->LockBox( i, &l, 0, 0 ) );
			ON_BLOCK_EXIT( [&] {
				stagingTexture->UnlockBox( i );
			} );

			if( !l.pBits )
			{
				return E_FAIL;
			}

			uint32_t levelDepth = std::max( desc.GetDepth() >> i, 1U );
			if( needsDecompression )
			{
				uint32_t levelWidth = std::max( desc.GetWidth() >> i, 1U );
				uint32_t levelHeight = std::max( desc.GetHeight() >> i, 1U );
				if( !BcDecompress( levelWidth, levelHeight, levelDepth, desc.GetFormat(), initialData[i], decompressed ) )
				{
					return E_FAIL;
				}
				memcpy( l.pBits, decompressed.get(), levelWidth * levelHeight * 4 * levelDepth );
			}
			else
			{
				memcpy( l.pBits, initialData[i].m_sysMem, initialData[i].m_sysMemSlicePitch * levelDepth );
			}
		}
		if( needsStagingTexture )
		{
			tex->AddDirtyBox( nullptr );
			renderContext.m_d3dDevice9->UpdateTexture( stagingTexture, tex );
		}
		return S_OK;
	}

	ALResult CreateRenderTarget( 
		CComPtr<IDirect3DTexture9>& tex, 
		CComPtr<IDirect3DTexture9>& mipGeneratedRT, 
		CComPtr<IDirect3DSurface9>& surface, 
		HANDLE* shared, 
		const Tr2BitmapDimensions& desc, 
		const Tr2MsaaDesc& msaa, 
		D3DFORMAT format, 
		uint32_t usage, 
		Tr2PrimaryRenderContextAL& renderContext )
	{
		if( msaa.samples > 1 )
		{
			CComQIPtr<IDirect3DDevice9Ex> exDevice( renderContext.m_d3dDevice9 );
			if( exDevice )
			{
				CR_RETURN_HR( exDevice->CreateRenderTargetEx(
					desc.GetWidth(),
					desc.GetHeight(),
					format,
					static_cast<D3DMULTISAMPLE_TYPE>( msaa.samples > 1 ? msaa.samples : 0 ),
					msaa.quality,
					false,
					&surface,
					shared,
					D3DUSAGE_NONSECURE ) );
			}
			else
			{
				CR_RETURN_HR( renderContext.m_d3dDevice9->CreateRenderTarget(
					desc.GetWidth(),
					desc.GetHeight(),
					format,
					static_cast<D3DMULTISAMPLE_TYPE>( msaa.samples > 1 ? msaa.samples : 0 ),
					msaa.quality,
					false,
					&surface,
					shared ) );
			}
		}
		else
		{
			auto hr = renderContext.m_d3dDevice9->CreateTexture(
				desc.GetWidth(),
				desc.GetHeight(),
				desc.GetMipCount(),
				D3DUSAGE_RENDERTARGET | usage,
				format,
				D3DPOOL_DEFAULT,
				&tex,
				shared );
			if( FAILED( hr ) )
			{
				CR_RETURN_HR( hr );
			}

			if( hr == D3DOK_NOAUTOGEN )
			{
				// We need to manually generate mip-levels. Unfortunately, many video cards (nVidia cards in
				// particular) seem to fail miserably when using StretcRect from one mip-level surface to the
				// next so we have to set up a separate render target texture for generating the mip-levels.
				// Using StretchRect between surface levels from different textures seems to work fine, though,
				// but it means we're wasting an extra top-level surface.
				CR_RETURN_HR( renderContext.m_d3dDevice9->CreateTexture(
					std::max( desc.GetWidth() / 2, 1u ),
					std::max( desc.GetHeight() / 2, 1u ),
					1,
					D3DUSAGE_RENDERTARGET | usage,
					format,
					D3DPOOL_DEFAULT,
					&mipGeneratedRT,
					nullptr ) );
			}
		}
		return S_OK;
	}

	ALResult CreateDepthStencil( 
		CComPtr<IDirect3DTexture9>& tex, 
		CComPtr<IDirect3DSurface9>& surface, 
		HANDLE* shared, 
		const Tr2BitmapDimensions& desc, 
		const Tr2MsaaDesc& msaa, 
		Tr2GpuUsage::Type gpuUsage, 
		D3DFORMAT& format, 
		Tr2PrimaryRenderContextAL& renderContext )
	{
		if( HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			if( format != D3DFMT_D24S8 )
			{
				return E_INVALIDARG;
			}
			format = D3DFORMAT( MAKEFOURCC( 'I', 'N', 'T', 'Z' ) );
			CR_RETURN_HR( renderContext.m_d3dDevice9->CreateTexture(
				desc.GetWidth(),
				desc.GetHeight(),
				1,
				D3DUSAGE_DEPTHSTENCIL,
				format,
				D3DPOOL_DEFAULT,
				&tex,
				nullptr ) );
		}
		else
		{
			auto sampleType = static_cast<D3DMULTISAMPLE_TYPE>( msaa.samples > 1 ? msaa.samples : 0 );
			CComQIPtr<IDirect3DDevice9Ex> exDevice( renderContext.m_d3dDevice9 );
			if( exDevice )
			{
				CR_RETURN_HR( exDevice->CreateDepthStencilSurfaceEx(
					desc.GetWidth(),
					desc.GetHeight(),
					format,
					sampleType,
					msaa.quality,
					/*Discard*/ TRUE,
					&surface,
					shared,
					D3DUSAGE_NONSECURE ) );
			}
			else
			{
				CR_RETURN_HR( renderContext.m_d3dDevice9->CreateDepthStencilSurface(
					desc.GetWidth(),
					desc.GetHeight(),
					format,
					sampleType,
					msaa.quality,
					/*Discard*/ TRUE,
					&surface,
					shared ) );
			}
		}
		return S_OK;
	}

	RECT GetRegionRect( const Tr2TextureSubresource& region, const Tr2BitmapDimensions& desc )
	{
		RECT rect;
		if( region.HasBox() )
		{
			rect.left = region.m_left;
			rect.top = region.m_top;
			rect.right = region.m_right;
			rect.bottom = region.m_bottom;
		}
		else
		{
			rect.left = 0;
			rect.top = 0;
			rect.right = desc.GetMipWidth( region.m_startMipLevel );
			rect.bottom = desc.GetMipHeight( region.m_startMipLevel );
		}
		return rect;
	}

	void GetSurface( 
		IDirect3DBaseTexture9* texture,
		Tr2RenderContextEnum::TextureType type,
		uint32_t cubeFace,
		uint32_t mipLevel,
		IDirect3DSurface9** surface )
	{
		*surface = nullptr;
		switch( type )
		{
		case Tr2RenderContextEnum::TEX_TYPE_1D:
		case Tr2RenderContextEnum::TEX_TYPE_2D:
		{
			CComQIPtr<IDirect3DTexture9> tex2D( texture );
			if( !tex2D || FAILED( tex2D->GetSurfaceLevel( mipLevel, surface ) ) )
			{
				surface = nullptr;
			}
		}
		return;
		case Tr2RenderContextEnum::TEX_TYPE_CUBE:
		{
			CComQIPtr<IDirect3DCubeTexture9> texCube( texture );
			if( !texCube || FAILED( texCube->GetCubeMapSurface( D3DCUBEMAP_FACES( cubeFace ), mipLevel, surface ) ) )
			{
				surface = nullptr;
			}
		}
		return;
		}
	}

	ALResult UpdateRect( const D3DLOCKED_RECT& dest, const void* source, uint32_t pitch, uint32_t width, uint32_t height, Tr2RenderContextEnum::PixelFormat format )
	{
		if( !dest.pBits )
		{
			return E_FAIL;
		}

		const uint8_t* src = static_cast<const uint8_t*>( source );
		uint8_t* dst = static_cast<uint8_t*>( dest.pBits );

		if( IsCompressedFormat( format ) )
		{
			uint32_t blockByteSize = GetBlockByteSize( format );
			uint32_t blockPixelSize = 4;
			uint32_t blocksX = width / blockPixelSize;

			for( uint32_t line = 0; line < height; line += blockPixelSize )
			{
				memcpy( dst, src, blocksX * blockByteSize );

				src += pitch;
				dst += dest.Pitch;
			}
		}
		else
		{
			uint32_t byteCount = GetBytesPerPixel( format );

			for( uint32_t line = 0; line != height; ++line )
			{
				memcpy( dst, src, width * byteCount );

				src += pitch;
				dst += dest.Pitch;
			}
		}
		return S_OK;
	}
}


namespace TrinityALImpl
{
	Tr2TextureAL::Tr2TextureAL()
		:m_sharedHandle( nullptr ),
		m_pool( D3DPOOL_DEFAULT ),
		m_format( D3DFMT_UNKNOWN ),
		m_gpuUsage( Tr2GpuUsage::NONE ),
		m_cpuUsage( Tr2CpuUsage::NONE ),
		m_lockedSubresource( 0 )
	{
	}

	ALResult Tr2TextureAL::Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( HasBufferFlags( gpuUsage ) )
		{
			return E_INVALIDARG;
		}

		if( !renderContext.IsValid() )
		{
			return E_FAIL;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			return E_INVALIDARG;
		}
		if( msaa.samples > 1 )
		{
			if( HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
			{
				return E_INVALIDARG;
			}
			if( desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D )
			{
				return E_INVALIDARG;
			}
			if( desc.GetTrueMipCount() > 1 )
			{
				return E_INVALIDARG;
			}
		}
		if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
		{
			if( desc.GetArraySize() != 6 )
			{
				return E_INVALIDARG;
			}
		}
		else if( desc.GetArraySize() > 1 )
		{
			return E_INVALIDARG;
		}
		if( desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D )
		{
			if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
			{
				return E_INVALIDARG;
			}
			if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
			{
				return E_INVALIDARG;
			}
		}
		if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D && cpuUsage != Tr2CpuUsage::NONE )
		{
			return E_INVALIDARG;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) && HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			return E_INVALIDARG;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) || HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			if( Tr2CpuUsage::HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) )
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			if( !initialData && !Tr2CpuUsage::HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) )
			{
				return E_INVALIDARG;
			}
		}
		if( msaa.samples > 1 && cpuUsage != Tr2CpuUsage::NONE )
		{
			return E_INVALIDARG;
		}

		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) && cpuUsage != Tr2CpuUsage::NONE )
		{
			return E_INVALIDARG;
		}

		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) && desc.GetTrueMipCount() > 1 )
		{
			return E_INVALIDARG;
		}

		D3DPOOL pool = D3DPOOL_DEFAULT;
		uint32_t usage = 0;
		if( !HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) && !HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			if( Tr2CpuUsage::HasFlag( cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
			{
				pool = D3DPOOL_DEFAULT;
				usage = D3DUSAGE_DYNAMIC;
			}
			else if( g_useManagedDX9Buffers && !g_usingEXDevice )
			{
				pool = D3DPOOL_MANAGED;
			}
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) && desc.GetTrueMipCount() > 1 )
		{
			usage |= D3DUSAGE_AUTOGENMIPMAP;
		}

		auto format = renderContext.ConvertToD3D9Format( desc.GetFormat() );
		if( format == D3DFMT_UNKNOWN )
		{
			return E_FAIL;
		}


		HANDLE handle = nullptr;

		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
		{
			CComPtr<IDirect3DSurface9> surface;
			CComPtr<IDirect3DTexture9> mipGeneratedRT;
			CComPtr<IDirect3DTexture9> tex;
			FORWARD_HR( CreateRenderTarget( tex, mipGeneratedRT, surface, HasFlag( gpuUsage, Tr2GpuUsage::SHARED ) ? &handle : nullptr, desc, msaa, format, usage, renderContext ) );
			if( !surface )
			{
				CR_RETURN_HR( tex->GetSurfaceLevel( 0, &surface ) );
			}
			m_texture = tex;
			m_mipGeneratedRT = mipGeneratedRT;
			m_surface = surface;
		}
		else if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			CComPtr<IDirect3DSurface9> surface;
			CComPtr<IDirect3DTexture9> tex;
			FORWARD_HR( CreateDepthStencil( tex, surface, HasFlag( gpuUsage, Tr2GpuUsage::SHARED ) ? &handle : nullptr, desc, msaa, gpuUsage, format, renderContext ) );
			if( !surface )
			{
				CR_RETURN_HR( tex->GetSurfaceLevel( 0, &surface ) );
			}
			m_texture = tex;
			m_surface = surface;
		}
		else
		{
			if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
			{
				CComPtr<IDirect3DCubeTexture9> tex;
				FORWARD_HR( CreateCube( tex, HasFlag( gpuUsage, Tr2GpuUsage::SHARED ) ? &handle : nullptr, desc, format, pool, usage, initialData, renderContext ) );
				m_texture.Attach( tex.Detach() );
			}
			else if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D )
			{
				CComPtr<IDirect3DVolumeTexture9> tex;
				FORWARD_HR( Create3D( tex, HasFlag( gpuUsage, Tr2GpuUsage::SHARED ) ? &handle : nullptr, desc, format, pool, usage, initialData, renderContext ) );
				m_texture.Attach( tex.Detach() );
			}
			else
			{
				CComPtr<IDirect3DTexture9> tex;
				FORWARD_HR( Create2D( tex, HasFlag( gpuUsage, Tr2GpuUsage::SHARED ) ? &handle : nullptr, desc, format, pool, usage, initialData, renderContext ) );
				m_texture.Attach( tex.Detach() );
			}
			if( g_preloadTextureToDeviceOnPrepare && pool == D3DPOOL_MANAGED )
			{
				m_texture->PreLoad();
			}
		}


		m_pool = pool;
		m_format = format;
		m_desc = desc;
		m_gpuUsage = gpuUsage;
		m_cpuUsage = cpuUsage;
		m_msaa = msaa;

		m_memory.Set( Tr2MemoryCounterAL::TEXTURE, m_desc, msaa );

		return S_OK;
	}

	ALResult Tr2TextureAL::OpenShared( uintptr_t, Tr2GpuUsage::Type, Tr2PrimaryRenderContextAL& )
	{
		return E_FAIL;
	}


	void Tr2TextureAL::Destroy()
	{
		m_texture = nullptr;
		m_mipGeneratedRT = nullptr;
		m_surface = nullptr;
		m_lockedSurface = nullptr;
		if( m_sharedHandle )
		{
			::CloseHandle( m_sharedHandle );
			m_sharedHandle = nullptr;
		}
		m_memory.Reset();
		memset( &m_desc, 0, sizeof( m_desc ) );
		m_msaa = Tr2MsaaDesc();
		m_gpuUsage = Tr2GpuUsage::NONE;
		m_cpuUsage = Tr2CpuUsage::NONE;
		m_pool = D3DPOOL_DEFAULT;
		m_format = D3DFMT_UNKNOWN;
	}

	bool Tr2TextureAL::IsValid() const
	{
		return m_texture != nullptr || m_surface != nullptr;
	}

	Tr2ALMemoryType Tr2TextureAL::GetMemoryClass() const
	{
		return m_pool == D3DPOOL_MANAGED ? AL_MEMORY_MANAGED : AL_MEMORY_VIDEO;
	}

	const Tr2BitmapDimensions& Tr2TextureAL::GetDesc() const
	{
		return m_desc;
	}

	const Tr2MsaaDesc& Tr2TextureAL::GetMsaaDesc() const
	{
		return m_msaa;
	}

	Tr2GpuUsage::Type Tr2TextureAL::GetGpuUsage() const
	{
		return m_gpuUsage;
	}

	Tr2CpuUsage::Type Tr2TextureAL::GetCpuUsage() const
	{
		return m_cpuUsage;
	}

	ALResult Tr2TextureAL::MapForReading( const Tr2TextureSubresource& region, const void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
	{
		data = nullptr;
		pitch = 0;

		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::READ ) )
		{
			return E_INVALIDCALL;
		}

		return Lock( region, D3DLOCK_READONLY, *const_cast<void**>( &data ), pitch, renderContext );
	}

	void Tr2TextureAL::UnmapForReading( Tr2RenderContextAL& renderContext )
	{
		CCP_UNUSED( renderContext );

		Unlock();
	}

	ALResult Tr2TextureAL::MapForWriting( const Tr2TextureSubresource& region, void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
	{
		data = nullptr;
		pitch = 0;

		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			return E_INVALIDCALL;
		}

		return Lock( region, Tr2CpuUsage::HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) ? D3DLOCK_DISCARD : 0, data, pitch, renderContext );
	}

	void Tr2TextureAL::UnmapForWriting( Tr2RenderContextAL& renderContext )
	{
		CCP_UNUSED( renderContext );

		Unlock();
	}

	ALResult Tr2TextureAL::UpdateSubresource( const Tr2TextureSubresource& region, const void* source, uint32_t pitch, uint32_t slicePitch, Tr2RenderContextAL& renderContext )
	{
		CCP_UNUSED( slicePitch );

		if( HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			return E_INVALIDCALL;
		}
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE ) && !IsWritable( m_gpuUsage ) )
		{
			return E_INVALIDCALL;
		}

		if( !IsValid() || !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		if( !region.IsValidForBitmap( m_desc ) )
		{
			return E_INVALIDARG;
		}
		if( !region.IsSingleSubresource() )
		{
			return E_INVALIDARG;
		}


		RECT rect = GetRegionRect( region, m_desc );
		const uint32_t width = rect.right - rect.left;
		const uint32_t height = rect.bottom - rect.top;

		CComQIPtr<IDirect3DTexture9> tex2D( m_texture );

		D3DLOCKED_RECT l =
		{
			0
		};
		if( FAILED( tex2D->LockRect( 0, &l, &rect, 0 ) ) )
		{
			CComPtr<IDirect3DSurface9> srcSurface, destSurface;
			CR_RETURN_HR( tex2D->GetSurfaceLevel( 0, &destSurface ) );
			CR_RETURN_HR( renderContext.m_d3dDevice9->CreateOffscreenPlainSurface( width, height, m_format, D3DPOOL_SYSTEMMEM, &srcSurface, nullptr ) );
			{
				CR_RETURN_HR( srcSurface->LockRect( &l, nullptr, 0 ) );
				ON_BLOCK_EXIT( [&] { srcSurface->UnlockRect(); } );

				FORWARD_HR( UpdateRect( l, source, pitch, width, height, m_desc.GetFormat() ) );
			}
			POINT destPoint = { rect.left, rect.top };
			CR_RETURN_HR( renderContext.m_d3dDevice9->UpdateSurface( srcSurface, nullptr, destSurface, &destPoint ) );

			return S_OK;
		}

		ON_BLOCK_EXIT( [&] { tex2D->UnlockRect( 0 ); } );

		FORWARD_HR( UpdateRect( l, source, pitch, width, height, m_desc.GetFormat() ) );

		return S_OK;
	}

	ALResult Tr2TextureAL::CopySubresourceRegion( const Tr2TextureSubresource& destSubresource, Tr2TextureAL& source, const Tr2TextureSubresource& sourceSubresource, Tr2RenderContextAL& renderContext )
	{
		if( !IsValid() || !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( !source.IsValid() )
		{
			return E_INVALIDARG;
		}
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE ) && !IsWritable( m_gpuUsage ) )
		{
			return E_INVALIDCALL;
		}
		if( m_desc.GetFormat() != source.GetDesc().GetFormat() )
		{
			return E_INVALIDARG;
		}
		if( m_desc.GetType() != source.GetDesc().GetType() )
		{
			return E_INVALIDARG;
		}
		if( m_desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D )
		{
			return E_INVALIDARG;
		}

		Tr2TextureSubresource src = sourceSubresource;
		Tr2TextureSubresource dst = destSubresource;

		if( !Crop( src, source.GetDesc(), dst, GetDesc() ) )
		{
			return E_FAIL;
		}

		if( HasFlag( GetGpuUsage(), Tr2GpuUsage::RENDER_TARGET ) )
		{
			if( !HasFlag( source.GetGpuUsage(), Tr2GpuUsage::RENDER_TARGET ) )
			{
				return E_FAIL;
			}

			RECT srcRect;
			srcRect.left = src.m_left;
			srcRect.top = src.m_top;
			srcRect.right = std::min( src.m_right, source.m_desc.GetWidth() );
			srcRect.bottom = std::min( src.m_bottom, source.m_desc.GetHeight() );

			RECT destRect;
			destRect.left = dst.m_left;
			destRect.top = dst.m_top;
			destRect.right = std::min( uint32_t( dst.m_left + ( srcRect.right - srcRect.left ) ), m_desc.GetWidth() );
			destRect.bottom = std::min( uint32_t( dst.m_top + ( srcRect.bottom - srcRect.top ) ), m_desc.GetHeight() );

			return renderContext.m_d3dDevice9->StretchRect( source.m_surface, &srcRect, m_surface, &destRect, D3DTEXF_POINT );
		}

		uint32_t lockFlag = 0;

		const bool nullSource = sourceSubresource == Tr2TextureSubresource();
		const bool nullDest = destSubresource == Tr2TextureSubresource();

		const bool isCompressed = IsCompressedFormat( m_desc.GetFormat() );
		const uint32_t blockSize = GetBlockByteSize( m_desc.GetFormat() );
		const uint32_t byteCount = GetBytesPerPixel( m_desc.GetFormat() );

		const uint32_t mipCount = std::min( src.GetMipCount(), dst.GetMipCount() );

		for( uint32_t mip = 0; mip != mipCount; ++mip )
		{
			for( uint32_t face = 0; face != src.GetFaceCount(); ++face )
			{
				if( HasFlag( source.GetGpuUsage(), Tr2GpuUsage::RENDER_TARGET ) )
				{
					CComQIPtr<IDirect3DTexture9> tex2D( m_texture );

					const void* srcData = nullptr;
					uint32_t srcPitch;
					Tr2TextureSubresource mipRegion = src;
					mipRegion.m_startMipLevel += mip;
					mipRegion.m_endMipLevel = mipRegion.m_startMipLevel + 1;
					CR_RETURN_HR( source.MapForReading( mipRegion, srcData, srcPitch, renderContext ) );
					ON_BLOCK_EXIT( [&] { source.UnmapForReading( renderContext ); } );

					RECT destLtrb =
					{
						(LONG)dst.m_left,
						(LONG)dst.m_top,
						(LONG)dst.m_right,
						(LONG)dst.m_bottom
					};

					D3DLOCKED_RECT lockedRect;
					uint32_t mipLevel = dst.m_startMipLevel + mip;
					CR_RETURN_HR( tex2D->LockRect( mipLevel, &lockedRect, &destLtrb, 0 ) );
					ON_BLOCK_EXIT( [&] { tex2D->UnlockRect( mipLevel ); } );

					FORWARD_HR( UpdateRect( lockedRect, srcData, srcPitch, std::min( src.GetWidth(), dst.GetWidth() ), std::min( src.GetHeight(), dst.GetHeight() ), m_desc.GetFormat() ) );
				}
				else
				{
					CComPtr<IDirect3DSurface9> srcSurface, dstSurface;
					GetSurface(
						source.m_texture,
						source.GetDesc().GetType(),
						src.m_startFace + face,
						src.m_startMipLevel + mip,
						&srcSurface );
					GetSurface(
						m_texture,
						GetDesc().GetType(),
						dst.m_startFace + face,
						dst.m_startMipLevel + mip,
						&dstSurface );

					if( !srcSurface || !dstSurface )
					{
						return E_FAIL;
					}

					RECT srcRect = GetRegionRect( src, source.GetDesc() );
					RECT dstRect = GetRegionRect( dst, m_desc );

					if( SUCCEEDED( renderContext.m_d3dDevice9->StretchRect(
						srcSurface,
						nullSource ? nullptr : &srcRect,
						dstSurface,
						nullDest ? nullptr : &dstRect,
						D3DTEXF_POINT ) ) )
					{
						continue;
					}

					D3DLOCKED_RECT srcLock = { 0 };
					CR_RETURN_HR( srcSurface->LockRect( &srcLock, &srcRect, D3DLOCK_READONLY ) );
					ON_BLOCK_EXIT( [&] { srcSurface->UnlockRect(); } );

					D3DLOCKED_RECT dstLock = { 0 };
					CR_RETURN_HR( dstSurface->LockRect( &dstLock, &dstRect, lockFlag ) );
					ON_BLOCK_EXIT( [&] { dstSurface->UnlockRect(); } );


					if( !srcLock.pBits || !dstLock.pBits )
					{
						return E_FAIL;
					}

					FORWARD_HR( UpdateRect( dstLock, srcLock.pBits, srcLock.Pitch, std::min( src.GetWidth(), dst.GetWidth() ), std::min( src.GetHeight(), dst.GetHeight() ), m_desc.GetFormat() ) );
				}
			}

			if( mip + 1 != mipCount )
			{
				AdvanceMip( src, source.GetDesc(), mip );
				AdvanceMip( dst, GetDesc(), mip );
			}
		}

		return S_OK;
	}

	ALResult Tr2TextureAL::GenerateMipMaps( Tr2RenderContextAL& renderContext )
	{
		if( !HasFlag( m_gpuUsage, Tr2GpuUsage::RENDER_TARGET ) || !HasFlag( m_gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			return E_INVALIDCALL;
		}
		if( m_desc.GetTrueMipCount() <= 1 )
		{
			return S_OK;
		}
		if( !m_mipGeneratedRT )
		{
			m_texture->GenerateMipSubLevels();
			return S_OK;
		}
		CComQIPtr<IDirect3DTexture9> tex2D( m_texture );
		if( !tex2D )
		{
			return E_FAIL;
		}

		// Some (most nVidia, it seems) video cards can't do a StretchRect from one mip-level
		// surface to the next within the same texture. We therefore have to copy (with filtering)
		// from the m_mainRT texture to the m_mipGeneratedRT, then copy those filtered results
		// back to use as source for the next level down.

		const uint32_t levelCount = m_texture->GetLevelCount();

		CComPtr<IDirect3DSurface9> srcLevel;
		CComPtr<IDirect3DSurface9> dstLevel;
		CComPtr<IDirect3DSurface9> scratch;

		RECT srcRect = { 0, 0, (LONG)m_desc.GetWidth(), (LONG)m_desc.GetHeight() };
		RECT dstRect = { 0, 0, (LONG)std::max( m_desc.GetWidth() / 2, 1u ), (LONG)std::max( m_desc.GetHeight() / 2, 1u ) };


		CR_RETURN_HR( m_mipGeneratedRT->GetSurfaceLevel( 0, &scratch ) );
		CR_RETURN_HR( tex2D->GetSurfaceLevel( 0, &srcLevel ) );

		for( uint32_t levelIx = 1; levelIx < levelCount; ++levelIx )
		{
			CR( renderContext.m_d3dDevice9->StretchRect( srcLevel, &srcRect, scratch, &dstRect, D3DTEXF_LINEAR ) );
			CR_RETURN_HR( tex2D->GetSurfaceLevel( levelIx, &dstLevel ) );
			CR( renderContext.m_d3dDevice9->StretchRect( scratch, &dstRect, dstLevel, &dstRect, D3DTEXF_NONE ) );
			srcLevel = dstLevel;

			srcRect = dstRect;
			dstRect.right = std::max( dstRect.right / 2, 1l );
			dstRect.bottom = std::max( dstRect.bottom / 2, 1l );
		}
		return S_OK;
	}

	ALResult Tr2TextureAL::Resolve( Tr2TextureAL& destination, Tr2RenderContextAL& renderContext )
	{
		if( m_msaa.samples <= 1 )
		{
			return destination.CopySubresourceRegion( Tr2TextureSubresource(), *this, Tr2TextureSubresource(), renderContext );
		}

		if( !IsValid() || !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( !destination.IsValid() )
		{
			return E_INVALIDARG;
		}
		if( !HasFlag( destination.m_cpuUsage, Tr2CpuUsage::WRITE ) && !IsWritable( destination.m_gpuUsage ) )
		{
			return E_INVALIDARG;
		}
		if( m_desc.GetWidth() != destination.m_desc.GetWidth() || m_desc.GetHeight() != destination.m_desc.GetHeight() )
		{
			return E_INVALIDARG;
		}
		if( m_desc.GetFormat() != destination.m_desc.GetFormat() )
		{
			return E_INVALIDARG;
		}
		if( destination.m_msaa.samples > 1 )
		{
			return E_INVALIDARG;
		}

		CComPtr<IDirect3DSurface9>	dst;
		if( destination.m_texture )
		{
			CComQIPtr<IDirect3DTexture9> tex2D( destination.m_texture );
			if( !tex2D )
			{
				return E_FAIL;
			}
			CR_RETURN_HR( tex2D->GetSurfaceLevel( 0, &dst ) );
		}
		else
		{
			dst = destination.m_surface;
		}
		if( !dst )
		{
			return E_FAIL;
		}

		CComPtr<IDirect3DSurface9>	src;
		if( m_texture )
		{
			CComQIPtr<IDirect3DTexture9> tex2D( m_texture );
			if( !tex2D )
			{
				return E_FAIL;
			}
			CR_RETURN_HR( tex2D->GetSurfaceLevel( 0, &src ) );
		}
		else
		{
			src = m_surface;
		}
		if( !src )
		{
			return E_FAIL;
		}
		return renderContext.m_d3dDevice9->StretchRect( src, nullptr, dst, nullptr, D3DTEXF_POINT );
	}

	uintptr_t Tr2TextureAL::GetSharedHandle() const
	{
		return reinterpret_cast<uintptr_t>( m_sharedHandle );
	}

	ALResult Tr2TextureAL::Attach( IDirect3DSurface9* surface, Tr2PrimaryRenderContextAL& renderContext )
	{
		CCP_UNUSED( renderContext );

		Destroy();
		if( !surface )
		{
			return S_OK;
		}

		D3DSURFACE_DESC desc;
		CR_RETURN_HR( surface->GetDesc( &desc ) );

		m_surface = surface;

		m_desc = Tr2BitmapDimensions(
			Tr2RenderContextEnum::TEX_TYPE_2D,
			Tr2RenderContextAL::ConvertFromD3D9Format( desc.Format ),
			desc.Width,
			desc.Height,
			1,
			1
		);
		m_msaa = Tr2MsaaDesc( desc.MultiSampleType == 0 ? 1 : desc.MultiSampleType, desc.MultiSampleQuality );
		m_gpuUsage = Tr2GpuUsage::RENDER_TARGET;
		m_cpuUsage = m_msaa.samples > 1 ? Tr2CpuUsage::NONE : Tr2CpuUsage::READ;

		m_format = desc.Format;
		m_pool = desc.Pool;

		m_memory.Set( Tr2MemoryCounterAL::TEXTURE, m_desc, m_msaa );

		return S_OK;
	}


	ALResult Tr2TextureAL::GetSurfaceLevel( CComPtr<IDirect3DSurface9>& surface, uint32_t face, uint32_t mip, Tr2RenderContextAL& renderContext )
	{
		if( HasFlag( m_gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
		{
			const uint32_t width = m_desc.GetMipWidth( mip );
			const uint32_t height = m_desc.GetMipHeight( mip );

			if( surface )
			{
				D3DSURFACE_DESC desc;
				if( FAILED( surface->GetDesc( &desc ) ) || desc.Width != width || desc.Height != height || desc.Format != m_format )
				{
					surface = nullptr;
				}
			}

			if( !surface )
			{
				CR_RETURN_HR( renderContext.m_d3dDevice9->CreateOffscreenPlainSurface( width, height, m_format, D3DPOOL_SYSTEMMEM, &surface, nullptr ) );
				if( !surface )
				{
					return E_FAIL;
				}
			}

			const bool isAutoGen = m_desc.GetMipCount() == 0 && !m_mipGeneratedRT;

			if( isAutoGen && mip > 0 )
			{
				// Need to go the long route of first rendering the renderTarget into a new, non-mipmapped renderTarget, and then
				// getting the data out of that second RT... :|

				CComPtr<IDirect3DTexture9>	nonMipRT;
				CComPtr<IDirect3DSurface9>	nonMipSurface;
				{
					CComPtr<IDirect3DSurface9>	oldRT, oldDS;

					CR_RETURN_HR( renderContext.m_d3dDevice9->CreateTexture( width, height, 1, D3DUSAGE_RENDERTARGET, m_format, D3DPOOL_DEFAULT, &nonMipRT, nullptr ) );
					if( !nonMipRT )
					{
						return E_FAIL;
					}
					CR_RETURN_HR( nonMipRT->GetSurfaceLevel( 0, &nonMipSurface ) );

					renderContext.m_frameDelayedDX9Objects.push_back( CComPtr<IUnknown>( nonMipRT ) );	// workaround - if FXAA is on, bad things happen if we destroy the RT asap. So wait until end of frame.

					CR_RETURN_HR( renderContext.InternalBlit( nonMipSurface, m_texture, width, height ) );
				}

				return renderContext.m_d3dDevice9->GetRenderTargetData( nonMipSurface, surface );
			}
			else if( mip == 0 )
			{
				return renderContext.m_d3dDevice9->GetRenderTargetData( m_surface, surface );
			}
			else
			{
				CComQIPtr<IDirect3DTexture9> texture( m_texture );
				if( !texture )
				{
					return E_FAIL;
				}
				CComPtr<IDirect3DSurface9> RT;
				CR_RETURN_HR( texture->GetSurfaceLevel( mip, &RT ) );
				return renderContext.m_d3dDevice9->GetRenderTargetData( RT, surface );
			}
		}
		else
		{
			surface = nullptr;

			if( m_desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_2D )
			{
				CComQIPtr<IDirect3DTexture9> tex2D( m_texture );
				if( !tex2D )
				{
					return E_FAIL;
				}
				return tex2D->GetSurfaceLevel( mip, &surface );
			}
			else if( m_desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
			{
				CComQIPtr<IDirect3DCubeTexture9> texCube( m_texture );
				if( !texCube )
				{
					return E_FAIL;
				}
				return texCube->GetCubeMapSurface( D3DCUBEMAP_FACES( face ), mip, &surface );
			}
			else
			{
				return E_FAIL;
			}
		}
	}

	ALResult Tr2TextureAL::Lock( const Tr2TextureSubresource& region, uint32_t lockType, void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
	{
		if( !IsValid() || !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( !region.IsValidForBitmap( m_desc ) )
		{
			return E_INVALIDARG;
		}
		if( !region.IsSingleSubresource() )
		{
			return E_INVALIDARG;
		}

		CComPtr<IDirect3DSurface9> surface = m_lockedSurface;
		FORWARD_HR( GetSurfaceLevel( surface, region.m_startFace, region.m_startMipLevel, renderContext ) );
		D3DLOCKED_RECT lr;
		HRESULT hr;
		if( !region.HasBox() )
		{
			hr = surface->LockRect( &lr, nullptr, lockType );
		}
		else
		{
			const RECT r =
			{
				(LONG)region.m_left,
				(LONG)region.m_top,
				(LONG)region.m_right,
				(LONG)region.m_bottom
			};
			hr = surface->LockRect( &lr, &r, lockType );
		}
		if( SUCCEEDED( hr ) && lr.pBits )
		{
			data = lr.pBits;
			pitch = lr.Pitch;
			m_lockedSurface = surface;
		}
		return hr;
	}

	void Tr2TextureAL::Unlock()
	{
		m_lockedSurface->UnlockRect();
		if( !Tr2CpuUsage::HasFlag( m_cpuUsage, Tr2CpuUsage::READ_OFTEN ) )
		{
			m_lockedSurface = nullptr;
		}
	}

	void Tr2TextureAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2TextureAL";

		unsigned size = 0;
		for( unsigned i = 0; i < m_desc.GetTrueMipCount(); ++i )
		{
			size += m_desc.GetMipSize( i );
		}

		description["size"] = std::to_string( long long( size ) );
		description["width"] = std::to_string( long long( m_desc.GetWidth() ) );
		description["height"] = std::to_string( long long( m_desc.GetHeight() ) );
		description["mipLevels"] = std::to_string( long long( m_desc.GetTrueMipCount() ) );
		description["format"] = std::to_string( long long( m_desc.GetFormat() ) );
	}
}






#endif
