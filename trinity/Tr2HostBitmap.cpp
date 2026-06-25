// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Include/TriMath.h"
#include "Tr2RenderTarget.h"
#include "Resources/TriTextureRes.h"
#include "Tr2HostBitmap.h"
#include "Tr2DxtCompressor.h"

#pragma warning( push, 4 )
#pragma warning( disable : 4189 ) // Scopeguard

using namespace Tr2RenderContextEnum;

Tr2HostBitmap::Tr2HostBitmap( IRoot* )
{
}

Tr2HostBitmap::~Tr2HostBitmap()
{
	Destroy();
}

void Tr2HostBitmap::Destroy()
{
	CleanupAsyncSave( false );
	ImageIO::HostBitmap::Destroy();
}

ALResult Tr2HostBitmap::ChangeFormatFromScript( Tr2RenderContextEnum::PixelFormat format )
{
	return ChangeFormat( format ) ? S_OK : E_FAIL;
}

bool Tr2HostBitmap::CopyFromRenderTarget( Tr2TextureAL& rtAL, Tr2RenderContext& renderContext )
{
	return m_type == TEX_TYPE_2D ? SharedCopyFaceFromRenderTarget( CUBEMAP_FACE_FIRST, rtAL, nullptr, 0, 0, renderContext ) : false;
}

bool Tr2HostBitmap::CopyFromRenderTarget( Tr2TextureAL& rtAL, const int* srcRect, int offsetX, int offsetY, Tr2RenderContext& renderContext )
{
	return m_type == TEX_TYPE_2D ? SharedCopyFaceFromRenderTarget( CUBEMAP_FACE_FIRST, rtAL, srcRect, offsetX, offsetY, renderContext ) : false;
}

bool Tr2HostBitmap::CopyFromRenderTargetPython( Tr2RenderTarget* rt )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !rt )
	{
		return false;
	}
	return CopyFromRenderTarget( *rt, renderContext );
}

bool Tr2HostBitmap::CopyFaceFromRenderTarget( Tr2RenderContextEnum::CubemapFace face, Tr2TextureAL& rtAL, Tr2RenderContext& renderContext )
{
	return m_type == TEX_TYPE_CUBE ? SharedCopyFaceFromRenderTarget( face, rtAL, nullptr, 0, 0, renderContext ) : false;
}

bool Tr2HostBitmap::SharedCopyFaceFromRenderTarget( Tr2RenderContextEnum::CubemapFace face, Tr2TextureAL& rtAL, const int* srcRect, int offsetX, int offsetY, Tr2RenderContext& renderContext )
{
	bool alphaConvert = false;
	if( !CheckForMatch( rtAL.GetDesc(), srcRect == nullptr, alphaConvert, "CopyFromRenderTarget" ) )
	{
		return false;
	}

	if( !rtAL.IsValid() || IsCompressed() )
	{
		CCP_LOGWARN( "CopyFromRenderTarget: invalid source" );
		return false;
	}

	int left = 0;
	int top = 0;
	int right = rtAL.GetWidth();
	int bottom = rtAL.GetHeight();

	if( srcRect )
	{
		left = max( left, srcRect[0] );
		top = max( top, srcRect[1] );
		right = min( right, srcRect[2] );
		bottom = min( bottom, srcRect[3] );
	}
	if( offsetX < 0 )
	{
		left = max( left, -offsetX );
		right = min( right, int( rtAL.GetWidth() ) + offsetX );
	}
	if( offsetY < 0 )
	{
		top = max( top, -offsetY );
		bottom = min( bottom, int( rtAL.GetHeight() ) + offsetY );
	}
	offsetX += left;
	offsetY += top;

	if( left >= right ||
		top >= bottom ||
		int( m_width ) < offsetX ||
		int( m_height ) < offsetY )
	{
		return true;
	}

	unsigned width = min( unsigned( right - left ), m_width - offsetX );
	unsigned height = min( unsigned( bottom - top ), m_height - offsetY );

	const unsigned mipCount = std::min( GetTrueMipCount(), rtAL.GetTrueMipCount() );

	for( unsigned mipLevel = 0; mipLevel != mipCount; ++mipLevel )
	{
		const void* data;
		unsigned srcPitch;
		if( FAILED( rtAL.MapForReading( Tr2TextureSubresource( mipLevel ), data, srcPitch, renderContext ) ) )
		{
			CCP_LOGWARN( "SharedCopyFaceFromRenderTarget: failed to lock renderTarget level %d", mipLevel );
			return false;
		}

		uint8_t* dst = (uint8_t*)GetMipRawData( mipLevel, face );
		const uint8_t* src = (uint8_t*)data;

		const unsigned dstPitch = GetMipPitch( mipLevel );

		dst += GetBytesPerPixel( m_format ) * offsetX + dstPitch * offsetY;
		src += GetBytesPerPixel( m_format ) * left + srcPitch * top;

		if( alphaConvert )
		{
			for( unsigned j = 0; j != height; ++j, src += srcPitch, dst += dstPitch )
			{
				const uint8_t* in = src;
				uint8_t* out = dst;
				for( unsigned i = 0; i != width; ++i )
				{
					*out++ = *in++;
					*out++ = *in++;
					*out++ = *in++;
					*out++ = 0xFF;
					++in;
				}
			}
		}
		else
		{
			const unsigned pitch = width * GetBytesPerPixel( m_format );
			for( unsigned j = 0; j != height; ++j, src += srcPitch, dst += dstPitch )
			{
				memcpy( dst, src, pitch );
			}
		}

		width = max( width / 2, 1u );
		height = max( height / 2, 1u );
		offsetX /= 2;
		offsetY /= 2;
		left /= 2;
		top /= 2;

		rtAL.UnmapForReading( renderContext );
	}
	return true;
}

