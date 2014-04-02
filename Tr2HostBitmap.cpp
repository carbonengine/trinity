#include "StdAfx.h"

#include "Tr2RenderTarget.h"
#if DEPRECATED_ENABLED
#include "TriConvolutionMatrix5.h"
#endif
#include "Resources/TriTextureRes.h"
#include "Tr2HostBitmap.h"
#include "Tr2DxtCompressor.h"

#pragma warning( push, 4 )
#pragma warning( disable: 4189 )	// Scopeguard

using namespace Tr2RenderContextEnum;

Tr2HostBitmap::Tr2HostBitmap( IRoot* )
{}

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

bool Tr2HostBitmap::CopyFromRenderTarget( Tr2RenderTargetAL& rtAL, Tr2RenderContext& renderContext )
{
	return m_type == TEX_TYPE_2D ? SharedCopyFaceFromRenderTarget( CUBEMAP_FACE_FIRST, rtAL, nullptr, 0, 0, renderContext ) : false;
}

bool Tr2HostBitmap::CopyFromRenderTarget( Tr2RenderTargetAL& rtAL, const int* srcRect, int offsetX, int offsetY, Tr2RenderContext& renderContext )
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

bool Tr2HostBitmap::CopyFaceFromRenderTarget( Tr2RenderContextEnum::CubemapFace face, Tr2RenderTargetAL& rtAL, Tr2RenderContext& renderContext )
{
	return m_type == TEX_TYPE_CUBE ? SharedCopyFaceFromRenderTarget( face, rtAL, nullptr, 0, 0, renderContext ) : false;
}

