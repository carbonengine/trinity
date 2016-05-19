#include "StdAfx.h"
#include "Tr2TextureALDx9.h"
#include "BcDecompress.h"

using namespace Tr2RenderContextEnum;

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "ALLog.h"

#pragma warning( disable: 4189 )	// Scopeguard

bool g_preloadTextureToDeviceOnPrepare = true;

extern bool g_usingEXDevice;
extern bool g_useManagedDX9Buffers;

namespace
{

void ConvertUsage( BufferUsage usage,
				   uint32_t& usage9,
				   D3DPOOL& pool9 )
{
	usage9 = 0;
	pool9 = D3DPOOL_DEFAULT;

	if ( usage & USAGE_LOCK_FREQUENTLY )
	{
		usage9 = D3DUSAGE_DYNAMIC;
	}
	else if ( g_useManagedDX9Buffers && !g_usingEXDevice )//&& ( ( usage & USAGE_CPU_READ ) || ( usage & USAGE_CPU_WRITE ) ) )
	{
		pool9 = D3DPOOL_MANAGED;
	}
}

}


Tr2TextureAL::Tr2TextureAL()
: m_currentLock( LOCK_INVALID ),
  m_lockedMipLevel( 0 ),
  m_isAlias( false )
{
	Destroy();
}

void Tr2TextureAL::Destroy()
{
	Tr2BitmapDimensions::Destroy();
	m_usage = 0;
	m_format9 = D3DFMT_UNKNOWN;
	m_pool9 = D3DPOOL_DEFAULT;

	m_texture = nullptr;
	m_lockedMipLevelSurf = nullptr;
}

Tr2TextureAL::~Tr2TextureAL()
{
}