bool Tr2HostBitmap::CopyFaceFromRenderTargetPython( unsigned face, Tr2RenderTarget* rt )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !rt )
	{
		return false;
	}
	return CopyFaceFromRenderTarget( Tr2RenderContextEnum::CubemapFace( face ), *rt, renderContext );
}

bool Tr2HostBitmap::CopyFromRenderTargetRegionPython( Tr2RenderTarget* rt, int left, int top, int right, int bottom, unsigned offsetX, unsigned offsetY )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !rt )
	{
		return false;
	}
	int rect[] = { left, top, right, bottom };
	return CopyFromRenderTarget( *rt, rect, offsetX, offsetY, renderContext );
}

bool Tr2HostBitmap::CopyFromTexture( Tr2TextureAL& texture, Tr2RenderContext& renderContext )
{
	if( !texture.IsValid() )
	{
		return false;
	}

	if( texture.GetType() != TEX_TYPE_2D && texture.GetType() != TEX_TYPE_CUBE )
	{
		CCP_LOGERR( "Tr2HostBitmap::CopyFromTextureRes, only 2D and CUBE textures supported" );
		return false;
	}

	bool alphaConvert = false;

	if( GetType() != texture.GetType() || !CheckForMatch( texture.GetDesc(), true, alphaConvert, "CopyFromTextureRes" ) )
	{
		return false;
	}

	const uint32_t mipCount = std::min( GetTrueMipCount(), texture.GetTrueMipCount() );
	const uint32_t faceCount = GetArraySize();


	for( uint32_t face = 0; face != faceCount; ++face )
	{
		for( uint32_t mipLevel = 0; mipLevel != mipCount; ++mipLevel )
		{
			const void* srcData = nullptr;
			uint32_t srcPitch = 0;

			HRESULT hr = texture.MapForReading( Tr2TextureSubresource( face, mipLevel ), srcData, srcPitch, renderContext );
			if( FAILED( hr ) || srcData == nullptr )
			{
				CCP_LOGERR( "Tr2HostBitmap::CopyFromTextureRes, error locking surface" );
				return false;
			}


			uint8_t* dst = (uint8_t*)GetMipRawData( mipLevel, face );
			const uint8_t* src = (uint8_t*)srcData;

			const uint32_t dstPitch = GetMipPitch( mipLevel );

			if( alphaConvert )
			{
				const uint32_t width = GetMipWidth( mipLevel );
				const uint32_t height = GetMipHeight( mipLevel );
				for( uint32_t j = 0; j != height; ++j, src += srcPitch, dst += dstPitch )
				{
					const uint8_t* in = src;
					uint8_t* out = dst;
					for( uint32_t i = 0; i != width; ++i )
					{
						*out++ = *in++;
						*out++ = *in++;
						*out++ = *in++;
						*out++ = 0xFF;
						++in;
					}
				}
			}
			else
			{
				const uint32_t height = IsCompressed() ? GetMipHeight( mipLevel ) / 4 : GetMipHeight( mipLevel );
				for( uint32_t j = 0; j != height; ++j, src += srcPitch, dst += dstPitch )
				{
					memcpy( dst, src, dstPitch );
				}
			}


			texture.UnmapForReading( renderContext );
		}
	}

	return true;
}

