#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2TextureALOrbis.h"
#include "Tr2MemoryAllocator.h"
#include "ALLog.h"
#include <gpu_address.h>

using namespace Tr2RenderContextEnum;

bool g_preloadTextureToDeviceOnPrepare = true;

namespace
{

void InitializeTexture( sce::Gnm::Texture& texture, TextureType textureType, PixelFormat format, Tr2SubresourceData* initialData )
{
	uint32_t mipWidth = texture.getWidth();
	uint32_t mipHeight = texture.getHeight();
	uint32_t mipDepth = texture.getDepth();
	uint32_t bpp = GetBytesPerPixel( format );
	uint32_t mipCount = texture.getLastMipLevel() + 1;
	void* memory = texture.getBaseAddress();
	
	for( uint32_t mipLevel = 0; mipLevel < mipCount; ++mipLevel )
	{
		uint32_t sysMemPitch = initialData[mipLevel].m_sysMemPitch;

		for( uint32_t arraySlice = 0; arraySlice <= texture.getLastArraySliceIndex(); ++arraySlice )
		{
			uint32_t sysMemSlicePitch = initialData[arraySlice * mipCount + mipLevel].m_sysMemSlicePitch;

			uint64_t mipOffset;
			sce::GpuAddress::computeTextureSurfaceOffsetAndSize( 
				&mipOffset,
				( uint64_t* )( 0 ),
				&texture,
				mipLevel,
				arraySlice );
			uint8_t* mipMemory = reinterpret_cast<uint8_t*>( memory ) + mipOffset;

			sce::GpuAddress::TilingParameters tilingParams;
			tilingParams.m_arraySlice = arraySlice;
			tilingParams.m_baseTiledPitch = mipLevel == 0 ? 0 : texture.getPitch();
			tilingParams.m_elemFormat = texture.getDataFormat();
			tilingParams.m_linearDepth = mipDepth;
			tilingParams.m_linearHeight = mipHeight;
			tilingParams.m_linearWidth = mipWidth;
			tilingParams.m_mipLevel = mipLevel;
			tilingParams.m_numElementsPerPixel = 1;
			tilingParams.m_surfaceFlags.m_value = 0;
			tilingParams.m_surfaceFlags.m_cube = textureType == TEX_TYPE_CUBE ? 1 : 0;
			tilingParams.m_surfaceFlags.m_volume = textureType == TEX_TYPE_3D ? 1 : 0;
			tilingParams.m_surfaceFlags.m_pow2Pad = texture.isPaddedToPow2() ? 1 : 0;
			tilingParams.m_tileMode = texture.getTileMode();

			sce::GpuAddress::SurfaceInfo surfaceInfo;
			sce::GpuAddress::computeSurfaceInfo( &surfaceInfo, &tilingParams );

			uint32_t pitch = surfaceInfo.m_pitch;

			const uint8_t* src = reinterpret_cast<const uint8_t*>( initialData[arraySlice * mipCount + mipLevel].m_sysMem );

			if( tilingParams.m_tileMode != sce::Gnm::kTileModeDisplay_LinearAligned )
			{
				sce::GpuAddress::tileSurface( mipMemory, src, &tilingParams );
			}
			else
			{
				uint32_t bytesPerRow = pitch * bpp;
				uint32_t numRows = mipHeight;
				uint32_t mipLevelSize = bytesPerRow * numRows;

				for( uint32_t slice = 0; slice < texture.getDepth(); ++slice )
				{
					if( mipLevelSize == sysMemPitch * numRows )
					{
						memcpy( mipMemory, src, mipLevelSize );
						mipMemory += mipLevelSize;
					}
					else
					{
						for( uint32_t row = 0; row < numRows; ++row )
						{
							memcpy( mipMemory, src, bytesPerRow );
							src += sysMemPitch;
							mipMemory += bytesPerRow;
						}
					}
					src += sysMemSlicePitch;
				}
			}
		}
		mipWidth = std::max( mipWidth / 2, 1u );
		mipHeight = std::max( mipHeight / 2, 1u );
		mipDepth = std::max( mipDepth / 2, 1u );
	}
}

uint32_t GetTrueMipCount( uint32_t mipCount, uint32_t width, uint32_t height )
{
	uint32_t trueMipLevelCount = mipCount;
	if( trueMipLevelCount == 0 )
	{
		uint32_t size = std::max( width, height );
		while( size )
		{
			size /= 2;
			trueMipLevelCount++;
		}
	}
	return trueMipLevelCount;
}

uint32_t GetTrueMipCount( uint32_t mipCount, uint32_t width, uint32_t height, uint32_t depth )
{
	uint32_t trueMipLevelCount = mipCount;
	if( trueMipLevelCount == 0 )
	{
		uint32_t size = std::max( width, std::max( height, depth ) );
		while( size )
		{
			size /= 2;
			trueMipLevelCount++;
		}
	}
	return trueMipLevelCount;
}

}

