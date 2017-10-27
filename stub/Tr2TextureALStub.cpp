#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "Tr2TextureALStub.h"
#include "Tr2RenderContextStub.h"

using namespace Tr2RenderContextEnum;

#include "ALLog.h"

#pragma warning( disable: 4189 )	// Scopeguard

bool g_preloadTextureToDeviceOnPrepare = true;


Tr2TextureAL::Tr2TextureAL()
:	m_usage( 0 )
{
}

void Tr2TextureAL::Destroy()
{
	m_data = nullptr;
	m_usage = 0;
	m_arraySize = 0;
}

Tr2TextureAL::~Tr2TextureAL()
{
}

size_t Tr2TextureAL::GetTextureSize() const
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

ALResult Tr2TextureAL::Create(
	Tr2RenderContextEnum::TextureType type,
	uint32_t width,
	uint32_t height,
	uint32_t depth,
	uint32_t mipLevelCount,
	uint32_t arrayCount,
	Tr2RenderContextEnum::PixelFormat format,
	Tr2RenderContextEnum::BufferUsage usage,
	Tr2SubresourceData* initialData,
	Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if( usage == USAGE_IMMUTABLE && !initialData )
	{
		return E_INVALIDARG;
	}

	m_width = width;
	m_height = height;
	m_volumeDepth = 1;
	m_mipCount = mipLevelCount;
	m_format = format;
	m_type = type;
	m_usage = usage;
	m_arraySize = arrayCount;

	size_t size = GetTextureSize();

	m_data.reset( CCP_NEW( "Tr2TextureALStub::m_data" ) uint8_t[size], []( uint8_t* p ) { CCP_DELETE[] p; } );
	if( !m_data )
	{
		return E_OUTOFMEMORY;
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
	return Create( TEX_TYPE_2D, width, height, 1, mipLevelCount, 1, format, usage, initialData, renderContext );
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
	return Create( TEX_TYPE_2D, width, height, 1, mipLevelCount, arrayCount, format, usage, initialData, renderContext );
}

ALResult Tr2TextureAL::CreateCube( uint32_t width,
								   uint32_t height,
								   uint32_t mipLevelCount,
								   Tr2RenderContextEnum::PixelFormat format,
								   Tr2RenderContextEnum::BufferUsage usage,
								   Tr2SubresourceData* initialData,
								   Tr2RenderContextAL& renderContext )
{
	return Create( TEX_TYPE_CUBE, width, height, 1, mipLevelCount, 6, format, usage, initialData, renderContext );
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
	if( !initialData )
	{
		return E_INVALIDARG;
	}
	return Create( TEX_TYPE_3D, width, height, depth, mipLevelCount, 1, format, usage, initialData, renderContext );
}

bool Tr2TextureAL::IsValid() const
{
	return m_data != nullptr;
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

ALResult Tr2TextureAL::CopySubresourceRegion( const Tr2TextureSubresource& destSubresource,
											  Tr2TextureAL& source,
											  const Tr2TextureSubresource& sourceSubresource,
											  Tr2RenderContextAL& renderContext )
{
	return S_OK;
}

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
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( lockType != LOCK_READONLY && lockType != LOCK_WRITEONLY )
	{
		return E_INVALIDARG;
	}
	if( ltrb )
	{
		if ( ltrb[0] >= ltrb[2] || ltrb[1] >= ltrb[3] || ltrb[2] > GetMipWidth( mipLevel ) ||
			 ltrb[3] > GetMipHeight( mipLevel ) )
		{
			return E_FAIL;
		}
	}

	pitch = GetMipPitch( mipLevel );
	data = m_data.get();

	return S_OK;
}

ALResult Tr2TextureAL::Unlock( Tr2RenderContextAL& renderContext )
{
	return S_OK;
}

#endif