bool Tr2HostBitmap::CopyFromTextureRes( TriTextureRes& res, Tr2RenderContext& renderContext )
{
	if( !res.GetTexture() )
	{
		return false;
	}

	if( res.GetType() != TEX_TYPE_2D && res.GetType() != TEX_TYPE_CUBE )
	{
		CCP_LOGERR( "Tr2HostBitmap::CopyFromTextureRes, only 2D and CUBE textures supported" );
		return false;
	}

	if( !res.GetTexture() && res.GetType() == TEX_TYPE_CUBE )
	{
		CCP_LOGERR( "Tr2HostBitmap::CopyFromTextureRes, legacy CUBE textures are not supported" );
		return false;
	}

	bool alphaConvert = false;

	if( res.GetTexture() )
	{
		if( !res.GetTexture()->IsValid() || GetType() != res.GetTexture()->GetType() || !CheckForMatch( res.GetTexture()->GetDesc(), true, alphaConvert, "CopyFromTextureRes" ) )
		{
			return false;
		}
	}

	const uint32_t mipCount = std::min( GetTrueMipCount(), res.GetTrueMipCount() );
	const uint32_t faceCount = GetArraySize();


	for( uint32_t face = 0; face != faceCount; ++face )
	{
		for( uint32_t mipLevel = 0; mipLevel != mipCount; ++mipLevel )
		{
			const void* srcData = nullptr;
			uint32_t srcPitch = 0;

			HRESULT hr = E_FAIL;

			if( res.m_wrappedRenderTarget )
			{
				hr = res.m_wrappedRenderTarget->GetRenderTarget().MapForReading( Tr2TextureSubresource( face, mipLevel ), srcData, srcPitch, renderContext );
			}
			else if( res.GetTexture() )
			{
				hr = res.GetTexture()->MapForReading( Tr2TextureSubresource( face, mipLevel ), srcData, srcPitch, renderContext );
			}
			if( FAILED( hr ) || srcData == nullptr )
			{
				CCP_LOGERR( "Tr2HostBitmap::CopyFromTextureRes, error locking surface" );
				return false;
			}


			uint8_t* dst = (uint8_t*)GetMipRawData( mipLevel, face );
			const uint8_t* src = (uint8_t*)srcData;

			const uint32_t dstPitch = GetMipPitch( mipLevel );

			if( alphaConvert )
			{
				const uint32_t width = GetMipWidth( mipLevel );
				const uint32_t height = GetMipHeight( mipLevel );
				for( uint32_t j = 0; j != height; ++j, src += srcPitch, dst += dstPitch )
				{
					const uint8_t* in = src;
					uint8_t* out = dst;
					for( uint32_t i = 0; i != width; ++i )
					{
						*out++ = *in++;
						*out++ = *in++;
						*out++ = *in++;
						*out++ = 0xFF;
						++in;
					}
				}
			}
			else
			{
				const uint32_t height = IsCompressed() ? GetMipHeight( mipLevel ) / 4 : GetMipHeight( mipLevel );
				for( uint32_t j = 0; j != height; ++j, src += srcPitch, dst += dstPitch )
				{
					memcpy( dst, src, dstPitch );
				}
			}


			if( res.m_wrappedRenderTarget )
			{
				res.m_wrappedRenderTarget->GetRenderTarget().UnmapForReading( renderContext );
			}
			else if( res.GetTexture() )
			{
				res.GetTexture()->UnmapForReading( renderContext );
			}
		}
	}

	return true;
}

