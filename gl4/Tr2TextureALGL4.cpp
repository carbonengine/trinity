#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

#include "Tr2TextureALGL4.h"
#include "Tr2RenderContextGL4.h"
#include "BcDecompress.h"
#include "ALLog.h"



namespace
{
	ALResult FillInitialData( GLenum target, GLenum internalFormat, GLenum targetFormat, GLenum targetType, const Tr2BitmapDimensions& desc, Tr2SubresourceData* initialData )
	{
		const uint32_t trueMipLevelCount = desc.GetTrueMipCount();

		for( uint32_t i = 0; i != trueMipLevelCount; ++i )
		{
			uint32_t levelWidth = std::max( desc.GetWidth() >> i, 1U );
			uint32_t levelHeight = std::max( desc.GetHeight() >> i, 1U );
			if( targetType )
			{
				GL_FAIL( glTexSubImage2D(
					target,
					i,
					0,
					0,
					levelWidth,
					levelHeight,
					targetFormat,
					targetType,
					initialData[i].m_sysMem ) );
			}
			else
			{
				GL_FAIL( glCompressedTexSubImage2D(
					target,
					i,
					0,
					0,
					levelWidth,
					levelHeight,
					internalFormat,
					initialData[i].m_sysMemSlicePitch,
					initialData[i].m_sysMem ) );
			}
		}
		return S_OK;
	}


	ALResult Create2D( GLuint& texture, const Tr2BitmapDimensions& desc, Tr2SubresourceData* initialData )
	{
		GLenum internalFormat, targetFormat, targetType;
		if( !Tr2RenderContextAL::ConvertToGLPixelFormat( desc.GetFormat(), internalFormat, targetFormat, targetType ) )
		{
			return E_INVALIDARG;
		}

		GL_FAIL( glGenTextures( 1, &texture ) );
		bool succeeded = false;
		ON_BLOCK_EXIT( [&] { if( !succeeded ) glDeleteTextures( 1, &texture ); } );


		GL_FAIL( glBindTexture( GL_TEXTURE_2D, texture ) );
		GL_FAIL( glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ) );
		GL_VALIDATE( glTexStorage2D( GL_TEXTURE_2D, desc.GetTrueMipCount(), internalFormat, desc.GetWidth(), desc.GetHeight() ) );

		if( initialData )
		{
			FORWARD_HR( FillInitialData( GL_TEXTURE_2D, internalFormat, targetFormat, targetType, desc, initialData ) );
		}

