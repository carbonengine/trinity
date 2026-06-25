// Copyright © 2013 CCP ehf.

#include "StdAfx.h"
#include "Tr2StreamingBitmapSaver.h"
#include "Tr2RenderTarget.h"
#include <Tr2TgaHandler.h>

// --------------------------------------------------------------------------------------
// Description:
//   Tr2StreamingBitmapSaver default constructor
// --------------------------------------------------------------------------------------
Tr2StreamingBitmapSaver::Tr2StreamingBitmapSaver( IRoot* /* lockobj */ ) :
	m_width( 0 ),
	m_height( 0 ),
	m_format( Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN ),
	m_bytesPerPixel( 0 ),
	m_rowsPerBatch( 0 ),
	m_currentOffset( 0 )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2StreamingBitmapSaver destructor
// --------------------------------------------------------------------------------------
Tr2StreamingBitmapSaver::~Tr2StreamingBitmapSaver()
{
	if( IsSaving() )
	{
		EndSaving();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Starts saving TGA image. Opens a file and writes out a header.
// Arguments:
//   path - Res or physical path to output TGA image
//   width - Width of the image
//   height - Height of the image
//   pixelFormat - Image pixel format
// Return Value:
//   AL result of operation
// --------------------------------------------------------------------------------------
ALResult Tr2StreamingBitmapSaver::StartSaving(
	const wchar_t* path,
	uint32_t width,
	uint32_t height,
	Tr2RenderContextEnum::PixelFormat pixelFormat )
{
	if( IsSaving() )
	{
		CR_RETURN_HR( EndSaving() );
	}

	if( !ImageIO::Tga::IsSaveSupported( m_format ) )
	{
		CCP_LOGWARN(
			"Tr2StreamingBitmapSaver::StartSaving unsupported image format (%i)",
			pixelFormat );
		return E_INVALIDARG;
	}

	if( !width || !height )
	{
		CCP_LOGWARN(
			"Tr2StreamingBitmapSaver::StartSaving invalid image dimensions (%u x %u)",
			width,
			height );
		return E_INVALIDARG;
	}

	Be::Clsid resFileClsid( "blue", "ResFile" );
	IResFilePtr stream( resFileClsid );
	if( !( stream->FileExistsW( path ) ? stream->OpenW( path, false ) : stream->CreateW( path ) ) )
	{
		CCP_LOGWARN(
			"Tr2StreamingBitmapSaver::StartSaving failed to open Blue stream (%ls)",
			path );
		return E_FAIL;
	}

	if( !ImageIO::Tga::SaveHeader( width, height, pixelFormat, *stream ) )
	{
		CCP_LOGWARN(
			"Tr2StreamingBitmapSaver::StartSaving failed to write TGA header (%ls)",
			path );
		return E_FAIL;
	}

	m_output = stream;

	m_width = width;
	m_height = height;
	m_format = pixelFormat;
	m_bytesPerPixel = Tr2RenderContextEnum::GetBytesPerPixel( m_format );
	m_currentOffset = m_height;

	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Starts next batch (slice) of an image. Batch is a region of image spanning the
//   entire image width with a set height. The Tr2StreamingBitmapSaver object allocates
//   data to store contents of the slice in memory. This operation shifts saving window
//   inside the image upwards.
// Arguments:
//   rowsPerBatch - Height of the batch
// Return Value:
//   AL result of operation
// --------------------------------------------------------------------------------------
ALResult Tr2StreamingBitmapSaver::StartBatch( uint32_t rowsPerBatch )
{
	if( !IsSaving() )
	{
		CCP_LOGWARN( "Tr2StreamingBitmapSaver::StartBatch need to call StartSaving before StartBatch" );
		return E_INVALIDARG;
	}
	if( HasStartedBatch() )
	{
		FlushBatch();
	}
	if( !rowsPerBatch || rowsPerBatch > m_currentOffset )
	{
		return E_INVALIDARG;
	}

	m_rowsPerBatch = rowsPerBatch;
	m_currentOffset -= m_rowsPerBatch;
	size_t size = m_rowsPerBatch * m_width * m_bytesPerPixel;

	if( size != m_data.size() )
	{
		if( m_data.get() )
		{
			m_data.resize( "Tr2StreamingBitmapSaver::m_data", size );
		}
		else
		{
			m_data = CcpMallocBuffer( "Tr2StreamingBitmapSaver::m_data", size );
		}
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Copies region of a readable render target into current window of the image.
// Arguments:
//   rt - A render target
//   left, top, right, bottom - Rectangle inside render target to be copied
//   offsetX, offsetY - offset in the image to copy render target to
// Return Value:
//   AL result of operation
// --------------------------------------------------------------------------------------
ALResult Tr2StreamingBitmapSaver::CopyFromRenderTargetRegion(
	Tr2RenderTarget* rt,
	int left,
	int top,
	int right,
	int bottom,
	int offsetX,
	int offsetY )
{
	if( !rt || !rt->IsValid() || !m_output || !m_rowsPerBatch )
	{
		return E_INVALIDARG;
	}
	if( rt->GetRenderTarget().GetFormat() != m_format )
	{
		return E_INVALIDARG;
	}

	left = max( 0, left );
	top = max( 0, top );
	right = min( int( rt->GetRenderTarget().GetWidth() ), right );
	bottom = min( int( rt->GetRenderTarget().GetHeight() ), bottom );

	if( offsetX < 0 )
	{
		left = max( left, -offsetX );
		right = min( right, int( rt->GetRenderTarget().GetWidth() ) + offsetX );
	}
	if( offsetY < int( m_currentOffset ) )
	{
		top = max( top, int( m_currentOffset ) - offsetY );
		bottom = min( bottom, int( rt->GetRenderTarget().GetHeight() ) - ( int( m_currentOffset ) - offsetY ) );
	}
	offsetX += left;
	offsetY += top;
	offsetY -= m_currentOffset;

	if( left >= right ||
		top >= bottom ||
		int( m_width ) < offsetX ||
		int( m_rowsPerBatch ) < offsetY )
	{
		// source rectangle is empty
		return S_OK;
	}

	unsigned width = min( unsigned( right - left ), m_width - offsetX );
	unsigned height = min( unsigned( bottom - top ), m_rowsPerBatch - offsetY );

	USE_MAIN_THREAD_RENDER_CONTEXT();
	const void* data;
	unsigned srcPitch;
	HRESULT hr;

	if( FAILED( hr = rt->GetRenderTarget().MapForReading( Tr2TextureSubresource( 0 ), data, srcPitch, renderContext ) ) )
	{
		CCP_LOGWARN( "Tr2StreamingBitmapSaver::CopyFromRenderTargetRegion: failed to lock renderTarget" );
		return hr;
	}

	uint8_t* dst = reinterpret_cast<uint8_t*>( m_data.get() );
	const uint8_t* src = (uint8_t*)data;

	const unsigned dstPitch = m_bytesPerPixel * m_width;

	dst += m_bytesPerPixel * offsetX + dstPitch * offsetY;
	src += m_bytesPerPixel * left + srcPitch * top;

	const unsigned pitch = width * m_bytesPerPixel;
	for( unsigned j = 0; j != height; ++j, src += srcPitch, dst += dstPitch )
	{
		memcpy( dst, src, pitch );
	}

	rt->GetRenderTarget().UnmapForReading( renderContext );

	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Flushes current batch into the output stream.
// Return Value:
//   AL result of operation
// --------------------------------------------------------------------------------------
ALResult Tr2StreamingBitmapSaver::FlushBatch()
{
	if( !HasStartedBatch() )
	{
		return E_INVALIDARG;
	}

	if( !ImageIO::Tga::SaveRows( m_width, m_rowsPerBatch, m_format, m_data.get(), *m_output ) )
	{
		return E_FAIL;
	}
	m_rowsPerBatch = 0;

	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Finishes saving, closes the stream.
// Return Value:
//   AL result of operation
// --------------------------------------------------------------------------------------
ALResult Tr2StreamingBitmapSaver::EndSaving()
{
	if( !IsSaving() )
	{
		return E_INVALIDARG;
	}
	m_output.Unlock();
	m_data.clear();
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the image is currently saving (i.e. StartSaving was called).
// Return Value:
//   true If image is saving
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2StreamingBitmapSaver::IsSaving() const
{
	return m_output != nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if there is a valid batch (i.e. StartBatch was called).
// Return Value:
//   true If there is a valid batch
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2StreamingBitmapSaver::HasStartedBatch() const
{
	return m_rowsPerBatch != 0;
}