bool Tr2HostBitmap::CopyFromTextureResPython( TriTextureRes* tr )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !tr )
	{
		return false;
	}
	return CopyFromTextureRes( *tr, renderContext );
}

/// --------------------------------------------------
/// Description:
///   Take the pixels in the sub-block (margin, margin)...(width-margin,height-margin) and copy their
///   values into the border pixels to get a clamping effect.
/// --------------------------------------------------
bool Tr2HostBitmap::PopulateMargin( unsigned margin )
{
	if( !IsValid() || IsCompressed() || m_mipCount != 1 || 2 * margin >= GetWidth() || 2 * margin >= GetHeight() )
	{
		return false;
	}

	const unsigned bytesPerPixel = GetBytesPerPixel( GetFormat() );
	const unsigned bytesPerMargin = bytesPerPixel * margin;


	const unsigned width = GetWidth() - 2 * margin;
	const unsigned height = GetHeight() - 2 * margin;

	//top margin
	const char* src = GetRawData( margin, margin );
	for( unsigned i = 0; i != margin; ++i )
	{
		char* dst = GetRawData( margin, i );
		memcpy( dst, src, bytesPerPixel * width );
	}

	//bottom margin
	src = GetRawData( margin, height + margin - 1 );
	for( unsigned i = 0; i != margin; ++i )
	{
		char* dst = GetRawData( margin, height + margin + i );
		memcpy( dst, src, bytesPerPixel * width );
	}

	for( unsigned y = 0; y != height; ++y )
	{
		//left margin
		src = GetRawData( margin, y + margin );
		char* dst = GetRawData( 0, y + margin );
		for( unsigned i = 0; i != margin; ++i )
		{
			for( unsigned j = 0; j != bytesPerPixel; ++j )
			{
				dst[i * bytesPerPixel + j] = src[j];
			}
		}

		//right margin
		src += ( width - 1 ) * bytesPerPixel;
		dst += width * bytesPerPixel + bytesPerMargin;
		for( unsigned i = 0; i != margin; ++i )
		{
			for( unsigned j = 0; j != bytesPerPixel; ++j )
			{
				dst[i * bytesPerPixel + j] = src[j];
			}
		}
	}

	return true;
}

bool Tr2HostBitmap::Save( const wchar_t* path )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !IsValid() )
	{
		CCP_LOGWARN( "Tr2HostBitmap::Save not a valid bitmap" );
		return false;
	}

	Be::Clsid resFileClsid( "blue", "ResFile" );
	IResFilePtr stream( resFileClsid );
	if( !( stream->FileExistsW( path ) ? stream->OpenW( path, false ) : stream->CreateW( path ) ) )
	{
		CCP_LOGWARN( "Tr2HostBitmap::Save failed to open Blue stream (%S)", path );
		return false;
	}
	ON_BLOCK_EXIT( [&] { stream->Close(); } );

	return ImageIO::SaveImage( path, *this, *stream );
}

bool Tr2HostBitmap::SaveAsync( const wchar_t* path )
{
	return StartAsyncSave( path );
}

bool Tr2HostBitmap::DoPrepareAsyncSave()
{
	return true;
}

bool Tr2HostBitmap::DoExecuteAsyncSave()
{
	return Save( m_saveFilename.c_str() );
}

void Tr2HostBitmap::DoCleanupAsyncSave()
{
}

bool Tr2HostBitmap::CreateFromFile( const std::wstring& file )
{
	CleanupAsyncSave( false );
	Destroy();

	if( file.empty() )
	{
		return false;
	}

	IBlueStreamPtr stream;
	if( !BeIsSuccess( BePaths->GetFileContentsWithYield( file.c_str(), &stream ) ) )
	{
		CCP_LOGERR( "Error opening file (%S)", file.c_str() );
		return false;
	}

	if( !ImageIO::ReadImage( *stream, ImageIO::LoadParameters( file.c_str() ), *this ) )
	{
		CCP_LOGERR( "Error reading file (%S)", file.c_str() );
		return false;
	}

	return true;
}

