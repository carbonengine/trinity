#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2TextureALGLES2.h"
#include "BcDecompress.h"
#include "ALLog.h"
#include "Tr2RenderContextGLES2.h"


using namespace Tr2RenderContextEnum;


bool g_preloadTextureToDeviceOnPrepare = true;

extern bool g_usingEXDevice;
extern bool g_useManagedDX9Buffers;

namespace
{

struct OnDeleteTexture
{
	void operator()( GLuint* object )
	{
		if ( object && *object )
		{
			glDeleteTextures( 1, object );
		}
	}
};

#define DEFINE_CHECK_EXT(name)													\
	bool Has##name()															\
	{																			\
		static bool initialized = false;										\
		static bool hasExt = false;												\
		if( !initialized )														\
		{																		\
			const char* extensions = (const char*)glGetString( GL_EXTENSIONS );	\
			hasExt = strstr( extensions, #name ) != 0;							\
			initialized = true;													\
		}																		\
		return hasExt;															\
	}
#define CHECK_EXT(name) (Has##name())

DEFINE_CHECK_EXT( OES_packed_depth_stencil )
DEFINE_CHECK_EXT( OES_depth24 )
DEFINE_CHECK_EXT( OES_depth32 )
DEFINE_CHECK_EXT( OES_depth_texture );
}



Tr2TextureAL::Tr2TextureAL()
: m_currentLock( LOCK_INVALID ),
  m_internalFormat( 0 ),
  m_targetFormat( 0 ),
  m_targetType( 0 ),
  m_texture( 0 ),
  m_isAlias( false )
{
	Destroy();
}

void Tr2TextureAL::Destroy()
{
	Tr2BitmapDimensions::Destroy();
	m_usage = 0;

	m_internalFormat = 0;
	m_targetFormat = 0;
	m_targetType = 0;

#ifdef __ANDROID__
    if( m_texture )
    {
        if( --m_texture[1] == 0 )
        {
            delete[] m_texture;
        }
    }
#endif
	m_texture = nullptr;

	memset( &m_currentSampler, 0, sizeof( m_currentSampler ) );
}

Tr2TextureAL::~Tr2TextureAL()
{
	Destroy();
}

ALResult Tr2TextureAL::Create2D( uint32_t width,
								 uint32_t height,
								 uint32_t mipLevelCount,
								 Tr2RenderContextEnum::PixelFormat format,
								 Tr2RenderContextEnum::BufferUsage usage,
								 Tr2SubresourceData* initialData,
								 Tr2RenderContextAL& renderContext )
{
	Destroy();

	if ( width == 0 || height == 0 )
	{
		return E_INVALIDARG;
	}

	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Create2D: Trying to create an immutable texture without providing data" );
		return E_INVALIDARG;
	}

	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	if ( !Tr2RenderContextAL::ConvertToGLPixelFormat( format, m_internalFormat, m_targetFormat, m_targetType ) )
	{
		return E_INVALIDARG;
	}