		if( desc.GetTrueMipCount() == 1 )
		{
			CR_GL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
		}
		succeeded = true;
		return S_OK;
	}

	ALResult CreateCube( GLuint& texture, const Tr2BitmapDimensions& desc, Tr2SubresourceData* initialData )
	{
		GLenum internalFormat, targetFormat, targetType;
		if( !Tr2RenderContextAL::ConvertToGLPixelFormat( desc.GetFormat(), internalFormat, targetFormat, targetType ) )
		{
			return E_INVALIDARG;
		}

		GL_FAIL( glGenTextures( 1, &texture ) );
		bool succeeded = false;
		ON_BLOCK_EXIT( [&] { if( !succeeded ) glDeleteTextures( 1, &texture ); } );
		GL_FAIL( glBindTexture( GL_TEXTURE_CUBE_MAP, texture ) );
		GL_FAIL( glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ) );
		GL_VALIDATE( glTexStorage2D( GL_TEXTURE_CUBE_MAP, desc.GetTrueMipCount(), internalFormat, desc.GetWidth(), desc.GetHeight() ) );

		const uint32_t trueMipLevelCount = desc.GetTrueMipCount();
		if( initialData )
		{
			for( uint32_t face = 0; face < 6; ++face )
			{
				FORWARD_HR( FillInitialData( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, internalFormat, targetFormat, targetType, desc, initialData + face * trueMipLevelCount ) );
			}
		}

		if( trueMipLevelCount == 1 )
		{
			CR_GL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
		}
		succeeded = true;
		return S_OK;
	}

	ALResult Create3D( GLuint& texture, const Tr2BitmapDimensions& desc, Tr2SubresourceData* initialData )
	{
		GLenum internalFormat, targetFormat, targetType;
		if( !Tr2RenderContextAL::ConvertToGLPixelFormat( desc.GetFormat(), internalFormat, targetFormat, targetType ) )
		{
			return E_INVALIDARG;
		}

		GL_FAIL( glGenTextures( 1, &texture ) );
		if( !texture )
		{
			return E_FAIL;
		}
		bool succeeded = false;
		bool decompress = false;
		ON_BLOCK_EXIT( [&] { if( !succeeded ) glDeleteTextures( 1, &texture ); } );

		GL_FAIL( glBindTexture( GL_TEXTURE_3D, texture ) );
		GL_FAIL( glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ) );
		glTexStorage3D( GL_TEXTURE_3D, desc.GetTrueMipCount(), internalFormat, desc.GetWidth(), desc.GetHeight(), desc.GetDepth() );
		if( glGetError() && IsCompressedFormat( desc.GetFormat() ) )
		{
			Tr2RenderContextAL::ConvertToGLPixelFormat( Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, internalFormat, targetFormat, targetType );
			GL_VALIDATE( glTexStorage3D( GL_TEXTURE_3D, desc.GetTrueMipCount(), internalFormat, desc.GetWidth(), desc.GetHeight(), desc.GetDepth() ) );
			decompress = true;
		}


		const uint32_t trueMipLevelCount = desc.GetTrueMipCount();
		std::unique_ptr<uint8_t[]> decompressed;

		for( uint32_t i = 0; i != trueMipLevelCount; ++i )
		{
			uint32_t levelWidth = std::max( desc.GetWidth() >> i, 1U );
			uint32_t levelHeight = std::max( desc.GetHeight() >> i, 1U );
			uint32_t levelDepth = std::max( desc.GetDepth() >> i, 1U );
			if( decompress )
			{
				if( !BcDecompress( levelWidth, levelHeight, levelDepth, desc.GetFormat(), initialData[i], decompressed ) )
				{
					return E_FAIL;
				}
				GL_VALIDATE( glTexSubImage3D( GL_TEXTURE_3D, i, 0, 0, 0, levelWidth, levelHeight, levelDepth,
					targetFormat, targetType, decompressed.get() ) );
			}
			else if( targetType )
			{
				GL_VALIDATE( glTexSubImage3D( GL_TEXTURE_3D, i, 0, 0, 0, levelWidth, levelHeight, levelDepth,
					targetFormat, targetType, initialData[i].m_sysMem ) );
			}
			else
			{
#ifndef __APPLE__
				if( GLEW_NV_texture_compression_vtc )
				{
					std::unique_ptr<uint8_t[]> level( new uint8_t[initialData[i].m_sysMemSlicePitch * levelDepth] );
					int blockSize = targetFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ? 16 : 8;
					int cw = int( levelWidth + 3 ) / 4;
					int ch = int( levelHeight + 3 ) / 4;
					int d = ( levelDepth & ~0x3 );
					for( int z = 0; z < int( levelDepth ); ++z )
					{
						const uint8_t* src = static_cast<const uint8_t*>( initialData[i].m_sysMem ) + initialData[i].m_sysMemSlicePitch * z;
						for( int y = 0; y < ch; ++y )
						{
							for( int x = 0; x < cw; ++x )
							{
								int blockOffset;
								if( z >= d )
								{
									blockOffset = blockSize * ( cw * ch * d + x + cw * ( y + ch * ( z - d ) ) );
								}
								else
								{
									blockOffset = blockSize * 4 * ( x + cw * ( y + ch * ( z / 4 ) ) );
								}
								blockOffset += ( z % 4 ) * blockSize;
								memcpy( level.get() + blockOffset, src, blockSize );
								src += blockSize;
							}
						}
					}
					GL_VALIDATE( glCompressedTexSubImage3D( GL_TEXTURE_3D, i, 0, 0, 0, levelWidth, levelHeight, levelDepth, internalFormat,
						initialData[i].m_sysMemSlicePitch * levelDepth, level.get() ) );
				}
				else
#endif
				{
					GL_VALIDATE( glCompressedTexSubImage3D( GL_TEXTURE_3D, i, 0, 0, 0, levelWidth, levelHeight, levelDepth, internalFormat,
						initialData[i].m_sysMemSlicePitch * levelDepth, initialData[i].m_sysMem ) );
				}
			}
		}


		if( trueMipLevelCount == 1 )
		{
			CR_GL( glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
		}
		succeeded = true;
		return S_OK;
	}

	ALResult CreateDepthStencil( GLuint& texture, GLuint& msaaBuffer, bool readable, const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2SubresourceData* initialData )
	{
		texture = 0;
		msaaBuffer = 0;

		bool succeeded = false;
		ON_BLOCK_EXIT( [&] {
			if( !succeeded )
			{
				if( texture )
				{
					glDeleteTextures( 1, &texture );
					texture = 0;
				}
				if( msaaBuffer )
				{
					glDeleteRenderbuffers( 1, &msaaBuffer );
					msaaBuffer = 0;
				}
			}
		} );

		if( readable )
		{
			GL_FAIL( glGenTextures( 1, &texture ) );
			GL_FAIL( glBindTexture( GL_TEXTURE_2D, texture ) );
			GL_VALIDATE( glTexStorage2D( GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, desc.GetWidth(), desc.GetHeight() ) );
		}
		else
		{
			GLenum glDepthFormat = 0, glStencilFormat = 0;

			switch( desc.GetFormat() )
			{
			case Tr2RenderContextEnum::PIXEL_FORMAT_D24_UNORM_S8_UINT:
				glDepthFormat = GL_DEPTH24_STENCIL8;
				break;
			default:
				return E_INVALIDARG;
			}

			GL_FAIL( glGenRenderbuffers( 1, &msaaBuffer ) );
			GL_FAIL( glBindRenderbuffer( GL_RENDERBUFFER, msaaBuffer ) );
			GL_FAIL( glRenderbufferStorage( GL_RENDERBUFFER, glDepthFormat, desc.GetWidth(), desc.GetHeight() ) );
		}
		succeeded = true;
		return S_OK;
	}
}