bool Tr2HostBitmap::Compress( unsigned compressionFormat, unsigned qualityLevel, TriTextureRes* output )
{
	if( !output || !IsValid() || GetType() != TEX_TYPE_2D || compressionFormat >= TR2DXT_COMPRESS_COUNT )
	{
		return false;
	}

	static const PixelFormat format[TR2DXT_COMPRESS_COUNT] = {
		PIXEL_FORMAT_BC1_UNORM, // TR2DXT_COMPRESS_RT_DXT1			= 0,
		PIXEL_FORMAT_BC3_UNORM, // TR2DXT_COMPRESS_RT_DXT5			= 1,
		PIXEL_FORMAT_BC3_UNORM, // TR2DXT_COMPRESS_RT_DXT5N			= 2,
		PIXEL_FORMAT_BC3_UNORM, // TR2DXT_COMPRESS_RT_YCOCGDXT5		= 3,
		PIXEL_FORMAT_BC3_UNORM, // TR2DXT_COMPRESS_RT_3DC			= 4,
		PIXEL_FORMAT_BC1_UNORM, // TR2DXT_COMPRESS_SQUISH_DXT1		= 5,
		PIXEL_FORMAT_BC1_UNORM, // TR2DXT_COMPRESS_SQUISH_DXT3		= 6,
		PIXEL_FORMAT_BC3_UNORM, // TR2DXT_COMPRESS_SQUISH_DXT5		= 7,
		PIXEL_FORMAT_BC4_UNORM, // TR2DXT_COMPRESS_SQUISH_KBC4		= 8,
		PIXEL_FORMAT_BC5_UNORM, // TR2DXT_COMPRESS_SQUISH_KBC5		= 9,
	};

	{
		unsigned pitch = ( m_width + 3 ) / 4 * Tr2RenderContextEnum::GetBlockByteSize( format[compressionFormat] );
		CcpMallocBuffer destination( "Tr2HostBitmap::Compress", pitch * ( m_height + 3 ) / 4 );

		Tr2DxtCompressControl* control = CCP_NEW( "TriDevice::CompressSurface/control" ) Tr2DxtCompressControl;
		ON_BLOCK_EXIT( [&] { CCP_DELETE( control ); } );

		if( !Tr2DxtCompressSurfaceAsync( (Tr2DxtCompressionFormat)compressionFormat,
										 (uint8_t*)GetRawData(),
										 m_width,
										 m_height,
										 (uint8_t*)destination.get(),
										 pitch,
										 control,
										 qualityLevel ) )
		{
			return false;
		}

		Tr2HostBitmapPtr bitmap;
		bitmap.CreateInstance();
		bitmap->Create( m_width, m_height, 1, format[compressionFormat] );
		memcpy( bitmap->GetRawData(), destination.get(), bitmap->GetRawDataSize() );
		output->CreateFromHostBitmap( bitmap );
	}
	return true;
}


bool chan_selected( const std::string& channels, char channel )
{
	return channels.find_first_of( channel ) != std::string::npos;
}


bool check_chan_value( uint32_t mask, uint32_t shift, uint32_t pixval, uint32_t testval )
{
	uint32_t chan_val = ( pixval & mask ) >> shift;
	return chan_val == testval;
}


uint32_t Tr2HostBitmap::CountPixelsOfValue( const std::string& channels, uint32_t value ) const
{
	// Count the number of pixels for which all rgba channels listed in "channels" param match value.

	if( !IsValid() || GetBytesPerPixel( m_format ) != 4 || IsCompressed() )
	{
		return 0;
	}

	uint32_t numpix = 0;

	for( uint32_t x = 0; x < m_width; x++ )
	{
		for( uint32_t y = 0; y < m_height; y++ )
		{
			uint32_t pixval;
			if( GetPixel( x, y, &pixval ) )
			{
				bool matches = true;
				if( chan_selected( channels, 'a' ) )
				{
					matches &= check_chan_value( 0xFF000000, 24, pixval, value );
				}

				if( chan_selected( channels, 'r' ) )
				{
					matches &= check_chan_value( 0x00FF0000, 16, pixval, value );
				}

				if( chan_selected( channels, 'g' ) )
				{
					matches &= check_chan_value( 0x0000FF00, 8, pixval, value );
				}

				if( chan_selected( channels, 'b' ) )
				{
					matches &= check_chan_value( 0x000000FF, 0, pixval, value );
				}

				if( matches )
				{
					numpix++;
				}
			}
		}
	}

	return numpix;
}