	GLuint texture;
	GL_FAIL( glGenTextures( 1, &texture ) );
#ifdef __ANDROID__
    m_texture = new GLuint[2];
    m_texture[0] = texture;
    m_texture[1] = 1;
#else
	m_texture = std::shared_ptr<GLuint>( new GLuint( texture ), OnDeleteTexture() );
#endif
	GL_FAIL( glBindTexture( GL_TEXTURE_2D, *m_texture ) );
	GL_FAIL( glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ) );

	const uint32_t trueMipLevelCount = mipLevelCount ? mipLevelCount : 1;

	for ( uint32_t i = 0; i != trueMipLevelCount; ++i )
	{
		uint32_t levelWidth = std::max( width >> i, 1U );
		uint32_t levelHeight = std::max( height >> i, 1U );
		if ( m_targetType )
		{
			GL_FAIL( glTexImage2D( GL_TEXTURE_2D, i, m_internalFormat, levelWidth, levelHeight, 0,
								   m_targetFormat, m_targetType,
								   initialData ? initialData[i].m_sysMem : nullptr ) );
		}
		else if ( initialData )
		{
			GL_FAIL( glCompressedTexImage2D( GL_TEXTURE_2D, i, m_internalFormat, levelWidth, levelHeight, 0,
											 initialData[i].m_sysMemSlicePitch,
											 initialData[i].m_sysMem ) );
		}
		else
		{
			GL_FAIL( glTexImage2D( GL_TEXTURE_2D, i, m_internalFormat, levelWidth, levelHeight, 0,
								   GL_RGB, GL_UNSIGNED_BYTE,
								   nullptr ) );
		}
	}

	if ( trueMipLevelCount > 1 )
	{
		// If we have mipmapped texture, but do not provide all mip levels, GL will
		// think of such texture as invalid. So we define lower mip levels leaving
		// them with garbage. ...And GLES does not have a way of limiting mips used(
		for ( uint32_t i = trueMipLevelCount;
			  i < uint32_t( 0.5 + log( std::max<double>( height, width ) ) / log( 2.0 ) ) + 1; ++i )
		{
			uint32_t levelWidth = std::max( width >> i, 1U );
			uint32_t levelHeight = std::max( height >> i, 1U );
			if ( m_targetType )
			{
				GL_FAIL( glTexImage2D( GL_TEXTURE_2D, i, m_internalFormat, levelWidth, levelHeight, 0,
									   m_targetFormat, m_targetType,
									   nullptr ) );
			}
			else
			{
				GL_FAIL( glCompressedTexImage2D( GL_TEXTURE_2D, i, m_internalFormat, levelWidth, levelHeight, 0,
												 0,
												 nullptr ) );
			}
		}
	}

	if ( !mipLevelCount )
	{
		GL_FAIL( glGenerateMipmap( GL_TEXTURE_2D ) );
	}
	else if ( mipLevelCount == 1 )
	{
		CR_GL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
	}

	m_format = format;
	m_usage = usage;
	m_type = TEX_TYPE_2D;
	m_width = width;
	m_height = height;
	m_volumeDepth = 1;
	m_mipCount = mipLevelCount;
	m_arraySize = 1;
	m_isAlias = false;
	ChangeObjectId();
	float borderColor[4] = { 0.f, 0.f, 0.f, 0.f };
	Tr2SamplerStateAL::CreateStateData( Tr2SamplerDescription( TF_POINT,
													TF_POINT,
													TF_POINT,
													false,
													TA_CLAMP,
													TA_CLAMP,
													TA_CLAMP,
													0,
													0,
													CMP_ALWAYS,
													borderColor,
													0,
													1 ), m_currentSampler );
	Tr2SamplerStateAL::Apply( GL_TEXTURE_2D, false, m_currentSampler );

	return S_OK;
}

ALResult Tr2TextureAL::CreateCube( uint32_t width,
								   uint32_t height,
								   uint32_t mipLevelCount,
								   Tr2RenderContextEnum::PixelFormat format,
								   Tr2RenderContextEnum::BufferUsage usage,
								   Tr2SubresourceData* initialData,
								   Tr2RenderContextAL& renderContext )
{
	Destroy();

	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "CreateCube: Trying to create an immutable texture without providing data" );
		return E_INVALIDARG;
	}

	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	if ( !Tr2RenderContextAL::ConvertToGLPixelFormat( format, m_internalFormat, m_targetFormat, m_targetType ) )
	{
		return E_INVALIDARG;
	}

	GLuint texture;
	GL_FAIL( glGenTextures( 1, &texture ) );
#ifdef __ANDROID__
    m_texture = new GLuint[2];
    m_texture[0] = texture;
    m_texture[1] = 1;
#else
	m_texture = std::shared_ptr<GLuint>( new GLuint( texture ), OnDeleteTexture() );
