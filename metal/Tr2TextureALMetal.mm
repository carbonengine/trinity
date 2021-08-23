#include "StdAfx.h"
#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2TextureALMetal.h"
#include "Tr2RenderContextMetal.h"
#include "MetalContext.h"
#include "ALLog.h"
#include "../BcDecompress.h"


namespace TrinityALImpl
{
	Tr2TextureAL::Tr2TextureAL()
		: m_gpuUsage( Tr2GpuUsage::NONE )
		, m_cpuUsage( Tr2CpuUsage::NONE )
		, m_dataLayout( {nullptr, 0, 0} )
        , m_mtlWriteBuffer( nil )
        , m_mappedRange{ 0, 0, 0 }
		, m_mtlReadBackBuffer( nil )
		, m_mtlTexture( nil )
		, m_mtlTextureSRGBView( nil )
		, m_metalContext( nullptr )
		, m_wrappedTexture( false )
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
		if( msaa.samples > 1 )
		{
			if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
			{
				return E_INVALIDARG;
			}
			if( cpuUsage != Tr2CpuUsage::NONE )
			{
				return E_INVALIDARG;
			}
			if( desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D )
			{
				return E_INVALIDARG;
			}
		}
		if( desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D )
		{
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
		}
		if( desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D && HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			return E_INVALIDARG;
		}
		if( msaa.samples > 1 && desc.GetTrueMipCount() > 1 )
		{
			return E_INVALIDARG;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) && HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) )
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
		if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D && cpuUsage != Tr2CpuUsage::NONE )
		{
			return E_INVALIDARG;
		}
		if( !IsWritable( gpuUsage ) && !HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) && !initialData )
		{
			return E_INVALIDARG;
		}

		// TODO: MTLTextureUsagePixelFormatView has been removed but might be necessary in the future (e.g. if we need a special pixel format/component layout).
		MTLTextureUsage metalTextureUsage = 0;

		MetalContext *metalContext = renderContext.GetMetalContext();
		MetalWorkQueue* workQueue = renderContext.GetMetalWorkQueue();

		// It seems that trinity will assume that a depth stencil can be used as a render target and doesn't set the RENDER_TARGET flag
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) ||
			HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			metalTextureUsage |= MTLTextureUsageRenderTarget;
		}

		if( HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			metalTextureUsage |= MTLTextureUsageShaderRead;
		}

		if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			metalTextureUsage |= MTLTextureUsageShaderWrite;
		}

		MTLPixelFormat metalPixelFormat = metalContext->m_utils->GetMTLPixelFormat( desc.GetFormat() );

		if( metalPixelFormat == MTLPixelFormatInvalid )
		{
			return E_INVALIDARG;
		}

		MTLTextureType metalTextureType = metalContext->m_utils->GetMTLTextureType( desc.GetType(), desc.GetArraySize(), msaa.samples );
		MTLStorageMode metalStorageMode = MTLStorageModePrivate;

#if 0
		// Might want to revisit this wrapping a texture approach and seperate it out. Use this now for expediency.
		if( texture )
		{
			m_mtlTexture = texture;
			m_wrappedTexture = true;

			CCP_ASSERT(texture.width == desc.GetWidth() && texture.height == desc.GetHeight());
		}
		else
