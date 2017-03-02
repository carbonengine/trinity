#include "StdAfx.h"
#include "Tr2TextureALStub.h"

using namespace Tr2RenderContextEnum;

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "ALLog.h"

#pragma warning( disable: 4189 )	// Scopeguard

bool g_preloadTextureToDeviceOnPrepare = true;

extern bool g_usingEXDevice;
extern bool g_useManagedDX9Buffers;




Tr2TextureAL::Tr2TextureAL()
:	m_isValid( false ),
	m_currentLock( LOCK_INVALID ),
	m_usage( 0 )
{
}

void Tr2TextureAL::Destroy()
{
	m_isValid = false;
	m_data = nullptr;
	m_usage = 0;
	m_currentLock = LOCK_INVALID;
	m_arraySize = 0;
}

Tr2TextureAL::~Tr2TextureAL()
{
}

Tr2TextureAL& Tr2TextureAL::operator=( Tr2TextureAL&& other )
{
	if( this != &other )
	{
		m_format		= other.m_format;
		m_usage			= other.m_usage;
		m_type			= other.m_type;
		m_width			= other.m_width;
		m_height		= other.m_height;
		m_volumeDepth	= other.m_volumeDepth;
		m_mipCount		= other.m_mipCount;
		m_isValid = other.m_isValid;
		m_data = std::move(other.m_data);
		m_arraySize = other.m_arraySize;
		other.m_isValid = false;
		ChangeObjectId();
	}

	return *this;
}

Tr2TextureAL& Tr2TextureAL::operator=( Tr2TextureAL& other )
{
	if( this != &other )
	{
		m_format		= other.m_format;
		m_usage			= other.m_usage;
		m_type			= other.m_type;
		m_width			= other.m_width;
		m_height		= other.m_height;
		m_volumeDepth	= other.m_volumeDepth;
		m_mipCount		= other.m_mipCount;
		m_isValid = other.m_isValid;
		m_data = other.m_data;
		other.m_isValid = false;
		m_arraySize = other.m_arraySize;
		ChangeObjectId();
	}

	return *this;
}

size_t Tr2TextureAL::GetTextureSize()
{
	uint32_t mipLevelCount = GetTrueMipCount();
	uint32_t width = m_width;
	uint32_t height = m_height;
	uint32_t depth = m_volumeDepth;
	size_t size = 0;
	if( IsCompressedFormat( m_format ) )
	{
		while( mipLevelCount-- )
		{
			size += width * height * depth * GetBlockByteSize( m_format ) / 16;
			width  = std::max( width  / 2u, 4u );
			height = std::max( height / 2u, 4u );			
			depth = std::max( depth / 2u, 1u );			
		}
	}
	else
	{
		while( mipLevelCount-- )
		{
			size += width * height * depth * GetBytesPerPixel( m_format );
			width  = std::max( width  / 2u, 1u );
			height = std::max( height / 2u, 1u );			
			depth = std::max( depth / 2u, 1u );			
		}
	}
	size *= m_arraySize;
	return size;
}

ALResult Tr2TextureAL::LoadInitialData( Tr2SubresourceData* initialData )
{
	if( initialData )
	{
		unsigned int levelsToCopy = GetTrueMipCount();
		levelsToCopy *= m_arraySize;
		
		uint8_t* destination = m_data.get();
		unsigned int offset = 0;
		for( unsigned int level = 0; level < levelsToCopy; ++level )
		{
			auto size = initialData[level].m_sysMemSlicePitch;
			memcpy( destination, (char*)initialData[level].m_sysMem, size );
			destination += size;
		}
	}

	return S_OK;
}

ALResult Tr2TextureAL::Create2D( uint32_t width,
								 uint32_t height,
								 uint32_t mipLevelCount,
								 Tr2RenderContextEnum::PixelFormat format,
								 Tr2RenderContextEnum::BufferUsage usage,
								 Tr2SubresourceData* initialData,
								 Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if (usage == USAGE_IMMUTABLE && !initialData )
	{
		return E_INVALIDARG;
	}

	m_width = width;
	m_height = height;
	m_volumeDepth = 1;
	m_mipCount = mipLevelCount;
	m_format = format;
	m_type = TEX_TYPE_2D;
	m_isValid = true;
	m_usage = usage;
	m_arraySize = 1;

	size_t size = GetTextureSize();

	m_data.reset( CCP_NEW( "Tr2TextureALStub::m_data" ) uint8_t[size], []( uint8_t* p ) { CCP_DELETE[] p; } );
	
	return LoadInitialData(initialData);
}

ALResult Tr2TextureAL::Create2DArray(	
	uint32_t width, 
	uint32_t height, 
	uint32_t mipLevelCount,
	uint32_t arrayCount,
	Tr2RenderContextEnum::PixelFormat format,
	Tr2RenderContextEnum::BufferUsage usage,
	Tr2SubresourceData* initialData,
	Tr2PrimaryRenderContextAL &renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if (usage == USAGE_IMMUTABLE && !initialData )
	{
		return E_INVALIDARG;
	}

	m_width = width;
	m_height = height;
	m_volumeDepth = 1;
	m_mipCount = mipLevelCount;
	m_format = format;
	m_type = TEX_TYPE_2D;
	m_isValid = true;
	m_usage = usage;
	m_arraySize = arrayCount;

	size_t size = GetTextureSize();

	m_data.reset( CCP_NEW( "Tr2TextureALStub::m_data" ) uint8_t[size], []( uint8_t* p ) { CCP_DELETE[] p; } );
	
	return LoadInitialData(initialData);
}