#endif
	GL_FAIL( glBindTexture( GL_TEXTURE_CUBE_MAP, *m_texture ) );
	GL_FAIL( glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ) );

	const uint32_t trueMipLevelCount = mipLevelCount ? mipLevelCount : 1;

	for ( uint32_t face = 0; face < 6; ++face )
	{
		for ( uint32_t i = 0; i != trueMipLevelCount; ++i )
		{
			uint32_t levelWidth = std::max( width >> i, 1U );
			uint32_t levelHeight = std::max( height >> i, 1U );
			if ( m_targetType )
			{
				GL_FAIL( glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
									   i, m_internalFormat, levelWidth, levelHeight, 0,
									   m_targetFormat, m_targetType,
									   initialData ? initialData[face * trueMipLevelCount + i].m_sysMem : nullptr ) );
			}
			else if( initialData )
			{
				GL_FAIL( glCompressedTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
												 i, 
												 m_internalFormat, 
												 levelWidth, 
												 levelHeight, 
												 0,
												 initialData[face * trueMipLevelCount + i].m_sysMemSlicePitch,
												 initialData[face * trueMipLevelCount + i].m_sysMem ) );
			}
			else
			{
				GL_FAIL( glTexImage2D( 
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 
					i, 
					m_internalFormat, 
					levelWidth, 
					levelHeight, 
					0,
					GL_RGB, 
					GL_UNSIGNED_BYTE,
					nullptr ) );
			}
		}
	}

	if ( trueMipLevelCount > 1 )
	{
		// If we have mipmapped texture, but do not provide all mip levels, GL will
		// think of such texture as invalid. So we define lower mip levels leaving
		// them with garbage. ...And GLES does not have a way of limiting mips used(
		for ( uint32_t face = 0; face < 6; ++face )
		{
			for ( uint32_t i = trueMipLevelCount;
				  i < uint32_t( 0.5 + log( std::max<double>( height, width ) ) / log( 2.0 ) ) + 1; ++i )
			{
				uint32_t levelWidth = std::max( width >> i, 1U );
				uint32_t levelHeight = std::max( height >> i, 1U );
				if ( m_targetType )
				{
					GL_FAIL( glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
										   i, m_internalFormat, levelWidth, levelHeight, 0,
										   m_targetFormat, m_targetType,
										   nullptr ) );
				}
				else
				{
					GL_FAIL( glCompressedTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
													 i, m_internalFormat, levelWidth, levelHeight, 0,
													 0,
													 nullptr ) );
				}
			}
		}
	}

	if ( !mipLevelCount && initialData )
	{
		GL_FAIL( glGenerateMipmap( GL_TEXTURE_CUBE_MAP ) );
	}
	else if ( mipLevelCount == 1 )
	{
		CR_GL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
	}

	m_format = format;
	m_usage = usage;
	m_type = TEX_TYPE_CUBE;
	m_width = width;
	m_height = height;
	m_volumeDepth = 1;
	m_mipCount = mipLevelCount;
	m_arraySize = 1;
	m_isAlias = false;
	ChangeObjectId();
	float borderColor[4] = { 0.f, 0.f, 0.f, 0.f };
	Tr2SamplerStateAL::CreateStateData( Tr2SamplerDescription( TF_POINT,
													TF_POINT,
													TF_POINT,
													false,
													TA_CLAMP,
													TA_CLAMP,
													TA_CLAMP,
													0,
													0,
													CMP_ALWAYS,
													borderColor,
													0,
													1 ), m_currentSampler );
	Tr2SamplerStateAL::Apply( GL_TEXTURE_2D, false, m_currentSampler );

	return S_OK;
}