#endif
		bool needsDecompression = false;
		
		// macOS 10.14 can't handle compressed volume textures, so we decompress them on the fly
		// This only works with an assumption that we have BC 1, 2, 3 compression only and that such
		// textures are immutable textures only participating in draw commands (not copy/map, etc.)
		if( @available( macOS 10.15, * ) )
		{
		}
		else
		{
			if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D && IsCompressedFormat( desc.GetFormat() ) )
			{
				metalPixelFormat = MTLPixelFormatBGRA8Unorm;
				needsDecompression = true;
			}
		}
		
		{
			uint32_t mipMapCount = desc.GetTrueMipCount();

			uint32_t bytesPerRow    = 0;
			uint32_t bytesPerSlice  = 0;
			const void *textureData = nil;

			if( initialData )
			{
				textureData   = initialData->m_sysMem;
				bytesPerRow   = initialData->m_sysMemPitch;
				bytesPerSlice = initialData->m_sysMemSlicePitch;
			}

			m_mtlTexture = metalContext->CreateMetalTexture(metalTextureType,
															metalPixelFormat,
															desc.GetWidth(),
															desc.GetHeight(),
															desc.GetDepth(),
															mipMapCount,
															metalStorageMode,
															metalTextureUsage,
															msaa.samples,
															desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE ? 1 : desc.GetArraySize());
            
            if( initialData )
            {
                uint32_t sliceCount = desc.GetArraySize();
                uint32_t index = 0;
				std::unique_ptr<uint8_t[]> decompressed;
				for( uint32_t slice = 0; slice < sliceCount; ++slice )
				{
					for( uint32_t mip = 0; mip < mipMapCount; ++mip )
					{
						if( needsDecompression )
						{
							uint32_t levelWidth = std::max( desc.GetWidth() >> mip, 1U );
							uint32_t levelHeight = std::max( desc.GetHeight() >> mip, 1U );
							uint32_t levelDepth = std::max( desc.GetDepth() >> mip, 1U );
							if( !BcDecompress( levelWidth, levelHeight, levelDepth, desc.GetFormat(), initialData[mip], decompressed ) )
							{
								return E_FAIL;
							}
							workQueue->UploadTexture( m_mtlTexture, decompressed.get(), slice, mip, levelWidth * 4, levelWidth * levelHeight * 4, 0, false );
						}
						else
						{
							workQueue->UploadTexture( m_mtlTexture, initialData[index].m_sysMem, slice, mip, initialData[index].m_sysMemPitch, initialData[index].m_sysMemSlicePitch, 0, false );
						}
						++index;
                    }
                }
            }

			m_wrappedTexture = false;
		}

		m_metalContext  = metalContext;
		m_desc          = desc;
		m_gpuUsage      = gpuUsage;
		m_cpuUsage      = cpuUsage;
		m_msaa          = msaa;
		m_mtlTextureUAV.resize( desc.GetTrueMipCount() );
		if( initialData )
		{
			m_dataLayout = *initialData;
		}

		return S_OK;
	}

	id<MTLTexture> Tr2TextureAL::GetSRGBViewMetalTexture()
	{
		if( !m_mtlTextureSRGBView )
		{
			m_mtlTextureSRGBView = m_metalContext->CreateSRGBViewOfMetalTexture( m_mtlTexture );
		}
		return m_mtlTextureSRGBView;
	}
	
	id<MTLTexture> Tr2TextureAL::GetUAVMetalTexture( uint32_t mipLevel )
	{
		if( mipLevel >= m_mtlTextureUAV.size() )
		{
			return nil;
		}
		if( !m_mtlTextureUAV[mipLevel] )
		{
			m_mtlTextureUAV[mipLevel] = m_metalContext->CreateUAVOfMetalTexture( m_mtlTexture, mipLevel );
		}
		return m_mtlTextureUAV[mipLevel];
	}

	ALResult Tr2TextureAL::OpenShared( uintptr_t handle, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext )
	{
		return E_FAIL;
	}

	void Tr2TextureAL::Destroy()
	{
		if( m_mtlTexture )
		{
			m_metalContext->DestroyMetalTexture(m_mtlTexture);
		}
		if( m_mtlTextureSRGBView )
		{
			m_metalContext->DestroyMetalTexture(m_mtlTextureSRGBView);
		}
		for( auto texture : m_mtlTextureUAV )
		{
			m_metalContext->DestroyMetalTexture( texture );
		}
		m_mtlTextureUAV.clear();
        if( m_mtlReadBackBuffer )
        {
            m_metalContext->DestroyMetalBuffer(m_mtlReadBackBuffer);
        }
        if( m_mtlWriteBuffer )
        {
            m_metalContext->DestroyMetalBuffer( m_mtlWriteBuffer );
        }
        m_mappedRanges.clear();
        m_mappedRange.size = 0;

		memset( &m_desc, 0, sizeof( m_desc ) );
		m_msaa = Tr2MsaaDesc();
		m_gpuUsage = Tr2GpuUsage::NONE;
		m_cpuUsage = Tr2CpuUsage::NONE;

		m_dataLayout.m_sysMem = nullptr;
		m_dataLayout.m_sysMemPitch = 0;
		m_dataLayout.m_sysMemSlicePitch = 0;

		m_mtlReadBackBuffer  = nil;
		m_mtlTexture         = nil;
		m_mtlTextureSRGBView = nil;

		m_metalContext   = nil;
		m_wrappedTexture = false;
	}

	bool Tr2TextureAL::IsValid() const
	{
		return m_desc.GetWidth() != 0;
	}

	Tr2ALMemoryType Tr2TextureAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
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
		MetalContext *metalContext = renderContext.GetMetalContext();

		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::READ ) )
		{
			return E_INVALIDCALL;
		}

		if( !IsValid() || !renderContext.IsValid() )
		{
			return E_FAIL;
		}
		if( !region.IsValidForBitmap( m_desc ) )
		{
			return E_INVALIDARG;
		}
		if( !region.IsSingleSubresource() )
		{
			return E_INVALIDARG;
		}

		CCP_ASSERT(region.m_startFace == 0 && region.m_endFace == 1);

		MTLOrigin readOrigin;
		MTLSize   readSize;
		uint32_t readMipLevel  = region.m_startMipLevel;

		if( readMipLevel >= m_mtlTexture.mipmapLevelCount )
		{
			CCP_AL_LOGWARN( "Mip level %d does not exist for this texture. Defaulting to 0.", (int)readMipLevel );
			readMipLevel = 0;
		}

		if( region.HasBox() )
		{
			readOrigin = MTLOriginMake(region.m_left, region.m_bottom, 0);
			readSize   = MTLSizeMake(region.GetWidth(), region.GetHeight(), 1);
		}
		else
		{
			readOrigin = MTLOriginMake(0, 0, 0);
			readSize   = MTLSizeMake(m_desc.GetMipWidth(readMipLevel), m_desc.GetMipHeight(readMipLevel), m_desc.GetMipDepth(readMipLevel));
		}

		auto mipPitch = m_desc.GetMipPitch( readMipLevel );
		auto bufferSize = m_desc.GetMipSize( readMipLevel );

		if( !m_mtlReadBackBuffer )
		{
			m_mtlReadBackBuffer = metalContext->CreateMetalBuffer( renderContext.GetMetalWorkQueue(), bufferSize, MTLResourceStorageModeManaged, nil);
		}
		// We recreate the buffer if it's not an exact match but we could just do this for larger buffers if performance is an issue
		if( m_mtlReadBackBuffer.length != bufferSize )
		{
			metalContext->DestroyMetalBuffer(m_mtlReadBackBuffer);
			m_mtlReadBackBuffer = metalContext->CreateMetalBuffer(renderContext.GetMetalWorkQueue(), bufferSize, MTLResourceStorageModeManaged, nil);
		}

		renderContext.GetMetalWorkQueue()->CopyTextureToMTLBuffer( m_mtlTexture, m_mtlReadBackBuffer, mipPitch, bufferSize / std::max( 1u, m_desc.GetDepth() ), readOrigin, readSize, readMipLevel, true );

		pitch = mipPitch;
		data = m_mtlReadBackBuffer.contents;

		m_dataLayout.m_sysMem = data;
		m_dataLayout.m_sysMemPitch = mipPitch;
		m_dataLayout.m_sysMemSlicePitch = bufferSize;

		return S_OK;
	}

	void Tr2TextureAL::UnmapForReading( Tr2RenderContextAL& renderContext )
	{
#if 0
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::READ_OFTEN ) && !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			// ...
		}