ALResult Tr2TextureAL::CreateCube( uint32_t width,
								   uint32_t height,
								   uint32_t mipLevelCount,
								   Tr2RenderContextEnum::PixelFormat format,
								   Tr2RenderContextEnum::BufferUsage usage,
								   Tr2SubresourceData* initialData,
								   Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if(usage == USAGE_IMMUTABLE && !initialData )
	{
		return E_INVALIDARG;
	}
	m_width = width;
	m_height = height;
	m_volumeDepth = 1;
	m_mipCount = mipLevelCount;
	m_format = format;
	m_type = TEX_TYPE_CUBE;
	m_isValid = true;
	m_usage = usage;
	m_arraySize = 6;

	size_t size = GetTextureSize();

	m_data.reset( CCP_NEW( "Tr2TextureALStub::m_data" ) uint8_t[size], []( uint8_t* p ) { CCP_DELETE[] p; } );

	return LoadInitialData(initialData);
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
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	
	if( !initialData )
	{
		return E_INVALIDARG;
	}
	m_width = width;
	m_height = height;
	m_volumeDepth = depth;
	m_mipCount = mipLevelCount;
	m_format = format;
	m_type = TEX_TYPE_3D;
	m_isValid = true;
	m_usage = usage;
	m_arraySize = 1;

	size_t size = GetTextureSize();
	m_data.reset( CCP_NEW( "Tr2TextureALStub::m_data" ) uint8_t[size], []( uint8_t* p ) { CCP_DELETE[] p; } );
	
	return LoadInitialData(initialData);
}

bool Tr2TextureAL::IsValid() const
{
	return m_isValid;
}

ALResult Tr2TextureAL::UpdateSubresource( uint32_t left,
										  uint32_t top,
										  uint32_t right,
										  uint32_t bottom,
										  const void* source,
										  uint32_t sourcePitch,
										  Tr2RenderContextAL& renderContext )
{
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
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Copies part of the source renderTarget into this texture. Textures must have the same 
//   format, 3D textures are not supported.  The subresources are allowed to have different
//	 dimensions, in which case cropping occurs (no stretching)
// Arguments:
//   destSubresource - Desription of destination texture area
//   source - Source texture
//   sourceSubresource - Description of source renderTarget area
//   renderContext - Current render context
// Return Value:
//   HRESULT of operation
// --------------------------------------------------------------------------------------
ALResult Tr2TextureAL::CopySubresourceRegion( const Tr2TextureSubresource& destSubresource,
											  Tr2RenderTargetAL& rt,
											  const Tr2TextureSubresource& sourceSubresource,
											  Tr2RenderContextAL& renderContext )
{
	return S_OK;
}

ALResult Tr2TextureAL::Lock( uint32_t mipLevel,
							 void*& data,
							 uint32_t& pitch,
							 LockType lockType,
							 Tr2RenderContextAL& renderContext )
{
	return Lock( mipLevel, nullptr, data, pitch, lockType, renderContext );
}

ALResult Tr2TextureAL::Lock( uint32_t mipLevel,
							 uint32_t* ltrb,
							 void*& data,
							 uint32_t& pitch,
							 LockType lockType,
							 Tr2RenderContextAL& renderContext )
{
	return Lock( 0, mipLevel, ltrb, data, pitch, lockType, renderContext );
}

ALResult Tr2TextureAL::Lock( uint32_t face,
							 uint32_t mipLevel,
							 uint32_t* ltrb,
							 void*& data,
							 uint32_t& pitch,
							 LockType lockType,
							 Tr2RenderContextAL& renderContext )
{
	if( !m_isValid )
	{
		return E_INVALIDCALL;
	}
	if( m_type == TEX_TYPE_2D && lockType == LOCK_NO_OVERWRITE )
	{
		return E_INVALIDARG;
	}
	if( ltrb == nullptr )
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

		m_lockedRect[0] = ltrb[0];
		m_lockedRect[1] = ltrb[1];
		m_lockedRect[2] = ltrb[2];
		m_lockedRect[3] = ltrb[3];
	}

	m_lockedFace = face;
	m_lockedLevel = mipLevel;
	size_t bpp = GetBytesPerPixel( m_format );
	
	size_t face_offset = 0;
	if( face )
	{
		if( m_type == TEX_TYPE_CUBE )
		{
			face_offset = (GetTextureSize() / 6) * face;
		}
		else
		{
			return E_INVALIDARG;
		}
	}
	
	pitch = GetMipPitch(mipLevel);
	size_t height_offset = m_width * bpp * m_lockedRect[1];
	size_t width_offset = m_lockedRect[0] * bpp;
	size_t offset = face_offset + height_offset + width_offset;
	
	data = m_data.get() + offset;

	m_currentLock = LOCK_WRITEONLY;
	return S_OK;
}

ALResult Tr2TextureAL::Unlock( Tr2RenderContextAL& renderContext )
{
	if( m_currentLock != LOCK_READONLY && m_currentLock != LOCK_WRITEONLY )
	{
		CCP_AL_LOGERR( "Trying to Unlock buffer that's not locked" );
		return E_FAIL;
	}
	m_currentLock = LOCK_INVALID;
	return S_OK;
}

#endif