ALResult Tr2TextureAL::CreateVolume( uint32_t width,
									 uint32_t height,
									 uint32_t depth,
									 uint32_t mipLevelCount,
									 Tr2RenderContextEnum::PixelFormat format,
									 Tr2RenderContextEnum::BufferUsage usage,
									 Tr2SubresourceData* initialData,
									 Tr2RenderContextAL& renderContext )
{
	Destroy();

	if ( width == 0 || height == 0 || depth == 0 )
	{
		return E_INVALIDARG;
	}

	if( !initialData )
	{
		CCP_AL_LOGERR( "CreateVolume: Trying to create volume texture without providing data" );
		return E_INVALIDARG;
	}

	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	if ( !Tr2RenderContextAL::ConvertToGLPixelFormat( format, m_internalFormat, m_targetFormat, m_targetType ) )
	{
		return E_INVALIDARG;
	}

	GLuint texture = 0;
	GL_FAIL( glGenTextures( 1, &texture ) );
	if( !texture )
	{
		return E_FAIL;
	}
	m_texture = std::shared_ptr<GLuint>( new GLuint( texture ), OnDeleteTexture() );

	GL_FAIL( glBindTexture( GL_TEXTURE_3D, *m_texture ) );
	GL_FAIL( glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ) );

	const uint32_t trueMipLevelCount = mipLevelCount ? mipLevelCount : 1;

	std::unique_ptr<uint8_t[]> decompressed;

	for ( uint32_t i = 0; i != trueMipLevelCount; ++i )
	{
		uint32_t levelWidth = std::max( width >> i, 1U );
		uint32_t levelHeight = std::max( height >> i, 1U );
		uint32_t levelDepth = std::max( depth >> i, 1U );
		if( decompressed )
		{
			if( !BcDecompress( levelWidth, levelHeight, levelDepth, format, initialData[i], decompressed ) )
			{
				return E_FAIL;
			}
			GL_VALIDATE( glTexImage3D( GL_TEXTURE_3D, i, m_internalFormat, levelWidth, levelHeight, levelDepth, 0,
									m_targetFormat, m_targetType, decompressed.get() ) );
		}
		else if ( m_targetType )
		{
			GL_VALIDATE( glTexImage3D( GL_TEXTURE_3D, i, m_internalFormat, levelWidth, levelHeight, levelDepth, 0,
								   m_targetFormat, m_targetType,
								   initialData ? initialData[i].m_sysMem : nullptr ) );
		}
		else if ( initialData )
		{
			if( GLEW_NV_texture_compression_vtc )
			{
				std::unique_ptr<uint8_t[]> level( new uint8_t[initialData[i].m_sysMemSlicePitch * levelDepth] );
				int blockSize = m_targetFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ? 16 : 8;
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
				GL_VALIDATE( glCompressedTexImage3D( GL_TEXTURE_3D, i, m_internalFormat, levelWidth, levelHeight, levelDepth, 0,
												 initialData[i].m_sysMemSlicePitch * levelDepth,
												 level.get() ) );
			}
			else
			{
				glCompressedTexImage3D( GL_TEXTURE_3D, i, m_internalFormat, levelWidth, levelHeight, levelDepth, 0, initialData[i].m_sysMemSlicePitch * levelDepth, initialData[i].m_sysMem );
				if( glGetError() )
				{
					if( BcDecompress( levelWidth, levelHeight, levelDepth, format, initialData[i], decompressed ) )
					{
						Tr2RenderContextAL::ConvertToGLPixelFormat( PIXEL_FORMAT_B8G8R8A8_UNORM, m_internalFormat, m_targetFormat, m_targetType );
						GL_VALIDATE( glTexImage3D( GL_TEXTURE_3D, i, m_internalFormat, levelWidth, levelHeight, levelDepth, 0,
											   m_targetFormat, m_targetType, decompressed.get() ) );
					}
				}
			}
		}
		else
		{
			GL_VALIDATE( glTexImage3D( GL_TEXTURE_3D, i, m_internalFormat, levelWidth, levelHeight, levelDepth, 0,
								   GL_RGB, GL_UNSIGNED_BYTE,
								   nullptr ) );
		}
	}

	if ( !mipLevelCount )
	{
		GL_FAIL( glGenerateMipmap( GL_TEXTURE_3D ) );
	}
	else if ( mipLevelCount == 1 )
	{
		CR_GL( glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
	}

	m_format = format;
	m_usage = usage;
	m_type = TEX_TYPE_3D;
	m_width = width;
	m_height = height;
	m_volumeDepth = depth;
	m_mipCount = mipLevelCount;
	m_arraySize = 1;
	m_isAlias = false;
	ChangeObjectId();
	float borderColor[4] = { 0.f, 0.f, 0.f, 0.f };
	Tr2SamplerStateAL::CreateStateData( Tr2SamplerDescription( TF_POINT,
													TF_POINT,
													TF_POINT,
													false,
													TA_CLAMP,
													TA_CLAMP,
													TA_CLAMP,
													0,
													0,
													CMP_ALWAYS,
													borderColor,
													0,
													1 ), m_currentSampler );
	Tr2SamplerStateAL::Apply( GL_TEXTURE_3D, false, m_currentSampler );

	return S_OK;
}