Tr2TextureAL::Tr2TextureAL()
	:m_usage( 0 ),
	m_isAlias( false )
{
	m_texture.setBaseAddress( nullptr );
	m_textureSrgb.setBaseAddress( nullptr );
}

Tr2TextureAL::~Tr2TextureAL()
{
}

void Tr2TextureAL::Destroy()
{
	m_memory = nullptr;
	m_texture.setBaseAddress( nullptr );
	m_textureSrgb.setBaseAddress( nullptr );
	Tr2BitmapDimensions::Destroy();
	m_usage = 0;
}

Tr2TextureAL& Tr2TextureAL::operator=( Tr2TextureAL&& other )
{
	if( *this == other )
	{
		return *this;
	}
	static_cast<Tr2BitmapDimensions&>( *this ) = other;
	m_usage = other.m_usage;
	m_isAlias = other.m_isAlias;
	m_memory = std::move( other.m_memory );
	m_texture = other.m_texture;
	m_textureSrgb = other.m_textureSrgb;

	other.m_memory = nullptr;
	other.m_texture.setBaseAddress( nullptr );
	other.m_textureSrgb.setBaseAddress( nullptr );
	other.Tr2BitmapDimensions::Destroy();
	other.m_usage = 0;
	ChangeObjectId();

	return *this;
}

Tr2TextureAL& Tr2TextureAL::operator=( const Tr2TextureAL& other )
{
	if( *this == other )
	{
		return *this;
	}
	static_cast<Tr2BitmapDimensions&>( *this ) = other;
	m_usage = other.m_usage;
	m_isAlias = other.m_isAlias;
	m_memory = other.m_memory;
	m_texture = other.m_texture;
	m_textureSrgb = other.m_textureSrgb;
	ChangeObjectId();
	return *this;
}

bool Tr2TextureAL::IsValid() const
{
	return bool( m_memory );
}

ALResult Tr2TextureAL::InternalCreateTexture2D( 
	uint32_t width,
	uint32_t height,
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format,
	sce::Gnm::TileMode tileMode,
	sce::Gnm::NumSamples numSamples,
	Tr2BufferImplAL::Access cpuAccess,
	Tr2BufferImplAL::Access gpuAccess,
	Tr2RenderContextAL& renderContext )
{
	uint32_t trueMipLevelCount = ::GetTrueMipCount( mipLevelCount, width, height );

	sce::Gnm::DataFormat dataFormat;
	if( !Tr2RenderContextAL::ConvertPixelFormatToDataFormat( format, dataFormat ) )
	{
		CCP_AL_LOGERR( "Tr2TextureAL::Create2D: unsupported texture format %i", int( format ) );
		return E_INVALIDARG;
	}
	auto sizeAlign = m_texture.initAs2d( width, height, trueMipLevelCount, dataFormat, tileMode, numSamples );
	if( sizeAlign.m_size == 0 )
	{
		return E_FAIL;
	}

	void* initialDataPtr = nullptr;
	m_memory = std::shared_ptr<Tr2BufferImplAL>( CCP_NEW( "Texture::m_memory" ) Tr2BufferImplAL );
	CR_RETURN_HR( m_memory->Create( 
		sizeAlign.m_size,
		sizeAlign.m_align,
		cpuAccess,
		gpuAccess,
		Tr2BufferImplAL::DYNAMIC,
		Tr2MemoryAllocator::GARLIC,
		&initialDataPtr,
		renderContext ) );

	CCP_ASSERT( m_texture.getWidth() == width );
	m_texture.setBaseAddress( initialDataPtr );
	m_texture.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );

	PixelFormat formatSrgb = MakeSrgb( format );
	Tr2RenderContextAL::ConvertPixelFormatToDataFormat( formatSrgb, dataFormat );
	m_textureSrgb.initAs2d( width, height, trueMipLevelCount, dataFormat, tileMode, numSamples );
	m_textureSrgb.setBaseAddress( initialDataPtr );
	m_textureSrgb.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );

	m_format = format;
	m_type = TEX_TYPE_2D;
	m_width = width;
	m_height = height;
	m_volumeDepth = 1;
	m_mipCount = mipLevelCount;
	m_isAlias = false;
	return S_OK;
}

