////////////////////////////////////////////////////////////
//
//    Created:   April 2013
//    Copyright: CCP 2013
//

#include "StdAfx.h"
#include "Tr2StreamingBitmapSaver.h"
#include "Tr2RenderTarget.h"

BLUE_DEFINE_NONEXPOSED( Tr2StreamingBitmapSaver );


const Be::ClassInfo* Tr2StreamingBitmapSaver::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2StreamingBitmapSaver, "" )
		MAP_INTERFACE( Tr2StreamingBitmapSaver )

		MAP_METHOD_AND_WRAP( 
			"StartBatch", 
			StartBatch, 
			"Starts a batch of image data advancing Y offset in the resulting image\n"
			":param height: number of rows in the batch" );
		MAP_METHOD_AND_WRAP( 
			"CopyFromRenderTargetRegion", 
			CopyFromRenderTargetRegion, 
			"Copies data from a readable render target into the bitmap\n"
			":param source: Tr2RenderTarget of the same pixelFormat, and correct width/height\n"
			":param left: Source rectangle left offset in pixels\n"
			":param top: Source rectangle top offset in pixels\n"
			":param right: Source rectangle right offset in pixels\n"
			":param bottom: Source rectangle bottom offset in pixels\n"
			":param x: X offset in the bitmap in pixels\n"
			":param y: Y offset in the bitmap in pixels" );
		MAP_METHOD_AND_WRAP( 
			"FlushBatch", 
			FlushBatch, 
			"Flushes current batch to the disk" );
		MAP_METHOD_AND_WRAP( 
			"EndSaving", 
			EndSaving, 
			"Finishes saving an image" );
		MAP_ATTRIBUTE( "width", m_width, "Image width", Be::READ );
		MAP_ATTRIBUTE( "height", m_height, "Image height", Be::READ );
		MAP_ATTRIBUTE( "format", m_format, "Image pixel format (trinity.PIXEL_FORMAT)", Be::READ );
		MAP_ATTRIBUTE( "currentOffset", m_currentOffset, "Current Y offset of streaming window", Be::READ );
		MAP_PROPERTY_READONLY( "hasStartedBatch", HasStartedBatch, "Is between calls to StartBatch and FlushBatch" );
	EXPOSURE_END()
}

namespace
{

ALResult StartStreamingBitmap( const wchar_t* path, uint32_t width, uint32_t height, int/*Tr2RenderContextEnum::PixelFormat*/ pixelFormat, Tr2StreamingBitmapSaver** result )
{
	*result = nullptr;

	Tr2StreamingBitmapSaverPtr saver;
	saver.CreateInstance();
	CR_RETURN_HR( saver->StartSaving( path, width, height, Tr2RenderContextEnum::PixelFormat( pixelFormat ) ) );
	*result = saver.Detach();
	return S_OK;
}

}

MAP_FUNCTION_AND_WRAP( 
	"StartStreamingBitmap", 
	StartStreamingBitmap, 
	"Starts streaming large bitmap to disk.\n"
	":param path: path to TGA file to write\n"
	":param width: image width\n"
	":param height: image height\n"
	":param format: image pixel format (trinity.PIXEL_FORMAT)" );