ALResult Tr2TextureAL::CreateDepthTexture( uint32_t width,
										   uint32_t height,
										   Tr2RenderContextAL& renderContext )
{
	Destroy();
#ifdef TRINITY_AL_MOBILE
    if( !CHECK_EXT( OES_depth_texture ) )
    {
        return E_FAIL;
    }
#endif
	GLuint texture;
	GL_FAIL( glGenTextures( 1, &texture ) );
#ifdef __ANDROID__
    m_texture = new GLuint[2];
    m_texture[0] = texture;
    m_texture[1] = 1;
#else
	m_texture = std::shared_ptr<GLuint>( new GLuint( texture ), OnDeleteTexture() );
#endif
	GL_FAIL( glBindTexture( GL_TEXTURE_2D, *m_texture ) );
	//CR_GL( glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ) );
#ifdef TRINITY_AL_MOBILE
    if( CHECK_EXT( OES_packed_depth_stencil ) )
	{
		GL_FAIL( glTexImage2D( GL_TEXTURE_2D,
                              0,
                              GL_DEPTH_STENCIL_OES,
                              width,
                              height,
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
		GL_FAIL( glTexImage2D( GL_TEXTURE_2D,
                              0,
                              glDepthFormat,
                              width,
                              height,
                              0,
                              GL_DEPTH_COMPONENT,
                              GL_FLOAT,
                              nullptr ) );
	}
#else
	if ( GLEW_EXT_packed_depth_stencil )
	{
		GL_FAIL( glTexImage2D( GL_TEXTURE_2D,
							   0,
							   GL_DEPTH24_STENCIL8_EXT,
							   width,
							   height,
							   0,
							   GL_DEPTH_STENCIL_EXT,
							   GL_UNSIGNED_INT_24_8_EXT,
							   nullptr ) );
	}
	else
	{
		GL_FAIL( glTexImage2D( GL_TEXTURE_2D,
							   0,
							   GL_DEPTH_COMPONENT32,
							   width,
							   height,
							   0,
							   GL_DEPTH_COMPONENT,
							   GL_FLOAT,
							   nullptr ) );
	}
#endif
	m_width = width;
	m_height = height;
	m_volumeDepth = 0;
	m_mipCount = 1;
	m_arraySize = 1;
	m_format = PIXEL_FORMAT_D32_FLOAT;
	m_type = TEX_TYPE_2D;
	float borderColor[4] = { 0.f, 0.f, 0.f, 0.f };
	Tr2SamplerStateAL::CreateStateData( Tr2SamplerDescription( TF_POINT,
													TF_POINT,
													TF_POINT,
													false,
													TA_CLAMP,
													TA_CLAMP,
													TA_CLAMP,
													0,
													0,
													CMP_ALWAYS,
													borderColor,
													0,
													1 ), m_currentSampler );
	Tr2SamplerStateAL::Apply( GL_TEXTURE_2D, false, m_currentSampler );
	return S_OK;
}

bool Tr2TextureAL::IsValid() const
{
	return m_texture != 0;
}

ALResult Tr2TextureAL::UpdateSubresource( uint32_t left,
										  uint32_t top,
										  uint32_t right,
										  uint32_t bottom,
										  const void* source,
										  uint32_t sourcePitch,
										  Tr2RenderContextAL& /*renderContext*/ )
{
	if ( !IsValid() || !source || !sourcePitch || left >= right || top >= bottom || m_type != TEX_TYPE_2D )
	{
		return E_FAIL;
	}

	GL_FAIL( glBindTexture( GL_TEXTURE_2D, *m_texture ) );

	if ( m_targetType )
	{
		GL_FAIL( glTexSubImage2D( GL_TEXTURE_2D, 0, left, top, right - left, bottom - top, m_targetFormat,
								  m_targetType, source ) );
	}
	else
	{
		GL_FAIL( glCompressedTexSubImage2D( GL_TEXTURE_2D, 0, left, top, right - left, bottom - top, m_targetFormat,
											sourcePitch * ( bottom - top ), source ) );
	}

	return S_OK;
}
// --------------------------------------------------------------------------------------
// Description:
//   Copies part of the source texture into this texture. Textures must have the same 
//   format, subresources must have the same dimensions, 3D textures are not supported.  
// Arguments:
//   destSubresource - Desription of destination texture area
//   source - Source texture
//   sourceSubresource - Description of source texture area
//   renderContext - Current render context
// Return Value:
//   HRESULT of operation
// --------------------------------------------------------------------------------------
ALResult Tr2TextureAL::CopySubresourceRegion( const Tr2TextureSubresource& destSubresource,
											  Tr2TextureAL& source,
											  const Tr2TextureSubresource& sourceSubresource,
											  Tr2RenderContextAL& renderContext )
{
	return renderContext.CopySubresourceRegion( *this, destSubresource, source, sourceSubresource );
}

ALResult Tr2TextureAL::CopySubresourceRegion( const Tr2TextureSubresource& destSubresource,
											  Tr2RenderTargetAL& source,
											  const Tr2TextureSubresource& sourceSubresource,
											  Tr2RenderContextAL& renderContext )
{
	return renderContext.CopySubresourceRegion( *this, destSubresource, source.m_backingStore, sourceSubresource );
}

ALResult Tr2TextureAL::Lock( uint32_t mipLevel,
							 uint32_t* ltrb,
							 void*& data,
							 uint32_t& pitch,
							 LockType lockType,
							 Tr2RenderContextAL& renderContext )
{
	if ( m_currentLock != LOCK_INVALID )
	{
		CCP_AL_LOGERR( "Attempting to lock already locked texture" );
		return E_FAIL;
	}

	switch( lockType )
	{
	case LOCK_READONLY:
		// Not implemented: insanely inefficient in OpenGL
		return E_FAIL;
	case LOCK_WRITEONLY:
		return LockWriting( CUBEMAP_FACE_FIRST, mipLevel, ltrb, data, pitch, renderContext );
	default:
		return E_INVALIDARG;
	}
}

ALResult Tr2TextureAL::Lock( uint32_t mipLevel,
							 void*& data,
							 uint32_t& pitch,
							 LockType lockType,
							 Tr2RenderContextAL& renderContext )
{
	return Lock( mipLevel, nullptr, data, pitch, lockType, renderContext );
}

ALResult Tr2TextureAL::Lock( uint32_t face,
							 uint32_t mipLevel,
							 uint32_t* ltrb,
							 void*& data,
							 uint32_t& pitch,
							 LockType lockType,
							 Tr2RenderContextAL& renderContext )
{
	if ( m_currentLock != LOCK_INVALID )
	{
		CCP_AL_LOGERR( "Attempting to lock already locked texture" );
		return E_FAIL;
	}

	switch( lockType )
	{
	case LOCK_READONLY:
		// Not implemented: insanely inefficient in OpenGL
		return E_FAIL;
	case LOCK_WRITEONLY:
		return LockWriting( face, mipLevel, ltrb, data, pitch, renderContext );
	default:
		return E_INVALIDARG;
	}
}

ALResult Tr2TextureAL::Unlock( Tr2RenderContextAL& renderContext )
{
	if ( m_currentLock != LOCK_WRITEONLY )
	{
		CCP_AL_LOGERR( "Trying to Unlock buffer that's not locked" );
		return E_FAIL;
	}
	return UnlockWriting( renderContext );
}

ALResult Tr2TextureAL::LockWriting( uint32_t face,
									uint32_t mipLevel,
									uint32_t* ltrb,
									void*& data,
									uint32_t& pitch,
									Tr2RenderContextAL& /*renderContext*/ )
{
	CCP_ASSERT( m_usage != USAGE_IMMUTABLE );

	if ( !IsValid() )//|| ltrb == nullptr && ( m_usage & USAGE_CPU_WRITE ) == 0 )
	{
		data = nullptr;
		return E_FAIL;
	}

	if ( ltrb == nullptr )
	{
		m_lockedRect[0] = 0;
		m_lockedRect[1] = 0;
		m_lockedRect[2] = GetMipWidth( mipLevel );
		m_lockedRect[3] = GetMipHeight( mipLevel );
	}
	else
	{
		if ( ltrb[0] >= ltrb[2] || ltrb[1] >= ltrb[3] || ltrb[2] > GetMipWidth( mipLevel ) ||
			 ltrb[3] > GetMipHeight( mipLevel ) )
		{
			return E_FAIL;
		}
		std::copy( ltrb, ltrb + 4, m_lockedRect );
	}

	m_lockedFace = face;
	m_lockedLevel = mipLevel;
	if ( IsCompressed() )
	{
		pitch = ( m_lockedRect[2] - m_lockedRect[0] ) * GetBlockByteSize( m_format ) / 16;
		m_lockedData.resize( pitch * ( m_lockedRect[3] - m_lockedRect[1] ) );
	}
	else
	{
		pitch = ( m_lockedRect[2] - m_lockedRect[0] ) * GetBytesPerPixel( m_format );
		m_lockedData.resize( pitch * ( m_lockedRect[3] - m_lockedRect[1] ) );
	}
	data = &m_lockedData[0];

	m_currentLock = LOCK_WRITEONLY;

	return S_OK;
}

ALResult Tr2TextureAL::UnlockWriting( Tr2RenderContextAL& /*renderContext*/ )
{
#pragma warning( disable: 4189 )	// scopeguard

	ON_BLOCK_EXIT( [&] {
					m_lockedData.clear();
				   } );
	m_currentLock = LOCK_INVALID;

	GLenum texType = m_type == TEX_TYPE_CUBE ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + m_lockedFace : GL_TEXTURE_2D;

	GL_FAIL( glBindTexture( texType, *m_texture ) );

	if ( m_targetType )
	{
		GL_FAIL( glTexSubImage2D( texType,
								  m_lockedLevel,
								  m_lockedRect[0],
								  m_lockedRect[1],
								  m_lockedRect[2] - m_lockedRect[0],
								  m_lockedRect[3] - m_lockedRect[1],
								  m_targetFormat,
								  m_targetType,
								  &m_lockedData[0] ) );
	}
	else
	{
		GL_FAIL( glCompressedTexSubImage2D( texType,
											m_lockedLevel,
											m_lockedRect[0],
											m_lockedRect[1],
											m_lockedRect[2] - m_lockedRect[0],
											m_lockedRect[3] - m_lockedRect[1],
											m_internalFormat,
											( m_lockedRect[2] -
											  m_lockedRect[0] ) * ( m_lockedRect[3] - m_lockedRect[1] ) *
											GetBlockByteSize( m_format ) / 16,
											&m_lockedData[0] ) );
	}

	return S_OK;
}

#endif