ALResult Tr2TextureAL::Create2D( 
	uint32_t width,
	uint32_t height,
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format,
	Tr2RenderContextEnum::BufferUsage usage,
	Tr2SubresourceData* initialData,
	Tr2RenderContextAL& renderContext )
{
	AL_FUZZ( OT_TEXTURE );
	CCP_STATS_ZONE( __FUNCTION__ );

	Destroy();

	if ( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Invalid combination of USAGE flags passed to Tr2TextureAL Create function" );
		return E_INVALIDARG;
	}

	if ( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Trying to create an immutable texture without providing data" );
		return E_INVALIDARG;
	}

	if ( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	sce::Gnm::TileMode tileMode = Tr2RenderContextEnum::IsCompressedFormat( format ) ? sce::Gnm::kTileModeThin_1dThin : sce::Gnm::kTileModeDisplay_LinearAligned;

	CR_RETURN_HR( InternalCreateTexture2D( 
		width, 
		height, 
		mipLevelCount, 
		format, 
		tileMode, 
		sce::Gnm::kNumSamples1, 
		Tr2BufferImplAL::READ_WRITE,
		Tr2BufferImplAL::READ_ONLY,
		renderContext ) );

	if( initialData )
	{
		InitializeTexture( m_texture, TEX_TYPE_2D, format, initialData );
	}
	m_usage = usage;
	ChangeObjectId();

	return S_OK;
}

ALResult Tr2TextureAL::CreateCube( 
	uint32_t width,
	uint32_t height,
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format,
	Tr2RenderContextEnum::BufferUsage usage,
	Tr2SubresourceData* initialData,
	Tr2RenderContextAL& renderContext )
{
	AL_FUZZ( OT_TEXTURE );
	CCP_STATS_ZONE( __FUNCTION__ );

	Destroy();

	if ( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Invalid combination of USAGE flags passed to Tr2TextureAL Create function" );
		return E_INVALIDARG;
	}

	if ( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Trying to create an immutable texture without providing data" );
		return E_INVALIDARG;
	}

	if ( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	sce::Gnm::TileMode tileMode = Tr2RenderContextEnum::IsCompressedFormat( format ) ? sce::Gnm::kTileModeThin_1dThin : sce::Gnm::kTileModeDisplay_LinearAligned;

	uint32_t trueMipLevelCount = ::GetTrueMipCount( mipLevelCount, width, height );

	sce::Gnm::DataFormat dataFormat;
	if( !Tr2RenderContextAL::ConvertPixelFormatToDataFormat( format, dataFormat ) )
	{
		CCP_AL_LOGERR( "Tr2TextureAL::CreateCube: unsupported texture format %i", int( format ) );
		return E_INVALIDARG;
	}
	auto sizeAlign = m_texture.initAsCubemap( width, height, trueMipLevelCount, dataFormat, tileMode );
	if( sizeAlign.m_size == 0 )
	{
		return E_FAIL;
	}
	
	void* initialDataPtr = nullptr;
	m_memory = std::shared_ptr<Tr2BufferImplAL>( CCP_NEW( "Texture::m_memory" ) Tr2BufferImplAL );
	CR_RETURN_HR( m_memory->Create( 
		sizeAlign.m_size,
		sizeAlign.m_align,
		Tr2BufferImplAL::READ_WRITE,
		Tr2BufferImplAL::READ_WRITE,
		Tr2BufferImplAL::DYNAMIC,
		Tr2MemoryAllocator::GARLIC,
		&initialDataPtr,
		renderContext ) );

	CCP_ASSERT( m_texture.getWidth() == width );
	m_texture.setBaseAddress( initialDataPtr );
	m_texture.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );

	PixelFormat formatSrgb = MakeSrgb( format );
	Tr2RenderContextAL::ConvertPixelFormatToDataFormat( formatSrgb, dataFormat );
	m_textureSrgb.initAsCubemap( width, height, trueMipLevelCount, dataFormat, tileMode );
	m_textureSrgb.setBaseAddress( initialDataPtr );
	m_textureSrgb.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );

	if( initialData )
	{
		InitializeTexture( m_texture, TEX_TYPE_CUBE, format, initialData );
	}

	m_format = format;
	m_usage = usage;
	m_type = TEX_TYPE_CUBE;
	m_width = width;
	m_height = height;
	m_volumeDepth = 1;
	m_mipCount = mipLevelCount;
	m_isAlias = false;
	ChangeObjectId();

	return S_OK;
}