#endif

		m_dataLayout.m_sysMem = nullptr;
		m_dataLayout.m_sysMemPitch = 0;
		m_dataLayout.m_sysMemSlicePitch = 0;
	}

	ALResult Tr2TextureAL::MapForWriting( const Tr2TextureSubresource& region, void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
	{
		MetalContext *metalContext = renderContext.GetMetalContext();
		data = nullptr;

		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			return E_INVALIDCALL;
		}
		if( !IsValid() || !renderContext.IsValid() )
		{
			return E_FAIL;
		}
		if( !region.IsValidForBitmap( m_desc ) )
		{
			return E_INVALIDARG;
		}
		if( !region.IsSingleSubresource() )
		{
			return E_INVALIDARG;
		}
		if( region.HasBox() && Tr2RenderContextEnum::IsCompressedFormat( m_desc.GetFormat() ) )
		{
			return E_INVALIDARG;
		}
        
        CCP_ASSERT(region.m_startFace == 0 && region.m_endFace == 1);

        auto mipPitch = m_desc.GetMipPitch( region.m_startMipLevel );
        auto bufferSize = mipPitch * m_desc.GetMipHeight( region.m_startMipLevel );
        
        if( region.HasBox() )
        {
            auto width = MAX( MIN( region.GetWidth(), m_desc.GetMipWidth( region.m_startMipLevel ) ), 1);
            auto height = MAX( MIN( region.GetHeight(), m_desc.GetMipHeight( region.m_startMipLevel ) ), 1 );
            if( m_desc.IsCompressed() )
            {
                mipPitch = MAX( width / 4u, 1 ) * GetBlockByteSize( m_desc.GetFormat() );
            }
            else
            {
                mipPitch = width * GetBytesPerPixel( m_desc.GetFormat() );
            }
            bufferSize = mipPitch * height;
        }
        
        auto renderedFrame = metalContext->GetRenderedFrameNumber();
        
        while( !m_mappedRanges.empty() )
        {
            auto& r = m_mappedRanges.front();
            if( r.frame < renderedFrame )
            {
                m_mappedRanges.erase( m_mappedRanges.begin() );
            }
            else
            {
                break;
            }
        }
        
        if( !m_mtlWriteBuffer )
        {
            m_mtlWriteBuffer = metalContext->CreateMetalBuffer( renderContext.GetMetalWorkQueue(), m_desc.GetMipSize( 0 ), MTLResourceStorageModeManaged, nil );
        }
        uint32_t start = 0;
        if( !m_mappedRanges.empty() )
        {
            auto head = m_mappedRanges.front().offset;
            auto tail = m_mappedRanges.back().offset + m_mappedRanges.back().size;
            
            if( head < tail )
            {
                if( tail + bufferSize <= m_mtlWriteBuffer.length )
                {
                    start = tail;
                }
                else if( bufferSize <= head )
                {
                    auto remainder = m_mtlWriteBuffer.length - tail;
                    start = 0;
                }
                else
                {
                    metalContext->DestroyMetalBuffer( m_mtlWriteBuffer );
                    m_mtlWriteBuffer = metalContext->CreateMetalBuffer( renderContext.GetMetalWorkQueue(), m_desc.GetMipSize( 0 ) + m_mtlWriteBuffer.length, MTLResourceStorageModeManaged, nil );
                    m_mappedRanges.clear();
                    start = 0;
                }
            }
            else
            {
                if( tail + bufferSize <= head )
                {
                    start = tail;
                }
                else
                {
                    metalContext->DestroyMetalBuffer( m_mtlWriteBuffer );
                    m_mtlWriteBuffer = metalContext->CreateMetalBuffer( renderContext.GetMetalWorkQueue(), m_desc.GetMipSize( 0 ) + m_mtlWriteBuffer.length, MTLResourceStorageModeManaged, nil );
                    m_mappedRanges.clear();
                    start = 0;
                }
            }
        }

        m_mappedRange = { start, bufferSize, metalContext->GetRecordingFrameNumber() };
        m_mappedRanges.push_back( m_mappedRange );

        pitch = mipPitch;
        data = static_cast<uint8_t*>( m_mtlWriteBuffer.contents ) + start;
		
		m_mappedRegion = region;

        m_dataLayout.m_sysMem = data;
        m_dataLayout.m_sysMemPitch = mipPitch;
        m_dataLayout.m_sysMemSlicePitch = bufferSize;

		return S_OK;
	}

	void Tr2TextureAL::UnmapForWriting( Tr2RenderContextAL& renderContext )
	{
        if( m_mappedRange.size == 0 )
        {
            return;
        }

        m_metalContext->IndicateBufferModified( m_mtlWriteBuffer, m_mappedRange.offset, m_mappedRange.size );

		if( m_mappedRegion.HasBox() )
		{
			MTLOrigin origin = MTLOriginMake( m_mappedRegion.m_left, m_mappedRegion.m_top, m_mappedRegion.m_front );

			MTLSize size = MTLSizeMake(
				MAX( MIN( m_mappedRegion.GetWidth(), m_desc.GetMipWidth( m_mappedRegion.m_startMipLevel ) ), 1),
				MAX( MIN( m_mappedRegion.GetHeight(), m_desc.GetMipHeight( m_mappedRegion.m_startMipLevel ) ), 1),
				MAX( MIN( m_mappedRegion.GetDepth(), m_desc.GetMipDepth( m_mappedRegion.m_startMipLevel ) ), 1) );

			renderContext.GetMetalWorkQueue()->UploadTexture(m_mtlTexture, m_mtlWriteBuffer, m_mappedRegion.m_startFace, m_mappedRegion.m_startMipLevel, origin, size, m_dataLayout.m_sysMemPitch, m_dataLayout.m_sysMemSlicePitch, m_mappedRange.offset, false);
		}
		else
		{
			renderContext.GetMetalWorkQueue()->UploadTexture(m_mtlTexture, m_mtlWriteBuffer, m_mappedRegion.m_startFace, m_mappedRegion.m_startMipLevel, m_dataLayout.m_sysMemPitch, m_dataLayout.m_sysMemSlicePitch, m_mappedRange.offset, false);
		}

		m_dataLayout.m_sysMem = nullptr;
		m_dataLayout.m_sysMemPitch = 0;
		m_dataLayout.m_sysMemSlicePitch = 0;
        m_mappedRange.size = 0;
	}

	ALResult Tr2TextureAL::UpdateSubresource( const Tr2TextureSubresource& region, const void* source, uint32_t pitch, uint32_t slicePitch, Tr2RenderContextAL& renderContext )
	{
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

		void* dest;
		uint32_t destPitch;
		FORWARD_HR( MapForWriting( region, dest, destPitch, renderContext ) );
		auto bpp = Tr2RenderContextEnum::GetBytesPerPixel( m_desc.GetFormat() );
		auto width = region.m_right - region.m_left;
		width *= bpp;
		for( uint32_t i = region.m_top; i != region.m_bottom; ++i )
		{
			memcpy( dest, source, width );
			dest = static_cast<uint8_t*>( dest ) + destPitch;
			source = static_cast<const uint8_t*>( source ) + pitch;
		}
		UnmapForWriting( renderContext );

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

		Tr2TextureSubresource src = sourceSubresource;
		Tr2TextureSubresource dst = destSubresource;

		if( !Crop( src, source.m_desc, dst, m_desc ) )
		{
			return E_FAIL;
		}

		MetalContext* metalContext = renderContext.GetMetalContext();
		
		uint32_t slices = src.m_endFace - src.m_startFace;
		uint32_t mips = src.m_endMipLevel - src.m_startMipLevel;
		
		for( uint32_t slice = 0; slice < slices; ++slice )
		{
			for( uint32_t mip = 0; mip < mips; ++mip )
			{
				renderContext.GetMetalWorkQueue()->CopyTextureToTexture(
					source.GetMetalTexture(),
					src.m_startFace + slice,
					src.m_startMipLevel + mip,
					MTLOriginMake( src.m_left, src.m_top, src.m_front ),
					MTLSizeMake( src.GetWidth(), src.GetHeight(), src.GetDepth() ),
					GetMetalTexture(),
					dst.m_startFace + slice,
					dst.m_startMipLevel + mip,
					MTLOriginMake( dst.m_left, dst.m_top, dst.m_front ) );
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

		renderContext.GetMetalWorkQueue()->GenerateMipMaps(m_mtlTexture);

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
		
		renderContext.GetMetalWorkQueue()->ResolveMsaa(GetMetalTexture(), destination.GetMetalTexture());
		
		return S_OK;
	}

	uintptr_t Tr2TextureAL::GetSharedHandle() const
	{
		return 0;
	}

	void Tr2TextureAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2TextureAL";
	}
	
	void Tr2TextureAL::AssignFromTexture( Tr2TextureAL& backBuffer )
	{
		Destroy();
		m_desc = backBuffer.m_desc;
		m_msaa = backBuffer.m_msaa;
		m_gpuUsage = backBuffer.m_gpuUsage;
		m_cpuUsage = backBuffer.m_cpuUsage;
		m_mtlTexture = backBuffer.m_mtlTexture;
		m_mtlTextureSRGBView = backBuffer.m_mtlTextureSRGBView;
		m_metalContext = backBuffer.m_metalContext;
#if !__has_feature(objc_arc)
		[m_mtlTexture retain];
		[m_mtlTextureSRGBView retain];
#endif
	}
}

#endif