bool Tr2HostBitmap::SharedCopyFaceFromRenderTarget( Tr2RenderContextEnum::CubemapFace face, Tr2RenderTargetAL& rtAL, const int* srcRect, int offsetX, int offsetY, Tr2RenderContext& renderContext )
{
	bool alphaConvert = false;
	if( !CheckForMatch( rtAL, srcRect == nullptr, alphaConvert, "CopyFromRenderTarget" ) )
	{
		return false;
	}

	if( !rtAL.GetTexture().IsValid() || IsCompressed() )
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
		int( m_height ) < offsetY
		)
	{
		return true;
	}

	unsigned width  = min( unsigned( right - left ), m_width - offsetX );
	unsigned height = min( unsigned( bottom - top ), m_height - offsetY );

	const unsigned mipCount = std::min( GetTrueMipCount(), rtAL.GetTrueMipCount() );

	for( unsigned mipLevel = 0; mipLevel != mipCount; ++mipLevel )
	{
		void* data;
		unsigned srcPitch;
		if( FAILED( rtAL.Lock( mipLevel, nullptr, data, srcPitch, renderContext ) ) )
		{
			CCP_LOGWARN( "SharedCopyFaceFromRenderTarget: failed to lock renderTarget level %d", mipLevel );
			return false;
		}

			  uint8_t* dst = (uint8_t*)GetMipRawData( mipLevel, face );
		const uint8_t* src = (uint8_t*)data;

		const unsigned dstPitch  = GetMipPitch( mipLevel );

		dst += GetBytesPerPixel( m_format ) * offsetX + dstPitch * offsetY;
		src += GetBytesPerPixel( m_format ) * left + srcPitch * top;
		  
		if( alphaConvert )
		{
			for( unsigned j = 0; j != height; ++j, src += srcPitch, dst += dstPitch )
			{
				const uint8_t* in  = src;
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
		left    /= 2;
		top     /= 2;
	
		rtAL.Unlock( renderContext );
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

bool Tr2HostBitmap::CopyFromTextureRes ( TriTextureRes& res, Tr2RenderContext& renderContext )
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
		if( !res.GetTexture()->IsValid() || GetType() != res.GetTexture()->GetType() || !CheckForMatch( *res.GetTexture(), true, alphaConvert, "CopyFromTextureRes" ) )
		{
			return false;
		}
	}

	const uint32_t mipCount  = std::min( GetTrueMipCount(), res.GetTrueMipCount() );
	const uint32_t faceCount = GetType() == TEX_TYPE_CUBE ? 6 : 1;


	for( uint32_t face = CUBEMAP_FACE_FIRST; face != faceCount; ++face )
	{
		for( uint32_t mipLevel = 0; mipLevel != mipCount; ++mipLevel )
		{
			void* srcData = nullptr; 
			uint32_t srcPitch = 0;

			HRESULT hr = E_FAIL;

			if( res.m_wrappedRenderTarget )
			{
				hr = res.m_wrappedRenderTarget->GetRenderTarget().Lock( mipLevel, nullptr, srcData, srcPitch, renderContext );
			}
			else
			if( res.GetTexture() )
			{
				hr = res.GetTexture()->Lock( CubemapFace( face ), mipLevel, nullptr, srcData, srcPitch, LOCK_READONLY, renderContext );
			}
			if( FAILED( hr ) || srcData == nullptr )
			{
				CCP_LOGERR( "Tr2HostBitmap::CopyFromTextureRes, error locking surface" );
				return false;
			}


			uint8_t* dst = (uint8_t*)GetMipRawData( mipLevel, CubemapFace( face ) );
			const uint8_t* src = (uint8_t*)srcData;

			const uint32_t dstPitch  = GetMipPitch( mipLevel );

			if( alphaConvert )
			{
				const uint32_t width  = GetMipWidth ( mipLevel );
				const uint32_t height = GetMipHeight( mipLevel );
				for( uint32_t j = 0; j != height; ++j, src += srcPitch, dst += dstPitch )
				{
					const uint8_t* in  = src;
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
				res.m_wrappedRenderTarget->GetRenderTarget().Unlock( renderContext );
			}
			else if( res.GetTexture() )
			{
				res.GetTexture()->Unlock( renderContext );
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

#if DEPRECATED_ENABLED
template<typename MATRIX>
bool ApplyConvFilter(	
	Tr2HostBitmap*	dest,
	Tr2HostBitmap*	source,
	const MATRIX&	matrix,
	bool			tile
	)
{
	if( !source											||
		!dest											||
		!source->IsValid()								||
		!dest->IsValid()								||
		source->GetType() != TEX_TYPE_2D				||
		dest->GetType() != TEX_TYPE_2D					||
		//source->GetWidth() != dest->GetWidth()			||
		//source->GetHeight() != dest->GetHeight()		||
		source->GetFormat() != dest->GetFormat()		||
		GetBytesPerPixel( source->GetFormat() ) != 4	)
	{
		return false;
	}

	if( source->GetFormat() != PIXEL_FORMAT_B8G8R8A8_UNORM )
	{
		return false;
	}

	const float * const m_ = &matrix.m[0][0];

	const unsigned numCells = sizeof( matrix.m ) / sizeof( matrix.m[0][0] );
	const unsigned dim      = (unsigned)sqrtf( (float)numCells );
	

	const int w = std::min<int>( source->GetWidth() , dest->GetWidth()  );
	const int h = std::min<int>( source->GetHeight(), dest->GetHeight() );

	const int dx = -(int)dim / 2;
	const int dy = -(int)dim / 2;

	auto ToF_ = [&](const char* rgba){ return (*(const uint8_t*)rgba) / 255.0f; };
	
	for( int y = 0; y != h; ++y )
	{
		for( int x = 0; x != w; ++x )
		{
			float r = 0, g = 0, b = 0, a = 0;
			float weight = 0;

			for( unsigned j = 0; j != dim; ++j )
			{
				for( unsigned i = 0; i != dim; ++i )
				{
					int px = x + i + dx;
					int py = y + j + dy;
					if( tile )
					{
						if( px < 0 )
						{
							px += w;
						}
						else
						{
							px %= w;
						}
						if( py < 0 )
						{
							py += h;
						}
						else
						{
							py %= h;
						}
					}
					else
					if( px < 0 || px >= w || py < 0 || py >= h )
					{
						continue;
					}

					float w = m_[j * dim + i];
					if( !w )
					{
						continue;
					}

					const char* src = source->GetRawData( unsigned(px), unsigned(py) );					

					weight += w;

					r += ToF_(src+0) * w;
					g += ToF_(src+1) * w;
					b += ToF_(src+2) * w;
					a += ToF_(src+3) * w;
				}
			}

			if( weight != 0 )
			{
				weight = 1.0f / weight;
				r *= weight;
				g *= weight;
				b *= weight;
				a *= weight;
			}

			char* dst = dest  ->GetRawData( unsigned(x), unsigned(y) );

			auto ToC_ = [&](char* rgba, float f){ *rgba = (char)(255.0f * std::max( 0.0f, std::min (1.0f, f ) ) ); };

			ToC_(dst+0, r);
			ToC_(dst+1, g);
			ToC_(dst+2, b);
			ToC_(dst+3, a);
		}
	}

	return true;
}

bool Tr2HostBitmap::ApplyConvFilter( Tr2HostBitmap* source, const D3DXCONVOLUTIONMATRIX3*  mat, bool tile )
{
	return mat && m_mipCount == 1 ? ::ApplyConvFilter( this, source, *mat, tile ) : false;
}

bool Tr2HostBitmap::ApplyConvFilter( Tr2HostBitmap* source, const D3DXCONVOLUTIONMATRIX5*  mat, bool tile )
{
	return mat && m_mipCount == 1? ::ApplyConvFilter( this, source, *mat, tile ) : false;
}

bool Tr2HostBitmap::ApplyConvFilter( Tr2HostBitmap* source, const D3DXCONVOLUTIONMATRIX7*  mat, bool tile )
{
	return mat && m_mipCount == 1? ::ApplyConvFilter( this, source, *mat, tile ) : false;
}
#endif

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

	const unsigned bytesPerPixel  = GetBytesPerPixel( GetFormat() );
	const unsigned bytesPerMargin = bytesPerPixel * margin;


	const unsigned width  = GetWidth()  - 2 * margin;
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

bool Tr2HostBitmap::Save( const wchar_t* path, std::shared_ptr<Tr2ImageHandler> imageHandler )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !IsValid() )
	{
		CCP_LOGWARN( "Tr2HostBitmap::Save not a valid bitmap" );
		return false;
	}

	if( !imageHandler )
	{
		imageHandler.reset( CreateImageHandler( path ) );

		if( !imageHandler )
		{
			CCP_LOGERR( "Unsupported extension for saving (%S)", path );
			return false;
		}
	}

	Be::Clsid resFileClsid( "blue", "ResFile" );
	IResFilePtr stream( resFileClsid );
	if( !( stream->FileExistsW( path ) ? stream->OpenW( path, false ) : stream->CreateW( path ) ) )
	{
		CCP_LOGWARN( "Tr2HostBitmap::Save failed to open Blue stream (%S)", path );
		return false;
	}
	ON_BLOCK_EXIT( [&]{ stream->Close(); } );

	return imageHandler->Save( *this, stream );
}

bool Tr2HostBitmap::SaveAsync( const wchar_t* path, std::shared_ptr<Tr2ImageHandler> imageHandler )
{
	m_asyncSaveImage = imageHandler;
	return StartAsyncSave( path );
}

bool Tr2HostBitmap::DoPrepareAsyncSave()
{
	return true;
}

bool Tr2HostBitmap::DoExecuteAsyncSave()
{
	return Save( m_saveFilename.c_str(), m_asyncSaveImage );
}

void Tr2HostBitmap::DoCleanupAsyncSave()
{
	m_asyncSaveImage.reset();
}

bool Tr2HostBitmap::CreateFromFile( const std::wstring& file )
{
	CleanupAsyncSave( false );
	Destroy();

	if( file.empty() )
	{
		return false;
	}

	std::unique_ptr<Tr2ImageHandler> imageHandler( CreateImageHandler( file ) );	// only need extension, so 'file' is fine
	if( !imageHandler )
	{
		CCP_LOGERR( "Unsupported extension for CreateFromFile (%S)", file.c_str() );
		return false;
	}

	IBlueStreamPtr stream;
	if( !BePaths->GetStreamFromPathW( file.c_str(), &stream ) )
	{
		CCP_LOGERR( "Error opening file (%S)", file.c_str() );
		return false;
	}

	if( !imageHandler->ReadHeader( stream ) || !imageHandler->ReadImage( stream ) )
	{
		CCP_LOGERR( "Error reading file (%S)", file.c_str() );
		return false;
	}

	unsigned faces = 1;

	if( imageHandler->IsCubeTexture() )
	{
		if( !CreateCube( imageHandler->GetWidth(), imageHandler->GetMipLevelCount(), imageHandler->GetFormat() ) )
		{
			CCP_LOGERR( "Error creating hostBitmap" );
			return false;
		}
		m_type = TEX_TYPE_CUBE;
		faces = 6;
	}
	else if( imageHandler->IsVolumeTexture() )
	{
		if( !CreateVolume( imageHandler->GetWidth(), imageHandler->GetHeight(), imageHandler->GetDepth(), imageHandler->GetMipLevelCount(), imageHandler->GetFormat() ) )
		{
			CCP_LOGERR( "Error creating hostBitmap" );
			return false;
		}
		m_type = TEX_TYPE_3D;
		m_volumeDepth = imageHandler->GetDepth();
	}
	else
	{
		if( !Create( imageHandler->GetWidth(), imageHandler->GetHeight(), imageHandler->GetMipLevelCount(), imageHandler->GetFormat() ) )
		{
			CCP_LOGERR( "Error creating hostBitmap" );
			return false;
		}
		m_type = TEX_TYPE_2D;
	}

	if( imageHandler->GetTotalDataSize() != m_data.size() )
	{
		CCP_LOGERR( "Unsupported format (mipmaps/pixelFormat)" );
		return false;
	}


	m_width		= imageHandler->GetWidth();
	m_height	= imageHandler->GetHeight();
	m_format	= imageHandler->GetFormat();
	m_mipCount	= imageHandler->GetMipLevelCount();

	size_t ofs = 0;
	for( unsigned face = 0; face < faces; ++face )
	{
		for( unsigned mip = 0; mip != m_mipCount; ++mip )
		{
			CCP_ASSERT( ofs + imageHandler->GetMipLevelSize( mip ) <= m_data.size() );

			memcpy( m_data.get() + ofs, imageHandler->GetMipBytes( mip, face ), imageHandler->GetMipLevelSize( mip ) );
			ofs += imageHandler->GetMipLevelSize( mip );
		}
	}
	CCP_ASSERT( ofs == m_data.size() );

	return true;
}

bool Tr2HostBitmap::Compress( unsigned compressionFormat, unsigned qualityLevel, TriTextureRes* output )
{
#ifdef __ORBIS__
	// TODO: Enable for PS4
	return false;
#else
	if( !output || !IsValid() || GetType() != TEX_TYPE_2D || compressionFormat >= TR2DXT_COMPRESS_COUNT )
	{
		return false;
	}

	Tr2TextureAL texture;

	static const PixelFormat format[ TR2DXT_COMPRESS_COUNT ] = 
	{
		PIXEL_FORMAT_BC1_UNORM,		// TR2DXT_COMPRESS_RT_DXT1			= 0,
		PIXEL_FORMAT_BC3_UNORM,		// TR2DXT_COMPRESS_RT_DXT5			= 1,
		PIXEL_FORMAT_BC3_UNORM,		// TR2DXT_COMPRESS_RT_DXT5N			= 2,
		PIXEL_FORMAT_BC3_UNORM,		// TR2DXT_COMPRESS_RT_YCOCGDXT5		= 3,
		PIXEL_FORMAT_BC3_UNORM,		// TR2DXT_COMPRESS_RT_3DC			= 4,
		PIXEL_FORMAT_BC1_UNORM,		// TR2DXT_COMPRESS_SQUISH_DXT1		= 5,
		PIXEL_FORMAT_BC1_UNORM,		// TR2DXT_COMPRESS_SQUISH_DXT3		= 6,
		PIXEL_FORMAT_BC3_UNORM,		// TR2DXT_COMPRESS_SQUISH_DXT5		= 7,
	};

	{
		unsigned pitch = ( m_width + 3 ) / 4 * Tr2RenderContextEnum::GetBlockByteSize( format[ compressionFormat ] );
		CcpMallocBuffer destination( "Tr2HostBitmap::Compress", pitch * ( m_height + 3 ) / 4 );

		Tr2DxtCompressControl* control = CCP_NEW( "TriDevice::CompressSurface/control" ) Tr2DxtCompressControl;
		ON_BLOCK_EXIT( [&]{ CCP_DELETE( control ); } );

		if( !Tr2DxtCompressSurfaceAsync(	(Tr2DxtCompressionFormat)compressionFormat, 
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

		Tr2SubresourceData initialData;
		initialData.m_sysMem = destination.get();
		initialData.m_height = m_height;
		initialData.m_sysMemPitch = pitch;
		initialData.m_sysMemSlicePitch = uint32_t( destination.size() );

		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			CR_RETURN_VAL( texture.Create2D(	m_width, 
												m_height, 
												1, 
												format[ compressionFormat ], 
												USAGE_IMMUTABLE, 
												&initialData, 
												renderContext )
						, false );
		}
	}

	output->SetTexture( texture );
	return true;
#endif
}

bool Tr2HostBitmap::SetPixel( int x, int y, const void *data )
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
	
	char *dest = m_data.get() + y * m_width * 4 + x * 4;
	memcpy( dest, data, 4 );
	return true;
}

bool Tr2HostBitmap::GetPixel( int x, int y, void *data )
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

	char *src = m_data.get() +  y * m_width * 4 + x * 4;
	memcpy( data, src, 4 );
	return true;
}

#if BLUE_WITH_PYTHON
bool Tr2HostBitmap::SetPixelPy( PyObject *tuple )
{
	static_assert( sizeof( int ) == 4, "Assuming that RGBA8 fits in int" );
	int x, y, val;
	if(!PyArg_ParseTuple( tuple, "iii", &x, &y, &val ) )
	{
		return false;
	}
	
	return SetPixel( x, y, &val );
}

PyObject * Tr2HostBitmap::GetPixelPy( PyObject *tuple )
{
	int x, y, val=0;
	if( !PyArg_ParseTuple( tuple, "ii", &x, &y ) ||
		!GetPixel( x, y, &val ) )
	{
		Py_RETURN_NONE;
	}
	return PyInt_FromLong(val);
}

PyObject* Tr2HostBitmap::PySetPixel( PyObject* args )
{
	if( PyTuple_Size( args ) != 1 )
	{
		SetPixelPy( args );
	} 
	else 
	{
		PyObject *list;
		PyArg_ParseTuple( args, "O", &list );
		if( !PySequence_Check( list ) )
		{
			PyErr_SetString(PyExc_TypeError, "Expected sequence of tuples");
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
		return GetPixelPy(args);
	} 
	else 
	{
		PyObject *list;
		PyArg_ParseTuple( args, "O", &list );
		if( !PySequence_Check( list ) )
		{
			PyErr_SetString(PyExc_TypeError, "Expected sequence of tuples");
			return nullptr;
		}
		Py_ssize_t len = PySequence_Size( list );
		BluePyTuple res( len );
		for( Py_ssize_t i = 0; i< len; i++ )
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

PyObject *Tr2HostBitmap::PySetPixels( PyObject *args )
{
	// legacy method for cramming some pixels into a simple RGBA8 blob from python
	if( !IsValid() || GetBytesPerPixel( GetFormat() ) != 4 || GetType() != TEX_TYPE_2D || GetTrueMipCount() != 1 )
	{
		Py_RETURN_NONE;
	}

    int background    = 0;
    PyObject *pixelsO = 0;
	PyObject *discard = 0;
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
    
    Py_ssize_t size = PySequence_Fast_GET_SIZE( ( PyObject* )fast );
    for( Py_ssize_t i = 0; i < size; i++ ) 
	{
        PyObject *tuple = PySequence_Fast_GET_ITEM( ( PyObject* )fast, i );
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
#endif