ALResult Tr2TextureAL::CreateVolume( 
	uint32_t width,
	uint32_t height,
	uint32_t depth,
	uint32_t mipLevelCount,
	Tr2RenderContextEnum::PixelFormat format,
	Tr2RenderContextEnum::BufferUsage usage,
	Tr2SubresourceData* initialData,
	Tr2RenderContextAL& renderContext )
{
	AL_FUZZ( OT_TEXTURE );
	CCP_STATS_ZONE( __FUNCTION__ );

	Destroy();

	if ( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Invalid combination of USAGE flags passed to Tr2TextureAL Create function" );
		return E_INVALIDARG;
	}

	if ( !initialData )
	{
		CCP_AL_LOGERR( "Trying to create volume texture without providing data" );
		return E_INVALIDARG;
	}

	if ( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	sce::Gnm::TileMode tileMode = Tr2RenderContextEnum::IsCompressedFormat( format ) ? sce::Gnm::kTileModeThin_1dThin : sce::Gnm::kTileModeDisplay_LinearAligned;

	uint32_t trueMipLevelCount = ::GetTrueMipCount( mipLevelCount, width, height, depth );

	sce::Gnm::DataFormat dataFormat;
	if( !Tr2RenderContextAL::ConvertPixelFormatToDataFormat( format, dataFormat ) )
	{
		CCP_AL_LOGERR( "Tr2TextureAL::CreateVolume: unsupported texture format %i", int( format ) );
		return E_INVALIDARG;
	}
	auto sizeAlign = m_texture.initAs3d( width, height, depth, trueMipLevelCount, dataFormat, tileMode );
	if( sizeAlign.m_size == 0 )
	{
		return E_FAIL;
	}
	
	void* initialDataPtr = nullptr;
	m_memory = std::shared_ptr<Tr2BufferImplAL>( CCP_NEW( "Texture::m_memory" ) Tr2BufferImplAL );
	CR_RETURN_HR( m_memory->Create( 
		sizeAlign.m_size,
		sizeAlign.m_align,
		Tr2BufferImplAL::READ_WRITE,
		Tr2BufferImplAL::READ_WRITE,
		Tr2BufferImplAL::DYNAMIC,
		Tr2MemoryAllocator::GARLIC,
		&initialDataPtr,
		renderContext ) );

	CCP_ASSERT( m_texture.getWidth() == width );
	m_texture.setBaseAddress( initialDataPtr );
	m_texture.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );

	PixelFormat formatSrgb = MakeSrgb( format );
	Tr2RenderContextAL::ConvertPixelFormatToDataFormat( formatSrgb, dataFormat );
	m_textureSrgb.initAs3d( width, height, depth, trueMipLevelCount, dataFormat, tileMode );
	m_textureSrgb.setBaseAddress( initialDataPtr );
	m_textureSrgb.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );

	if( initialData )
	{
		InitializeTexture( m_texture, TEX_TYPE_3D, format, initialData );
	}

	m_format = format;
	m_usage = usage;
	m_type = TEX_TYPE_3D;
	m_width = width;
	m_height = height;
	m_volumeDepth = depth;
	m_mipCount = mipLevelCount;
	m_isAlias = false;
	ChangeObjectId();

	return S_OK;
}

