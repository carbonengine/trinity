#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2TextureALGLES2.h"
#include "BcDecompress.h"
#include "ALLog.h"
#include "Tr2RenderContextGLES2.h"

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
				GL_FAIL( glTexImage2D(
					target,
					i,
					internalFormat,
					levelWidth,
					levelHeight,
					0,
					targetFormat,
					targetType,
					initialData ? initialData[i].m_sysMem : nullptr ) );
			}
			else if( initialData )
			{
				GL_FAIL( glCompressedTexImage2D(
					target,
					i,
					internalFormat,
					levelWidth,
					levelHeight,
					0,
					initialData[i].m_sysMemSlicePitch,
					initialData[i].m_sysMem ) );
			}
			else
			{
				GL_FAIL( glTexImage2D(
					target,
					i,
					internalFormat,
					levelWidth,
					levelHeight,
					0,
					GL_RGB,
					GL_UNSIGNED_BYTE,
					nullptr ) );
			}
		}

		if( trueMipLevelCount > 1 )
		{
			// If we have mipmapped texture, but do not provide all mip levels, GL will
			// think of such texture as invalid. So we define lower mip levels leaving
			// them with garbage. ...And GLES does not have a way of limiting mips used(
			auto fullMipCount = uint32_t( 0.5 + log( std::max<double>( desc.GetHeight(), desc.GetWidth() ) ) / log( 2.0 ) ) + 1;
			for( uint32_t i = trueMipLevelCount; i < fullMipCount; ++i )
			{
				uint32_t levelWidth = std::max( desc.GetWidth() >> i, 1U );
				uint32_t levelHeight = std::max( desc.GetHeight() >> i, 1U );
				if( targetType )
				{
					GL_FAIL( glTexImage2D(
						target,
						i,
						internalFormat,
						levelWidth,
						levelHeight,
						0,
						targetFormat,
						targetType,
						nullptr ) );
				}
				else
				{
					GL_FAIL( glCompressedTexImage2D(
						target,
						i,
						internalFormat,
						levelWidth,
						levelHeight,
						0,
						0,
						nullptr ) );
				}
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

		FORWARD_HR( FillInitialData( GL_TEXTURE_2D, internalFormat, targetFormat, targetType, desc, initialData ) );

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

		const uint32_t trueMipLevelCount = desc.GetTrueMipCount();

		for( uint32_t face = 0; face < 6; ++face )
		{
			FORWARD_HR( FillInitialData( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, internalFormat, targetFormat, targetType, desc, initialData ? initialData + face * trueMipLevelCount : nullptr ) );
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
		ON_BLOCK_EXIT( [&] { if( !succeeded ) glDeleteTextures( 1, &texture ); } );

		GL_FAIL( glBindTexture( GL_TEXTURE_3D, texture ) );
		GL_FAIL( glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ) );

		const uint32_t trueMipLevelCount = desc.GetTrueMipCount();

		std::unique_ptr<uint8_t[]> decompressed;

		for( uint32_t i = 0; i != trueMipLevelCount; ++i )
		{
			uint32_t levelWidth = std::max( desc.GetWidth() >> i, 1U );
			uint32_t levelHeight = std::max( desc.GetHeight() >> i, 1U );
			uint32_t levelDepth = std::max( desc.GetDepth() >> i, 1U );
			if( decompressed )
			{
				if( !BcDecompress( levelWidth, levelHeight, levelDepth, desc.GetFormat(), initialData[i], decompressed ) )
				{
					return E_FAIL;
				}
				GL_VALIDATE( glTexImage3D( 
					GL_TEXTURE_3D, 
					i, 
					internalFormat, 
					levelWidth, 
					levelHeight, 
					levelDepth, 
					0,
					targetFormat, 
					targetType, 
					decompressed.get() ) );
			}
			else if( targetType )
			{
				GL_VALIDATE( glTexImage3D( 
					GL_TEXTURE_3D, 
					i, 
					internalFormat, 
					levelWidth, 
					levelHeight, 
					levelDepth, 
					0,
					targetFormat, 
					targetType,
					initialData ? initialData[i].m_sysMem : nullptr ) );
			}
			else if( initialData )
			{
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
					GL_VALIDATE( glCompressedTexImage3D( 
						GL_TEXTURE_3D, 
						i, 
						internalFormat, 
						levelWidth, 
						levelHeight, 
						levelDepth, 
						0,
						initialData[i].m_sysMemSlicePitch * levelDepth,
						level.get() ) );
				}
				else
				{
					glCompressedTexImage3D( 
						GL_TEXTURE_3D, 
						i, 
						internalFormat, 
						levelWidth, 
						levelHeight, 
						levelDepth, 
						0, 
						initialData[i].m_sysMemSlicePitch * levelDepth, 
						initialData[i].m_sysMem );
					if( glGetError() )
					{
						if( BcDecompress( levelWidth, levelHeight, levelDepth, desc.GetFormat(), initialData[i], decompressed ) )
						{
							Tr2RenderContextAL::ConvertToGLPixelFormat( Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, internalFormat, targetFormat, targetType );
							GL_VALIDATE( glTexImage3D( 
								GL_TEXTURE_3D, 
								i, 
								internalFormat, 
								levelWidth, 
								levelHeight, 
								levelDepth, 
								0,
								targetFormat, 
								targetType, 
								decompressed.get() ) );
						}
					}
				}
			}
			else
			{
				GL_VALIDATE( glTexImage3D( 
					GL_TEXTURE_3D, 
					i, 
					internalFormat, 
					levelWidth, 
					levelHeight, 
					levelDepth, 
					0,
					GL_RGB, 
					GL_UNSIGNED_BYTE,
					nullptr ) );
			}
		}

		if( trueMipLevelCount == 1 )
		{
			CR_GL( glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
		}
		succeeded = true;
		return S_OK;
	}

	ALResult CreateDepthStencil( GLuint& texture, GLuint& msaaBuffer, GLuint& stencilBuffer, bool readable, const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2SubresourceData* initialData )
	{
		texture = 0;
		msaaBuffer = 0;
		stencilBuffer = 0;

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
				if( stencilBuffer )
				{
					glDeleteRenderbuffers( 1, &stencilBuffer );
					stencilBuffer = 0;
				}
			}
		} );

		if( readable )
		{
#ifdef TRINITY_AL_MOBILE
			if( !CHECK_EXT( OES_depth_texture ) )
			{
				return E_FAIL;
			}
#endif
			GL_FAIL( glGenTextures( 1, &texture ) );
			GL_FAIL( glBindTexture( GL_TEXTURE_2D, texture ) );
#ifdef TRINITY_AL_MOBILE
			if( CHECK_EXT( OES_packed_depth_stencil ) )
			{
				GL_FAIL( glTexImage2D( 
					GL_TEXTURE_2D,
					0,
					GL_DEPTH_STENCIL_OES,
					desc.GetWidth(),
					desc.GetHeight(),
					0,
					GL_DEPTH_STENCIL_OES,
					GL_UNSIGNED_INT_24_8_OES,
					nullptr ) );
			}
			else
			{
				GLint glDepthFormat = 0;
				if( CHECK_EXT( OES_depth24 ) )
				{
					glDepthFormat = GL_DEPTH_COMPONENT24_OES;
				}
				else if( CHECK_EXT( OES_depth32 ) )
				{
					glDepthFormat = GL_DEPTH_COMPONENT32_OES;
				}
				else
				{
					return GL_DEPTH_COMPONENT16;
				}
				GL_FAIL( glTexImage2D( 
					GL_TEXTURE_2D,
					0,
					glDepthFormat,
					desc.GetWidth(),
					desc.GetHeight(),
					0,
					GL_DEPTH_COMPONENT,
					GL_FLOAT,
					nullptr ) );
			}
#else
			if( GLEW_EXT_packed_depth_stencil )
			{
				GL_FAIL( glTexImage2D( 
					GL_TEXTURE_2D,
					0,
					GL_DEPTH24_STENCIL8_EXT,
					desc.GetWidth(),
					desc.GetHeight(),
					0,
					GL_DEPTH_STENCIL_EXT,
					GL_UNSIGNED_INT_24_8_EXT,
					nullptr ) );
			}
			else
			{
				GL_FAIL( glTexImage2D( GL_TEXTURE_2D,
					0,
					GL_DEPTH_COMPONENT24,
					desc.GetWidth(),
					desc.GetHeight(),
					0,
					GL_DEPTH_COMPONENT,
					GL_FLOAT,
					nullptr ) );
			}
#endif
#ifdef TRINITY_AL_MOBILE
			if( !CHECK_EXT( OES_packed_depth_stencil ) )
#else
			if( !GL_EXT_packed_depth_stencil )
#endif
			{
				// we'll also need a stencil buffer
				GL_FAIL( glGenRenderbuffers( 1, &stencilBuffer ) );
				GL_FAIL( glBindRenderbuffer( GL_RENDERBUFFER, stencilBuffer ) );
				GL_FAIL( glRenderbufferStorage( GL_RENDERBUFFER, GL_STENCIL_INDEX8, desc.GetWidth(), desc.GetHeight() ) );
			}
		}
		else
		{
			GLenum glDepthFormat = 0, glStencilFormat = 0;
			bool separateStencilBuffer = true;

			switch( desc.GetFormat() )
			{
			case Tr2RenderContextEnum::PIXEL_FORMAT_D24_UNORM_S8_UINT:
#ifdef TRINITY_AL_MOBILE
				if( CHECK_EXT( OES_packed_depth_stencil ) )
				{
					glDepthFormat = GL_DEPTH24_STENCIL8_OES;
					separateStencilBuffer = false;
				}
				else
				{
					if( CHECK_EXT( OES_depth24 ) )
					{
						glDepthFormat = GL_DEPTH_COMPONENT24_OES;
					}
					else if( CHECK_EXT( OES_depth32 ) )
					{
						glDepthFormat = GL_DEPTH_COMPONENT32_OES;
					}
					else
					{
						return E_INVALIDARG;
					}
				}
#else
				if( GLEW_EXT_packed_depth_stencil )
				{
					glDepthFormat = GL_DEPTH24_STENCIL8_EXT;
					separateStencilBuffer = false;
				}
				else
				{
					glDepthFormat = GL_DEPTH_COMPONENT24;
					glStencilFormat = GL_STENCIL_INDEX8;
				}
#endif
				break;
			default:
				return E_INVALIDARG;
			}

			GL_FAIL( glGenRenderbuffers( 1, &msaaBuffer ) );
			GL_FAIL( glBindRenderbuffer( GL_RENDERBUFFER, msaaBuffer ) );
#ifndef TRINITY_AL_MOBILE
			if( msaa.samples > 1 )
			{
				GL_FAIL( glRenderbufferStorageMultisample( GL_RENDERBUFFER, msaa.samples, glDepthFormat, desc.GetWidth(), desc.GetHeight() ) );
			}
			else
#endif
			{
				GL_FAIL( glRenderbufferStorage( GL_RENDERBUFFER, glDepthFormat, desc.GetWidth(), desc.GetHeight() ) );
			}

			if( separateStencilBuffer )
			{
				GL_FAIL( glGenRenderbuffers( 1, &stencilBuffer ) );
				GL_FAIL( glBindRenderbuffer( GL_RENDERBUFFER, stencilBuffer ) );
#ifndef TRINITY_AL_MOBILE
				if( msaa.samples > 1 )
				{
					GL_FAIL( glRenderbufferStorageMultisample( GL_RENDERBUFFER, msaa.samples, glStencilFormat, desc.GetWidth(), desc.GetHeight() ) );
				}
				else
#endif
				{
					GL_FAIL( glRenderbufferStorage( GL_RENDERBUFFER, glStencilFormat, desc.GetWidth(), desc.GetHeight() ) );
				}
			}
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
		m_stencilBuffer( 0 ),
		m_gpuUsage( Tr2GpuUsage::NONE ),
		m_cpuUsage( Tr2CpuUsage::NONE )
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
		GLuint stencilBuffer = 0;
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) && msaa.samples > 1 )
		{
			GLenum internalFormat, targetFormat, targetType;
			if( !Tr2RenderContextAL::ConvertToGLPixelFormat( desc.GetFormat(), internalFormat, targetFormat, targetType ) )
			{
				return E_INVALIDARG;
			}

			CR_GL( glGenRenderbuffers( 1, &msaaBuffer ) );
			CR_GL( glBindRenderbuffer( GL_RENDERBUFFER, msaaBuffer ) );
			CR_GL( glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER, msaa.samples, internalFormat, desc.GetWidth(), desc.GetHeight() ) );
			CR_GL( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );
		}
		else if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			FORWARD_HR( CreateDepthStencil( texture, msaaBuffer, stencilBuffer, HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ), desc, msaa, initialData ) );
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
		m_stencilBuffer = stencilBuffer;

		m_desc = desc;
		m_msaa = msaa;
		m_gpuUsage = gpuUsage;
		m_cpuUsage = cpuUsage;
		m_memory.Set( Tr2MemoryCounterAL::TEXTURE, desc, msaa );

		float borderColor[4] = { 0.f, 0.f, 0.f, 0.f };
		TrinityALImpl::Tr2SamplerStateAL::CreateStateData( Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_CLAMP,
			Tr2RenderContextEnum::TA_CLAMP,
			Tr2RenderContextEnum::TA_CLAMP,
			0,
			0,
			Tr2RenderContextEnum::CMP_ALWAYS,
			borderColor,
			0,
			1 ), m_currentSampler );
		TrinityALImpl::Tr2SamplerStateAL::Apply( GL_TEXTURE_2D, false, m_currentSampler );

		return S_OK;
	}

	ALResult Tr2TextureAL::OpenShared( uintptr_t handle, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext )
	{
		return E_FAIL;
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
		if( m_stencilBuffer )
		{
			glDeleteRenderbuffers( 1, &m_stencilBuffer );
			m_stencilBuffer = 0;
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