bool Tr2HostBitmap::IsMonochrome() const
{
	if( !IsValid() || GetBytesPerPixel( m_format ) != 4 || IsCompressed() )
	{
		return false;
	}

	for( uint32_t x = 0; x < m_width; x++ )
	{
		for( uint32_t y = 0; y < m_height; y++ )
		{
			uint32_t pixval;
			if( GetPixel( x, y, &pixval ) )
			{
				bool matches = true;
				uint32_t red_val = ( pixval & 0x00FF0000 ) >> 16;
				matches &= check_chan_value( 0x0000FF00, 8, pixval, red_val );
				matches &= check_chan_value( 0x000000FF, 0, pixval, red_val );
				if( !matches )
				{
					return false;
				}
			}
		}
	}
	return true;
}

bool Tr2HostBitmap::SetPixel( int x, int y, const void* data )
{
	if( !IsValid() || GetBytesPerPixel( m_format ) != 4 || IsCompressed() )
	{
		return false;
	}
	if( x < 0 || (unsigned)x > m_width ||
		y < 0 || (unsigned)y > m_height )
	{
		return false;
	}

	char* dest = m_data.get() + y * m_width * 4 + x * 4;
	memcpy( dest, data, 4 );
	return true;
}

bool Tr2HostBitmap::GetPixel( int x, int y, void* data ) const
{
	if( !IsValid() || GetBytesPerPixel( m_format ) != 4 || IsCompressed() )
	{
		return false;
	}
	if( x < 0 || (unsigned)x > m_width ||
		y < 0 || (unsigned)y > m_height )
	{
		return false;
	}

	char* src = m_data.get() + y * m_width * 4 + x * 4;
	memcpy( data, src, 4 );
	return true;
}

#if BLUE_WITH_PYTHON
bool Tr2HostBitmap::SetPixelPy( PyObject* tuple )
{
	static_assert( sizeof( int ) == 4, "Assuming that RGBA8 fits in int" );
	int x, y, val;
	if( !PyArg_ParseTuple( tuple, "iii", &x, &y, &val ) )
	{
		return false;
	}

	return SetPixel( x, y, &val );
}

PyObject* Tr2HostBitmap::GetPixelPy( PyObject* tuple )
{
	int x, y, val = 0;
	if( !PyArg_ParseTuple( tuple, "ii", &x, &y ) ||
		!GetPixel( x, y, &val ) )
	{
		Py_RETURN_NONE;
	}
	return ToPython( val );
}

PyObject* Tr2HostBitmap::PySetPixel( PyObject* args )
{
	if( PyTuple_Size( args ) != 1 )
	{
		SetPixelPy( args );
	}
	else
	{
		PyObject* list;
		PyArg_ParseTuple( args, "O", &list );
		if( !PySequence_Check( list ) )
		{
			PyErr_SetString( PyExc_TypeError, "Expected sequence of tuples" );
			return nullptr;
		}
		Py_ssize_t len = PySequence_Size( list );
		for( Py_ssize_t i = 0; i < len; i++ )
		{
			BluePy tuple( PySequence_GetItem( list, i ) );
			if( !tuple || !SetPixelPy( tuple ) )
			{
				return 0;
			}
		}
	}
	Py_RETURN_NONE;
}

PyObject* Tr2HostBitmap::PyGetPixel( PyObject* args )
{
	if( PyTuple_Size( args ) != 1 )
	{
		return GetPixelPy( args );
	}
	else
	{
		PyObject* list;
		PyArg_ParseTuple( args, "O", &list );
		if( !PySequence_Check( list ) )
		{
			PyErr_SetString( PyExc_TypeError, "Expected sequence of tuples" );
			return nullptr;
		}
		Py_ssize_t len = PySequence_Size( list );
		BluePyTuple res( len );
		for( Py_ssize_t i = 0; i < len; i++ )
		{
			BluePy tuple( PySequence_GetItem( list, i ) );
			if( !tuple )
			{
				return 0;
			}
			BluePy obj( GetPixelPy( tuple ) );
			if( !obj )
			{
				return 0;
			}
			res.Set( i, obj );
		}
		return res.Detach();
	}
}