ALResult Tr2TextureAL::InternalAttachToDepthBuffer(
	const sce::Gnm::DepthRenderTarget& depthBuffer,
	const std::shared_ptr<Tr2BufferImplAL>& memory,
	Tr2RenderContextAL& renderContext )
{
	m_texture.initFromDepthRenderTarget( &depthBuffer );
	m_memory = memory;
	m_format = depthBuffer.getZFormat() == sce::Gnm::kZFormat32Float ? PIXEL_FORMAT_R32_FLOAT : PIXEL_FORMAT_R16_UNORM,
	m_isAlias = true;
	m_type = TEX_TYPE_2D;
	m_width = m_texture.getWidth();
	m_height = m_texture.getHeight();
	m_volumeDepth = 1;
	m_mipCount = 1;
	m_usage = 0;
	return S_OK;
}

ALResult Tr2TextureAL::InternalGetTextureForGpu( 
	Tr2RenderContextEnum::ColorSpace colorSpace, 
	sce::Gnm::Texture& texture, 
	Tr2RenderContextAL& renderContext ) const
{
	if( !IsValid() )
	{
		return E_FAIL;
	}
	void* memory = m_memory->GetMemoryForGpuReading( renderContext );
	if( !memory )
	{
		return E_OUTOFMEMORY;
	}
	if( colorSpace == COLOR_SPACE_SRGB )
	{
		m_textureSrgb.setBaseAddress( memory );
		texture = m_textureSrgb;
	}
	else
	{
		m_texture.setBaseAddress( memory );
		texture = m_texture;
	}
	return S_OK;
}

ALResult Tr2TextureAL::UpdateSubresource( 
	uint32_t left,
	uint32_t top,
	uint32_t right,
	uint32_t bottom,
	const void* source,
	uint32_t sourcePitch,
	Tr2RenderContextAL& renderContext )
{
	AL_FUZZ( OT_TEXTURE );
	CCP_STATS_ZONE( __FUNCTION__ );

	if ( !IsValid() || !source || !sourcePitch || left >= right || top >= bottom || m_type != TEX_TYPE_2D )
	{
		return E_INVALIDARG;
	}

	if ( ( m_usage & USAGE_CPU_WRITE ) || ( m_usage & USAGE_IMMUTABLE ) )
	{
		CCP_AL_LOGWARN( "UpdateSubResource only works with no USAGE_CPU_WRITE and no USAGE_IMMUTABLE flags" );
		return E_INVALIDARG;
	}

	if ( ( bottom > m_height ) || ( right > m_width ) )
	{
		CCP_AL_LOGERR( "UpdateSubResource out of bounds (%d, %d, %d, %d), texture dimensions (%d, %d)",
			left, top, right, bottom, m_width, m_height );
		return E_INVALIDARG;
	}

	// TODO: use copyData here?
	CCP_AL_LOGWARN( "UpdateSubResource is hugely inefficient" );
	renderContext.InternalSyncToGpu();

	void* memory = nullptr;
	if( m_isAlias )
	{
		memory = m_texture.getBaseAddress();
	}
	else
	{
		memory = m_memory->GetMemoryForCpuWriting( renderContext );
	}

	uint32_t bpp = GetBytesPerPixel( m_format );
	uint32_t destPitch = m_texture.getPitch() * bpp;
	uint8_t* dest = reinterpret_cast<uint8_t*>( memory ) + top * destPitch + left * bpp;
	const uint8_t* src = reinterpret_cast<const uint8_t*>( source );
	uint32_t rowSize = bpp * ( right - left );
	for( uint32_t i = 0; i < bottom - top; ++i )
	{
		memcpy( dest, src, rowSize );
		src += sourcePitch;
		dest += destPitch;
	}
	return S_OK;
}

ALResult Tr2TextureAL::CopySubresourceRegion( const Tr2TextureSubresource& destSubresource,
							   Tr2TextureAL& source,
							   const Tr2TextureSubresource& sourceSubresource,
							   Tr2RenderContextAL& renderContext )
{
	return E_INVALIDCALL;
}