Tr2TextureAL& Tr2TextureAL::operator=( Tr2TextureAL&& other )
{
	if ( this != &other )
	{
		m_texture.Attach( other.m_texture.Detach() );

		m_format		= other.m_format;
		m_usage			= other.m_usage;
		m_type			= other.m_type;
		m_width			= other.m_width;
		m_height		= other.m_height;
		m_volumeDepth	= other.m_volumeDepth;
		m_mipCount		= other.m_mipCount;
		m_format9		= other.m_format9;
		m_pool9			= other.m_pool9;
		m_isAlias		= other.m_isAlias;
		m_arraySize = other.m_arraySize;
		ChangeObjectId();
	}

	return *this;
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

	uint32_t usage9;
	ConvertUsage( usage, usage9, m_pool9 );

	CComPtr<IDirect3DTexture9> tex;

	if ( mipLevelCount == 0 )
	{
		usage9 |= D3DUSAGE_AUTOGENMIPMAP;
	}

	bool needsStagingTexture = false;
	if ( m_pool9 == D3DPOOL_DEFAULT && ( usage9 & D3DUSAGE_DYNAMIC ) == 0 )
	{
		needsStagingTexture = true;
	}

	m_format9 = renderContext.ConvertToD3D9Format( format );
	if ( m_format9 == D3DFMT_UNKNOWN )
	{
		CCP_AL_LOGWARN( "Can't load a texture with format %d on DX9", format );
		return E_FAIL;
	}

	HRESULT hr = renderContext.m_d3dDevice9->CreateTexture( width, height, mipLevelCount, usage9, m_format9,
															m_pool9, &tex, 0 );
	if ( FAILED( hr ) )
	{
		if ( hr == D3DERR_INVALIDCALL )
		{
			switch ( m_format9 )
			{
			case D3DFMT_DXT1:
			case D3DFMT_DXT2:
			case D3DFMT_DXT3:
			case D3DFMT_DXT4:
			case D3DFMT_DXT5:
				if ( ( width < 4 ) || ( height < 4 ) )
				{
					CCP_AL_LOGWARN( "Tr2TextureAL::Create2D failed - can't create compressed texture smaller than 4x4 : %d x %d", width, height)
					;
				}
				else if ( ( width % 4 ) || ( height % 4 ) )
				{
					CCP_AL_LOGWARN( "Tr2TextureAL::Create2D failed - compressed texture is not a multiple of 4, but %d x %d", width, height)
					;
				}
				break;
			}
		}
		return hr;
	}

	const uint32_t trueMipLevelCount = mipLevelCount ? mipLevelCount : 1;

	// Create a staging texture in system memory, load data into it.
	// Then we use UpdateTexture - this way works with default pool textures without
	// having to set the dynamic usage flag.
	CComPtr<IDirect3DTexture9> stagingTexture;

	if ( needsStagingTexture )
	{
		hr = renderContext.m_d3dDevice9->CreateTexture( width,
														height,
														trueMipLevelCount,
														0,
														m_format9,
														D3DPOOL_SYSTEMMEM,
														&stagingTexture,
														nullptr );

		if ( FAILED( hr ) )
		{
			CCP_AL_LOGERR( "Tr2TextureAL::Create2D failed - can't create staging texture" );
			return hr;
		}
	}
	else
	{
		stagingTexture = tex;
	}

	m_format = format;
	m_usage = usage;
	m_type = TEX_TYPE_2D;
	m_width = width;
	m_height = height;
	m_volumeDepth = 1;
	m_mipCount = mipLevelCount;
	m_isAlias = false;
	m_arraySize = 1;


	if ( initialData )
	{
		for ( uint32_t i = 0; i < trueMipLevelCount; ++i )
		{
			BYTE* const p = ( BYTE* )initialData[i].m_sysMem;
			if ( !p )
			{
				return E_FAIL;
			}

			D3DLOCKED_RECT l;
			hr = stagingTexture->LockRect( i, &l, 0, 0 );
			if ( FAILED( hr ) || !l.pBits )
			{
				return hr;
			}
			if( m_format9 == MAKEFOURCC( 'A', 'T', 'I', '1' ) )
			{
				// dx9 incorrectly reports pitch for BC4 format as if it's uncompressed
				l.Pitch *= 2;
			}

			const uint32_t numRows = GetMipNumRows( i );
			const uint32_t mipLevelSize = std::min( GetMipSize( i ), initialData[i].m_sysMemSlicePitch );

			if ( l.Pitch * numRows == mipLevelSize )
			{
				memcpy( l.pBits, p, mipLevelSize );
			}
			else
			{
				for ( uint32_t j = 0; j != numRows; ++j )
				{
					memcpy( ( char* )l.pBits + j * l.Pitch,
							p + j * initialData[i].m_sysMemPitch,
							std::min<uint32_t>( l.Pitch, initialData[i].m_sysMemPitch ) );
				}
			}

			stagingTexture->UnlockRect( i );
		}

		if ( needsStagingTexture )
		{
			hr = tex->AddDirtyRect( NULL );
			hr = renderContext.m_d3dDevice9->UpdateTexture( stagingTexture, tex );

			if ( FAILED( hr ) )
			{
				CCP_AL_LOGWARN( "Tr2TextureAL::Create2D - UpdateTexture failed" );
			}
		}
	}

	if ( g_preloadTextureToDeviceOnPrepare && m_pool9 == D3DPOOL_MANAGED )
	{
		tex->PreLoad();
	}

	m_texture.Attach( tex.Detach() );
	ChangeObjectId();
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

	// Smart pointer helps dealing with error handling. Once we're successful, out
	// is assigned by detaching from the smart pointer.
	CComPtr<IDirect3DCubeTexture9> tex;

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

	uint32_t usage9;
	ConvertUsage( usage, usage9, m_pool9 );


	m_format9 = renderContext.ConvertToD3D9Format( format );
	if ( m_format9 == D3DFMT_UNKNOWN )
	{
		CCP_AL_LOGWARN( "Can't load a texture with format %d on DX9", format );
		return E_FAIL;
	}


	bool needsStagingTexture = false;
	if ( initialData && m_pool9 == D3DPOOL_DEFAULT && ( usage9 & D3DUSAGE_DYNAMIC ) == 0 )
	{
		needsStagingTexture = true;
	}

	HRESULT hr = renderContext.m_d3dDevice9->CreateCubeTexture( width, mipLevelCount, usage9, m_format9, m_pool9, &tex,
																0 );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	const uint32_t trueMipLevelCount = mipLevelCount ? mipLevelCount : 1;


	CComPtr<IDirect3DTexture9> stagingTexture;

	if ( needsStagingTexture )
	{
		hr = renderContext.m_d3dDevice9->CreateTexture( width,
														height,
														trueMipLevelCount,
														0,
														m_format9,
														D3DPOOL_SYSTEMMEM,
														&stagingTexture,
														nullptr );

		if ( FAILED( hr ) )
		{
			CCP_AL_LOGERR( "Tr2TextureAL::Create2D failed - can't create staging texture" );
			return hr;
		}
	}


	if ( initialData )
	{
		for ( uint32_t i = 0; i != trueMipLevelCount; ++i )
		{
			for ( uint32_t face = 0; face != 6; ++face )
			{
				const Tr2SubresourceData& srd = initialData[ face * trueMipLevelCount + i ];

				D3DLOCKED_RECT l =
				{
					0
				};
				if ( stagingTexture )
				{
					CR_RETURN_HR( stagingTexture->LockRect( i, &l, 0, D3DLOCK_DISCARD ) );
				}
				else
				{
					CR_RETURN_HR( tex->LockRect( (D3DCUBEMAP_FACES)face, i, &l, 0, 0 ) );
				}

				if ( !l.pBits )
				{
					return E_FAIL;
				}
				memcpy( l.pBits, srd.m_sysMem, srd.m_sysMemSlicePitch );

				if ( stagingTexture )
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

	m_texture.Attach( tex.Detach() );

	m_format = format;
	m_usage = usage;
	m_type = TEX_TYPE_CUBE;
	m_width = width;
	m_height = height;
	m_volumeDepth = 1;
	m_mipCount = mipLevelCount;
	m_isAlias = false;
	m_arraySize = 6;
	ChangeObjectId();

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

	m_format9 = renderContext.ConvertToD3D9Format( format );
	if ( m_format9 == D3DFMT_UNKNOWN )
	{
		CCP_AL_LOGWARN( "Can't load a texture with format %d on DX9", format );
		return E_INVALIDARG;
	}

	if ( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	const uint32_t trueMipLevelCount = mipLevelCount ? mipLevelCount : 1;
	bool needsDecompression = false;

	uint32_t usage9 = 0;
	ConvertUsage( usage, usage9, m_pool9 );

	CComPtr<IDirect3DVolumeTexture9> tex;
	auto hr = renderContext.m_d3dDevice9->CreateVolumeTexture( width, height, depth, trueMipLevelCount, usage9, m_format9, m_pool9, &tex, 0 );
	if( FAILED( hr ) )
	{
		if( IsCompressedFormat( format ) )
		{
			m_format9 = renderContext.ConvertToD3D9Format( PIXEL_FORMAT_B8G8R8A8_UNORM );
			CR_RETURN_HR( renderContext.m_d3dDevice9->CreateVolumeTexture( width, height, depth, trueMipLevelCount, usage9, m_format9, m_pool9, &tex, 0 ) );
			needsDecompression = true;
		}
		else
		{
			return hr;
		}
	}
	

	bool needsStagingTexture = false;
	if ( initialData && m_pool9 == D3DPOOL_DEFAULT && ( usage9 & D3DUSAGE_DYNAMIC ) == 0 )
	{
		needsStagingTexture = true;
	}

	CComPtr<IDirect3DVolumeTexture9> stagingTexture;

	if ( needsStagingTexture )
	{
		CR_RETURN_HR( renderContext.m_d3dDevice9->CreateVolumeTexture( 
			width,
			height,
			depth,
			trueMipLevelCount,
			0,
			m_format9,
			D3DPOOL_SYSTEMMEM,
			&stagingTexture,
			nullptr ) );
	}
	else
	{
		stagingTexture = tex;
	}

	std::unique_ptr<uint8_t[]> decompressed;

	for ( uint32_t i = 0; i != trueMipLevelCount; ++i )
	{
		D3DLOCKED_BOX l =
		{
			0
		};
		CR_RETURN_HR( stagingTexture->LockBox( i, &l, 0, 0 ) );
		ON_BLOCK_EXIT( [&]{
						stagingTexture->UnlockBox( i );
					   } );

		if ( !l.pBits )
		{
			return E_FAIL;
		}

		uint32_t levelDepth = std::max( depth >> i, 1U );
		if( needsDecompression )
		{
			uint32_t levelWidth = std::max( width >> i, 1U );
			uint32_t levelHeight = std::max( height >> i, 1U );
			if( !BcDecompress( levelWidth, levelHeight, levelDepth, format, initialData[i], decompressed ) )
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
	if ( needsStagingTexture )
	{
		tex->AddDirtyBox( nullptr );
		renderContext.m_d3dDevice9->UpdateTexture( stagingTexture, tex );
	}

	tex->PreLoad();

	m_format = format;
	m_usage = usage;
	m_type = TEX_TYPE_3D;
	m_width = width;
	m_height = height;
	m_volumeDepth = depth;
	m_mipCount = mipLevelCount;
	m_isAlias = false;
	m_arraySize = 1;

	m_texture.Attach( tex.Detach() );
	ChangeObjectId();
	return S_OK;
}

bool Tr2TextureAL::IsValid() const
{
	return m_texture != nullptr;
}

ALResult Tr2TextureAL::UpdateSubresource( uint32_t left,
										  uint32_t top,
										  uint32_t right,
										  uint32_t bottom,
										  const void* source,
										  uint32_t sourcePitch,
										  Tr2RenderContextAL& renderContext )
{
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

	RECT rect;
	rect.left = left;
	rect.top = top;
	rect.right = right;
	rect.bottom = bottom;

	const uint32_t width = right - left;
	const uint32_t height = bottom - top;

	CComQIPtr<IDirect3DTexture9> tex2D( m_texture );
	CCP_ASSERT( tex2D );

	D3DLOCKED_RECT l =
	{
		0
	};
	if ( FAILED( tex2D->LockRect( 0, &l, &rect, 0 ) ) )
	{
		// Try to use UpdateSurface
		if ( renderContext.m_d3dDevice9 == nullptr )
		{
			return E_FAIL;
		}

		CComPtr<IDirect3DSurface9> srcSurface, destSurface;
		CR_RETURN_HR( tex2D->GetSurfaceLevel( 0, &destSurface ) );
		CR_RETURN_HR( renderContext.m_d3dDevice9->CreateOffscreenPlainSurface( width, height, m_format9, D3DPOOL_SYSTEMMEM, &srcSurface, nullptr ) )
		;
		{
			CR_RETURN_HR( srcSurface->LockRect( &l, nullptr, 0 ) );
			ON_BLOCK_EXIT( [&]{
							srcSurface->UnlockRect();
						   } );

			const uint8_t* src = static_cast<const uint8_t*>( source );
			uint8_t* dst = static_cast<uint8_t*>( l.pBits );

			if ( IsCompressedFormat( GetFormat() ) )
			{
				uint32_t blockByteSize = GetBlockByteSize( GetFormat() );
				uint32_t blockPixelSize = 4;
				uint32_t blocksX = width / blockPixelSize;

				for ( uint32_t line = 0; line < height; line += blockPixelSize )
				{
					memcpy( dst, src, blocksX * blockByteSize );

					src += sourcePitch;
					dst += l.Pitch;
				}
			}
			else
			{
				uint32_t byteCount = GetBytesPerPixel( GetFormat() );

				for ( uint32_t line = 0; line != height; ++line )
				{
					memcpy( dst, src, width * byteCount );

					src += sourcePitch;
					dst += l.Pitch;
				}
			}
		}
		POINT destPoint =
		{
			left,
			top
		};
		CR_RETURN_HR( renderContext.m_d3dDevice9->UpdateSurface( srcSurface, nullptr, destSurface, &destPoint ) );

		return S_OK;
	}

	ON_BLOCK_EXIT( [&]{
					tex2D->UnlockRect( 0 );
				   } );
	if ( !l.pBits )
	{
		return E_FAIL;
	}

	const uint8_t* src = static_cast<const uint8_t*>( source );
	uint8_t* dst = static_cast<uint8_t*>( l.pBits );

	if ( IsCompressedFormat( GetFormat() ) )
	{
		uint32_t blockByteSize = GetBlockByteSize( GetFormat() );
		uint32_t blockPixelSize = 4;
		uint32_t blocksX = width / blockPixelSize;

		for ( uint32_t line = 0; line < height; line += blockPixelSize )
		{
			memcpy( dst, src, blocksX * blockByteSize );

			src += sourcePitch;
			dst += l.Pitch;
		}
	}
	else
	{
		uint32_t byteCount = GetBytesPerPixel( GetFormat() );

		for ( uint32_t line = 0; line != height; ++line )
		{
			memcpy( dst, src, width * byteCount );

			src += sourcePitch;
			dst += l.Pitch;
		}
	}

	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Helper function to get D3D surface of the texture.  
// Arguments:
//   texture - Texture to get surface from
//   type - Texture type
//   cubeFace - Cube face if texture is a cube map
//   mipLevel - Mip level
//   surface - (out) Surface or nullptr on failure
// --------------------------------------------------------------------------------------
static void GetSurface( IDirect3DBaseTexture9* texture,
						TextureType type,
						uint32_t cubeFace,
						uint32_t mipLevel,
						IDirect3DSurface9** surface )
{
	*surface = nullptr;
	switch ( type )
	{
	case TEX_TYPE_1D:
	case TEX_TYPE_2D:
		{
			CComQIPtr<IDirect3DTexture9> tex2D( texture );
			if ( !tex2D || FAILED( tex2D->GetSurfaceLevel( mipLevel, surface ) ) )
			{
				surface = nullptr;
			}
		}
		return;
	case TEX_TYPE_CUBE:
		{
			CComQIPtr<IDirect3DCubeTexture9> texCube( texture );
			if ( !texCube || FAILED( texCube->GetCubeMapSurface( D3DCUBEMAP_FACES( cubeFace ), mipLevel, surface ) ) )
			{
				surface = nullptr;
			}
		}
		return;
	}
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
	CCP_ASSERT( IsValid() );
	CCP_ASSERT( source.IsValid() );
	CCP_ASSERT( GetFormat() == source.GetFormat() );
	CCP_ASSERT( renderContext.m_d3dDevice9 );

	if ( !renderContext.m_d3dDevice9 || !m_texture || !source.m_texture || GetFormat() != source.GetFormat() )
	{
		return E_FAIL;
	}

	if ( m_type == TEX_TYPE_3D || source.m_type == TEX_TYPE_3D )
	{
		return E_FAIL;
	}

	Tr2TextureSubresource src = sourceSubresource;
	Tr2TextureSubresource dst = destSubresource;

	if ( !Crop( src, source, dst, *this ) )
	{
		return E_FAIL;
	}

	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	AL_UPDATE_RESOURCE_FRAME_USAGE( source );
	uint32_t lockFlag = 0;
	//if( m_usage & USAGE_DYNAMIC )
	//{
	//	lockFlag = D3DLOCK_DISCARD;
	//}

	const bool nullSource = sourceSubresource == Tr2TextureSubresource();
	const bool nullDest = destSubresource == Tr2TextureSubresource();

	const bool isCompressed = IsCompressedFormat( GetFormat() );
	const uint32_t blockSize = GetBlockByteSize( GetFormat() );
	const uint32_t byteCount = GetBytesPerPixel( GetFormat() );

	const uint32_t mipCount = std::min( src.GetMipCount(), dst.GetMipCount() );

	for ( uint32_t mip = 0; mip != mipCount; ++mip )
	{
		for ( uint32_t face = 0; face != src.GetFaceCount(); ++face )
		{
			CComPtr<IDirect3DSurface9> srcSurface, dstSurface;
			GetSurface( source.m_texture, source.m_type, src.m_startFace + face,
						src.m_startMipLevel + mip, &srcSurface );
			GetSurface( m_texture, m_type, dst.m_startFace + face, dst.m_startMipLevel + mip, &dstSurface );

			if ( !srcSurface || !dstSurface )
			{
				return E_FAIL;
			}

			// for debugging
			D3DSURFACE_DESC srcDesc;
			srcSurface->GetDesc( &srcDesc );

			D3DSURFACE_DESC dstDesc;
			dstSurface->GetDesc( &dstDesc );

			RECT srcRect =
			{
				src.m_left,
				src.m_top,
				src.m_right,
				src.m_bottom
			};
			RECT dstRect =
			{
				dst.m_left,
				dst.m_top,
				dst.m_right,
				dst.m_bottom
			};

			if ( SUCCEEDED( renderContext.m_d3dDevice9->StretchRect( srcSurface, nullSource ? nullptr : &srcRect,
																	 dstSurface, nullDest   ? nullptr : &dstRect,
																	 D3DTEXF_POINT ) ) )
			{
				continue;
			}

			D3DLOCKED_RECT srcLock =
			{
				0
			};
			CR_RETURN_HR( srcSurface->LockRect( &srcLock, &srcRect, D3DLOCK_READONLY ) );
			ON_BLOCK_EXIT( [&]{
							srcSurface->UnlockRect();
						   } );

			D3DLOCKED_RECT dstLock =
			{
				0
			};
			CR_RETURN_HR( dstSurface->LockRect( &dstLock, &dstRect, lockFlag ) );
			ON_BLOCK_EXIT( [&]{
							dstSurface->UnlockRect();
						   } );


			if ( !srcLock.pBits || !dstLock.pBits )
			{
				return E_FAIL;
			}


			const uint32_t rows = std::min( src.GetHeight(), dst.GetHeight() ) / ( isCompressed ? 4 : 1 );
			const uint32_t pitch = std::min( std::min<uint32_t>( srcLock.Pitch, dstLock.Pitch ),
											 isCompressed	? std::min( src.GetWidth(),
																		dst.GetWidth() ) / 4 * blockSize
											 : std::min( src.GetWidth(), dst.GetWidth() ) * byteCount );

			for ( uint32_t row = 0; row != rows; ++row )
			{
				memcpy( ( BYTE* )dstLock.pBits + row * dstLock.Pitch,
						( BYTE* )srcLock.pBits + row * srcLock.Pitch,
						pitch );
			}
		}

		if ( mip + 1 != mipCount )
		{
			AdvanceMip( src, source, mip );
			AdvanceMip( dst, *this, mip );
		}
	}

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
	if ( !rt.IsValid() || !IsValid() || m_type != TEX_TYPE_2D || GetFormat() != rt.GetFormat() || rt.IsCompressed() )
	{
		return E_FAIL;
	}

	if ( !renderContext.m_d3dDevice9 )
	{
		return E_FAIL;
	}

	Tr2TextureSubresource src = sourceSubresource;
	Tr2TextureSubresource dst = destSubresource;

	if ( !Crop( src, rt.GetTexture(), dst, *this ) )
	{
		return E_FAIL;
	}

	CComQIPtr<IDirect3DTexture9, &IID_IDirect3DTexture9> tex2D( m_texture );
	if ( !tex2D )
	{
		return E_FAIL;
	}

	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	AL_UPDATE_RESOURCE_FRAME_USAGE( rt );

	const uint32_t byteCount = GetBytesPerPixel( GetFormat() );
	const uint32_t mipCount = std::min( src.GetMipCount(), dst.GetMipCount() );

	for ( uint32_t mip = 0; mip != mipCount; ++mip )
	{
		void* srcData = nullptr;
		uint32_t srcPitch;
		uint32_t srcLtrb[4] =
		{
			src.m_left,
			src.m_top,
			src.m_right,
			src.m_bottom
		};
		CR_RETURN_HR( rt.Lock( src.m_startMipLevel + mip, srcLtrb, srcData, srcPitch, renderContext ) );
		ON_BLOCK_EXIT( [&]{
						rt.Unlock( renderContext );
					   } );


		void* destData = nullptr;
		uint32_t destPitch;
		uint32_t destLtrb[4] =
		{
			dst.m_left,
			dst.m_top,
			dst.m_right,
			dst.m_bottom
		};

		D3DLOCKED_RECT lockedRect;
		uint32_t mipLevel = dst.m_startMipLevel + mip;
		CR_RETURN_HR( tex2D->LockRect( mipLevel, &lockedRect, reinterpret_cast<RECT*>( destLtrb ), 0 ) );
		ON_BLOCK_EXIT( [&]{
						tex2D->UnlockRect( mipLevel );
					   } );

		destData = lockedRect.pBits;
		destPitch = lockedRect.Pitch;

		// check for lying drivers
		if ( !srcData || !destData )
		{
			return E_FAIL;
		}


		const uint32_t rows = std::min( src.GetHeight(), dst.GetHeight() );
		const uint32_t pitch = std::min( std::min( srcPitch, destPitch ),
										 std::min( src.GetWidth(), dst.GetWidth() ) * byteCount );

		for ( uint32_t row = 0; row != rows; ++row )
		{
			memcpy( ( char* )destData + row * destPitch,
					( char* )srcData + row * srcPitch,
					pitch );
		}

		if ( mip + 1 != mipCount )
		{
			AdvanceMip( src, rt.GetTexture(), mip );
			AdvanceMip( dst, *this, mip );
		}
	}

	return S_OK;
}

ALResult Tr2TextureAL::GetSurfaceLevel( uint32_t face,
										uint32_t mipLevel )
{
	if ( !m_texture || m_type != TEX_TYPE_2D && m_type != TEX_TYPE_CUBE )
	{
		return E_FAIL;
	}
	if ( m_lockedMipLevelSurf != nullptr )
	{
		return E_FAIL;
	}

	CComQIPtr<IDirect3DTexture9, &IID_IDirect3DTexture9> tex2D( m_texture );
	if ( tex2D )
	{
		m_lockedMipLevel = mipLevel;

		return tex2D->GetSurfaceLevel( mipLevel, &m_lockedMipLevelSurf );
	}
	else
	{
		CComQIPtr<IDirect3DCubeTexture9, &IID_IDirect3DCubeTexture9> texCube( m_texture );
		CCP_ASSERT( texCube );
		if ( !texCube )
		{
			return E_FAIL;
		}

		m_lockedMipLevel = mipLevel;

		return texCube->GetCubeMapSurface( D3DCUBEMAP_FACES( face ), mipLevel, &m_lockedMipLevelSurf );
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
	if ( m_currentLock != LOCK_INVALID )
	{
		CCP_AL_LOGERR( "Attempting to lock already locked texture" );
		return E_FAIL;
	}
	if ( m_type != TEX_TYPE_CUBE && face > 0 )
	{
		return E_FAIL;
	}

	if ( ltrb &&
		 ltrb[0] == 0 &&
		 ltrb[1] == 0 &&
		 ltrb[2] == GetWidth() &&
		 ltrb[3] == GetHeight() )
	{
		ltrb = nullptr;
	}

	switch( lockType )
	{
	case LOCK_READONLY:
		return LockReading( face, mipLevel, ltrb, data, pitch, renderContext );
	case LOCK_WRITEONLY:
		return LockWriting( face, mipLevel, ltrb, data, pitch, renderContext );
	default:
		return E_INVALIDARG;
	}
}

ALResult Tr2TextureAL::Unlock( Tr2RenderContextAL& renderContext )
{
	switch ( m_currentLock )
	{
	case LOCK_READONLY:
		{
			return UnlockReading( renderContext );
		}
	case LOCK_WRITEONLY:
		{
			return UnlockWriting( renderContext );
		}
	}

	CCP_AL_LOGERR( "Trying to Unlock buffer that's not locked" );
	return E_FAIL;
}

ALResult Tr2TextureAL::LockImpl( uint32_t face,
								 uint32_t mipLevel,
								 uint32_t* ltrb,
								 void*& data,
								 uint32_t& pitch,
								 Tr2RenderContextAL&/*renderContext*/,
								 uint32_t d3dLockType )
{
	data = nullptr;
	pitch = 0;

	if ( m_type != TEX_TYPE_2D && m_type != TEX_TYPE_CUBE )
	{
		return E_NOTIMPL;
	}
	HRESULT hr = GetSurfaceLevel( face, mipLevel );
	if ( FAILED( hr ) )
	{
		return hr;
	}
	D3DLOCKED_RECT lr;
	if ( !ltrb )
	{
		hr = m_lockedMipLevelSurf->LockRect( &lr, nullptr, d3dLockType );
	}
	else
	{
		const RECT r =
		{
			ltrb[0],
			ltrb[1],
			ltrb[2],
			ltrb[3]
		};
		hr = m_lockedMipLevelSurf->LockRect( &lr, &r, d3dLockType );
	}
	if ( SUCCEEDED( hr ) && lr.pBits )
	{
		data = lr.pBits;
		pitch = lr.Pitch;
	}
	else
	{
		m_lockedMipLevelSurf = nullptr;
	}
	return hr;
}

ALResult Tr2TextureAL::LockReading( uint32_t face,
									uint32_t mipLevel,
									uint32_t* ltrb,
									void*& data,
									uint32_t& pitch,
									Tr2RenderContextAL& renderContext )
{
	if ( ( m_usage & USAGE_CPU_READ ) == 0 && ( m_usage & USAGE_IMMUTABLE ) == 0 )
	{
		return E_FAIL;
	}

	HRESULT hr = LockImpl( face, mipLevel, ltrb, data, pitch, renderContext, D3DLOCK_READONLY );
	if ( SUCCEEDED( hr ) )
	{
		m_currentLock = LOCK_READONLY;
	}
	return hr;
}

ALResult Tr2TextureAL::UnlockReading( Tr2RenderContextAL& /*renderContext*/ )
{
	if ( !m_lockedMipLevelSurf )
	{
		return E_FAIL;
	}
	HRESULT hr = m_lockedMipLevelSurf->UnlockRect();
	m_lockedMipLevelSurf = nullptr;
	m_lockedMipLevel = 0;
	m_currentLock = LOCK_INVALID;
	return hr;
}

ALResult Tr2TextureAL::LockWriting( uint32_t face,
									uint32_t mipLevel,
									uint32_t* ltrb,
									void*& data,
									uint32_t& pitch,
									Tr2RenderContextAL& renderContext )
{
	CCP_ASSERT( m_usage != USAGE_IMMUTABLE );

	HRESULT hr = LockImpl( face, mipLevel, ltrb, data, pitch, renderContext, ( m_usage & USAGE_LOCK_FREQUENTLY ) ?
						   D3DLOCK_DISCARD : 0 );
	if ( SUCCEEDED( hr ) )
	{
		m_currentLock = LOCK_WRITEONLY;
	}
	return hr;
}

ALResult Tr2TextureAL::UnlockWriting( Tr2RenderContextAL& renderContext )
{
	return UnlockReading( renderContext );
}

#endif