PyObject* Tr2HostBitmap::PySetPixels( PyObject* args )
{
	// legacy method for cramming some pixels into a simple RGBA8 blob from python
	if( !IsValid() || GetBytesPerPixel( GetFormat() ) != 4 || GetType() != TEX_TYPE_2D || GetTrueMipCount() != 1 )
	{
		Py_RETURN_NONE;
	}

	int background = 0;
	PyObject* pixelsO = 0;
	PyObject* discard = 0;
	if( !PyArg_ParseTuple( args, "i|OO:SetPixels", &background, &pixelsO, &discard ) )
	{
		return 0;
	}

	BluePy fast( PySequence_Fast( pixelsO, "Expected sequence of tuples" ) );
	if( pixelsO && !fast )
	{
		return 0;
	}

	for( unsigned y = 0; y != m_height; ++y )
	{
		for( unsigned x = 0; x != m_width; ++x )
		{
			*(int*)GetRawData( x, y ) = background;
		}
	}

	Py_ssize_t size = PySequence_Fast_GET_SIZE( (PyObject*)fast );
	for( Py_ssize_t i = 0; i < size; i++ )
	{
		PyObject* tuple = PySequence_Fast_GET_ITEM( (PyObject*)fast, i );
		int x = 0, y = 0;
		unsigned val;
		if( !PyArg_ParseTuple( tuple, "iiI:SetPixel", &x, &y, &val ) )
		{
			return 0;
		}
		if( x >= 0 && (unsigned)x < m_width &&
			y >= 0 && (unsigned)y < m_height )
		{
			*(unsigned*)GetRawData( x, y ) = val;
		}
	}

	Py_RETURN_NONE;
}

// --------------------------------------------------------------------------------------
// Description:
//   Creates a height map by scaling up source data to fill the bitmap
//   with bicubic interpolation.
//   Imitates the result of contour layer from chartdirector used by planetary interaction.
// Arguments:
//   data - source data, values from 0 to 1
//   width - width of the source data
//   height - height of the source data
// Return Value:
//   true if successful
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2HostBitmap::CreateFromHeightData( const std::vector<float>& data, int32_t width, int32_t height )
{
	float scaleX = float( width - 1 ) / float( m_width );
	float scaleY = float( height - 1 ) / float( m_height );

	int32_t yIndex[4];
	float yValues[4];

	int components = GetBytesPerPixel( m_format );
	if( !IsValid() || !( components == 4 || components == 1 ) || IsCompressed() )
	{
		return false;
	}

	for( uint32_t y = 0; y < m_height; y++ )
	{
		float sy = y * scaleY;
		int32_t dataY = int32_t( sy );
		sy = sy - dataY;

		for( int32_t yi = 0; yi < 4; yi++ )
		{
			// Clamp the y axis
			yIndex[yi] = ClampInt( dataY + yi - 1, 0, width - 1 ) * width;
		}

		for( uint32_t x = 0; x < m_width; x++ )
		{
			float sx = x * scaleX;
			int32_t dataX = int32_t( sx );
			sx = sx - dataX;

			for( int32_t i = 0; i < 4; i++ )
			{
				int32_t index = dataX + i - 1;
				// Wrap the x axis
				int32_t xIdx = index < 0 ? width - ( ( -index ) % width ) : index % width;
				yValues[i] = CubicInterpolate( data[yIndex[0] + xIdx], data[yIndex[1] + xIdx], data[yIndex[2] + xIdx], data[yIndex[3] + xIdx], sy );
			}
			uint8_t value = uint8_t( TriClamp( CubicInterpolate( yValues[0], yValues[1], yValues[2], yValues[3], sx ), 0.f, 1.f ) * 255 );

			if( components == 4 )
			{
				uint32_t d = ( value << 24 ) + ( value << 16 ) + ( value << 8 ) + value;
				char* dest = m_data.get() + y * m_width * 4 + x * 4;
				memcpy( dest, &d, 4 );
			}
			else
			{
				char* dest = m_data.get() + y * m_width + x;
				*dest = value;
			}
		}
	}
	return true;
}
#endif