ALResult Tr2TextureAL::CopySubresourceRegion( const Tr2TextureSubresource& destSubresource,
							   Tr2RenderTargetAL& source,
							   const Tr2TextureSubresource& sourceSubresource,
							   Tr2RenderContextAL& renderContext )
{
	return E_INVALIDCALL;
}

ALResult Tr2TextureAL::Lock( 
	uint32_t mipLevel,
	void*& data,
	uint32_t& pitch,
	Tr2RenderContextEnum::LockType lockType,
	Tr2RenderContextAL& renderContext )
{
	return Lock( mipLevel, nullptr, data, pitch, lockType, renderContext );
}

ALResult Tr2TextureAL::Lock( uint32_t mipLevel,
				uint32_t* ltrb,
				void*& data,
				uint32_t& pitch,
				Tr2RenderContextEnum::LockType lockType,
				Tr2RenderContextAL& renderContext )
{
	return Lock( CUBEMAP_FACE_FIRST, mipLevel, ltrb, data, pitch, lockType, renderContext );
}

ALResult Tr2TextureAL::Lock( Tr2RenderContextEnum::CubemapFace face,
				uint32_t mipLevel,
				uint32_t* ltrb,
				void*& data,
				uint32_t& pitch,
				Tr2RenderContextEnum::LockType lockType,
				Tr2RenderContextAL& renderContext )
{
	AL_FUZZ_LOCK( OT_TEXTURE );
	CCP_STATS_ZONE( __FUNCTION__ );
	if ( m_type != TEX_TYPE_CUBE && face > 0 )
	{
		return E_FAIL;
	}

	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	
	if( m_isAlias )
	{
		return E_INVALIDCALL;
	}
	switch( lockType )
	{
	case LOCK_WRITEONLY:
		m_texture.setBaseAddress( m_memory->GetMemoryForCpuWriting( renderContext ) );
		break;
	case LOCK_READONLY:
		m_texture.setBaseAddress( m_memory->GetMemoryForCpuReading( renderContext ) );
		break;
	default:
		return E_INVALIDARG;
	}


	uint64_t mipOffset;
	sce::GpuAddress::computeTextureSurfaceOffsetAndSize( 
		&mipOffset,
		( uint64_t* )( 0 ),
		&m_texture,
		mipLevel,
		face );

	sce::GpuAddress::TilingParameters tilingParams;
	tilingParams.m_arraySlice = face;
	tilingParams.m_baseTiledPitch = mipLevel == 0 ? 0 : m_texture.getPitch();
	tilingParams.m_elemFormat = m_texture.getDataFormat();
	tilingParams.m_linearDepth = 1;
	tilingParams.m_linearHeight = GetMipHeight( mipLevel );
	tilingParams.m_linearWidth = GetMipWidth( mipLevel );
	tilingParams.m_mipLevel = mipLevel;
	tilingParams.m_numElementsPerPixel = 1;
	tilingParams.m_surfaceFlags.m_value = 0;
	tilingParams.m_surfaceFlags.m_cube = m_type == TEX_TYPE_CUBE ? 1 : 0;
	tilingParams.m_surfaceFlags.m_value = m_type == TEX_TYPE_3D ? 1 : 0;
	tilingParams.m_surfaceFlags.m_pow2Pad = m_texture.isPaddedToPow2() ? 1 : 0;
	tilingParams.m_tileMode = m_texture.getTileMode();

	sce::GpuAddress::SurfaceInfo surfaceInfo;
	sce::GpuAddress::computeSurfaceInfo( &surfaceInfo, &tilingParams );

	uint8_t* mipMemory = reinterpret_cast<uint8_t*>( m_texture.getBaseAddress() ) + mipOffset;
	uint32_t bpp = GetBytesPerPixel( m_format );

	pitch = m_texture.getPitch() * bpp;

	if( ltrb )
	{
		mipMemory += pitch * ltrb[1] + ltrb[0] * bpp;
	}
	data = mipMemory;

	return S_OK;
}

ALResult Tr2TextureAL::Unlock( Tr2RenderContextAL& renderContext )
{
	return S_OK;
}

bool Tr2TextureAL::operator==( const Tr2TextureAL& other ) const
{
	return m_memory == other.m_memory;
}

#endif