namespace TrinityALImpl
{
	Tr2TextureAL::Tr2TextureAL() :
		m_texture( 0 ),
		m_msaaBuffer( 0 ),
		m_gpuUsage( Tr2GpuUsage::NONE ),
		m_cpuUsage( Tr2CpuUsage::NONE ),
		m_clObject( nullptr )
	{
	}

	ALResult Tr2TextureAL::Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( HasBufferFlags( gpuUsage ) )
		{
			return E_INVALIDARG;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			return E_INVALIDARG;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) && HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			return E_INVALIDARG;
		}

#if defined(TRINITY_AL_MOBILE)
		if( msaa.samples > 1 )
		{
			return E_INVALIDARG;
		}
#endif
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
		{
			if( msaa.samples > 1 )
			{
				if( desc.GetTrueMipCount() > 1 )
				{
					return E_INVALIDARG;
				}
				if( cpuUsage != Tr2CpuUsage::NONE )
				{
					return E_INVALIDARG;
				}
			}
			if( desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D )
			{
				return E_INVALIDARG;
			}
			if( desc.GetArraySize() > 1 )
			{
				return E_INVALIDARG;
			}
			if( HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) )
			{
				return E_INVALIDARG;
			}
			if( initialData )
			{
				return E_INVALIDARG;
			}
		}
		else if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			if( desc.GetTrueMipCount() > 1 )
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
			if( desc.GetArraySize() > 1 )
			{
				return E_INVALIDARG;
			}
			if( initialData )
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			if( HasFlag( cpuUsage, Tr2CpuUsage::READ ) )
			{
				return E_INVALIDARG;
			}
			if( msaa.samples > 1 )
			{
				return E_INVALIDARG;
			}
			if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
			{
				if( desc.GetArraySize() != 6 )
				{
					return E_INVALIDARG;
				}
			}
			else
			{
				if( desc.GetArraySize() > 1 )
				{
					return E_INVALIDARG;
				}
			}
			if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D )
			{
				if( cpuUsage != Tr2CpuUsage::NONE )
				{
					return E_INVALIDARG;
				}
			}
			if( !HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) && !initialData )
			{
				return E_INVALIDARG;
			}
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::SHARED ) )
		{
			return E_INVALIDARG;
		}

		if( !renderContext.IsValid() )
		{
			return E_FAIL;
		}

		GLuint texture = 0;
		GLuint msaaBuffer = 0;
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) && msaa.samples > 1 )
		{
			GLenum internalFormat, targetFormat, targetType;
			if( !Tr2RenderContextAL::ConvertToGLPixelFormat( desc.GetFormat(), internalFormat, targetFormat, targetType ) )
			{
				return E_INVALIDARG;
			}

			CR_GL( glGenRenderbuffers( 1, &msaaBuffer ) );
			CR_GL( glBindRenderbuffer( GL_RENDERBUFFER, msaaBuffer ) );
			CR_GL( glRenderbufferStorageMultisample( GL_RENDERBUFFER, msaa.samples, internalFormat, desc.GetWidth(), desc.GetHeight() ) );
			CR_GL( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );
		}
		else if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			FORWARD_HR( CreateDepthStencil( texture, msaaBuffer, HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ), desc, msaa, initialData ) );
		}
		else if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D )
		{
			FORWARD_HR( Create3D( texture, desc, initialData ) );
		}
		else if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
		{
			FORWARD_HR( CreateCube( texture, desc, initialData ) );
		}
		else
		{
			FORWARD_HR( Create2D( texture, desc, initialData ) );
		}

		m_texture = texture;
		m_msaaBuffer = msaaBuffer;

		m_desc = desc;
		m_msaa = msaa;
		m_gpuUsage = gpuUsage;
		m_cpuUsage = cpuUsage;
		m_memory.Set( Tr2MemoryCounterAL::TEXTURE, desc, msaa );

		return S_OK;
	}

	void Tr2TextureAL::Destroy()
	{
		if( m_texture )
		{
			glDeleteTextures( 1, &m_texture );
			m_texture = 0;
		}
		if( m_msaaBuffer )
		{
			glDeleteRenderbuffers( 1, &m_msaaBuffer );
			m_msaaBuffer = 0;
		}
		if( m_clObject )
		{
			clReleaseMemObject( m_clObject );
			m_clObject = nullptr;
		}
		m_memory.Reset();
		memset( &m_desc, 0, sizeof( m_desc ) );
		m_msaa = Tr2MsaaDesc();
		m_gpuUsage = Tr2GpuUsage::NONE;
		m_cpuUsage = Tr2CpuUsage::NONE;
	}

	bool Tr2TextureAL::IsValid() const
	{
		return m_texture != 0 || m_msaaBuffer != 0;
	}

	Tr2ALMemoryType Tr2TextureAL::GetMemoryClass()
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

		GLint x, y, width, height;
		if( region.HasBox() )
		{
			x = GLint( region.m_left );
			y = GLint( region.m_bottom );
			width = GLint( region.m_right ) - x;
			height = y - GLint( region.m_top );
		}
		else
		{
			x = 0;
			y = 0;
			width = m_desc.GetWidth();
			height = m_desc.GetHeight();
		}
		if( width <= 0 || height <= 0 )
		{
			return E_INVALIDARG;
		}

		renderContext.PushRenderTarget();
		renderContext.PushDepthStencil();
		ON_BLOCK_EXIT(
			[&] {
			renderContext.PopDepthStencil();
			renderContext.PopRenderTarget();
		} );

		::Tr2TextureAL rt;
		rt.m_texture.reset( this, []( Tr2TextureAL* ) {} );

		renderContext.SetRenderTarget( rt );
		renderContext.SetDepthStencil( ::Tr2TextureAL() );

		int bpp = Tr2RenderContextEnum::GetBytesPerPixel( m_desc.GetFormat() );
		size_t newSize = width * height * bpp;
		if( m_lockedData.empty() || m_lockedData.size() < newSize )
		{
			m_lockedData.resize( "Tr2TextureAL::m_lockedData", newSize );
		}
		if( m_lockedData.empty() )
		{
			return E_FAIL;
		}
		GLenum internalFormat, format, type;
		Tr2RenderContextAL::ConvertToGLPixelFormat( m_desc.GetFormat(), internalFormat, format, type );
		GL_FAIL( glReadPixels( x, y, width, height, format, type, m_lockedData.get() ) );
		pitch = width * bpp;
		data = m_lockedData.get();
		return S_OK;
	}

	void Tr2TextureAL::UnmapForReading( Tr2RenderContextAL& renderContext )
	{
		if( m_lockedData.get() && !HasFlag( m_cpuUsage, Tr2CpuUsage::READ_OFTEN ) )
		{
			m_lockedData.clear();
		}
	}

	ALResult Tr2TextureAL::MapForWriting( const Tr2TextureSubresource& region, void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
	{
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

		m_lockedRegion = region;
		uint32_t width, height;
		if( region.HasBox() )
		{
			width = region.m_right - region.m_left;
			height = region.m_bottom - region.m_top;
		}
		else
		{
			width = m_desc.GetMipWidth( region.m_startMipLevel );
			height = m_desc.GetMipHeight( region.m_startMipLevel );
		}

		if( IsCompressedFormat( m_desc.GetFormat() ) )
		{
			pitch = width * GetBlockByteSize( m_desc.GetFormat() ) / 16;
			m_lockedData.resize( "Tr2TextureAL::m_lockedData", pitch * height );
		}
		else
		{
			pitch = width * GetBytesPerPixel( m_desc.GetFormat() );
			m_lockedData.resize( "Tr2TextureAL::m_lockedData", pitch * height );
		}
		data = m_lockedData.get();

		return S_OK;
	}

	void Tr2TextureAL::UnmapForWriting( Tr2RenderContextAL& renderContext )
	{
		if( !m_lockedData.get() )
		{
			return;
		}

		GLenum texType = m_desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + m_lockedRegion.m_startFace : GL_TEXTURE_2D;
		GLenum internalFormat, targetFormat, targetType;
		if( !Tr2RenderContextAL::ConvertToGLPixelFormat( m_desc.GetFormat(), internalFormat, targetFormat, targetType ) )
		{
			return;
		}

		glBindTexture( texType, m_texture );

		GLint x, y, width, height;
		if( m_lockedRegion.HasBox() )
		{
			x = m_lockedRegion.m_left;
			y = m_lockedRegion.m_top;
			width = m_lockedRegion.m_right - x;
			height = m_lockedRegion.m_bottom - y;
		}
		else
		{
			x = 0;
			y = 0;
			width = m_desc.GetMipWidth( m_lockedRegion.m_startMipLevel );
			height = m_desc.GetMipHeight( m_lockedRegion.m_startMipLevel );
		}

		if( targetType )
		{
			glTexSubImage2D( texType,
				m_lockedRegion.m_startMipLevel,
				x,
				y,
				width,
				height,
				targetFormat,
				targetType,
				m_lockedData.get() );
		}
		else
		{
			glCompressedTexSubImage2D( texType,
				m_lockedRegion.m_startMipLevel,
				x,
				y,
				width,
				height,
				internalFormat,
				width * height * GetBlockByteSize( m_desc.GetFormat() ) / 16,
				m_lockedData.get() );
		}
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			m_lockedData.clear();
		}
	}

	ALResult Tr2TextureAL::UpdateSubresource( const Tr2TextureSubresource& region, const void* source, uint32_t pitch, uint32_t slicePitch, Tr2RenderContextAL& renderContext )
	{
		if( !HasFlag( m_cpuUsage, Tr2CpuUsage::WRITE ) )
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

		if( !source )
		{
			return E_FAIL;
		}

		GL_FAIL( glBindTexture( GL_TEXTURE_2D, m_texture ) );

		GLenum internalFormat, targetFormat, targetType;
		if( !Tr2RenderContextAL::ConvertToGLPixelFormat( m_desc.GetFormat(), internalFormat, targetFormat, targetType ) )
		{
			return E_FAIL;
		}
		GLint x, y, width, height;
		if( m_lockedRegion.HasBox() )
		{
			x = m_lockedRegion.m_left;
			y = m_lockedRegion.m_top;
			width = m_lockedRegion.m_right - x;
			height = m_lockedRegion.m_bottom - y;
		}
		else
		{
			x = 0;
			y = 0;
			width = m_desc.GetMipWidth( m_lockedRegion.m_startMipLevel );
			height = m_desc.GetMipHeight( m_lockedRegion.m_startMipLevel );
		}
		if( targetType )
		{
			GL_FAIL( glTexSubImage2D( GL_TEXTURE_2D, 0, x, y, width, height, targetFormat, targetType, source ) );
		}
		else
		{
			GL_FAIL( glCompressedTexSubImage2D( GL_TEXTURE_2D, 0, x, y, width, height, targetFormat, pitch * height, source ) );
		}

		return S_OK;
	}

	ALResult Tr2TextureAL::CopySubresourceRegion( const Tr2TextureSubresource& destSubresource, Tr2TextureAL& source, const Tr2TextureSubresource& sourceSubresource, Tr2RenderContextAL& renderContext )
	{
		return renderContext.CopySubresourceRegion( *this, destSubresource, source, sourceSubresource );
	}

	ALResult Tr2TextureAL::GenerateMipMaps( Tr2RenderContextAL& renderContext )
	{
		if( !HasFlag( m_gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
		{
			return E_INVALIDCALL;
		}

		if( m_desc.GetTrueMipCount() == 1 )
		{
			return S_OK;
		}

		if( !IsValid() )
		{
			return E_FAIL;
		}
		AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
		GL_FAIL( glBindTexture( GL_TEXTURE_2D, m_texture ) );
#if !defined(TRINITY_AL_MOBILE)
		GL_IGNORE_ERROR( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT ) );
#endif
		GL_FAIL( glGenerateMipmap( GL_TEXTURE_2D ) );

		return S_OK;
	}

	ALResult Tr2TextureAL::Resolve( Tr2TextureAL& destination, Tr2RenderContextAL& renderContext )
	{
		return renderContext.InternalResolveRT( destination, *this );
	}

	uintptr_t Tr2TextureAL::GetSharedHandle() const
	{
		return 0;
	}
}


#endif
