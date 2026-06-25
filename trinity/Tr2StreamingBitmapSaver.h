// Copyright © 2013 CCP ehf.

#pragma once
#ifndef Tr2StreamingBitmapSaver_H
#define Tr2StreamingBitmapSaver_H

BLUE_DECLARE( Tr2RenderTarget );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2StreamingBitmapSaver allows saving very big TGA images without allocating entire
//   image in memory.
// See Also:
//   Tr2HostBitmap
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2StreamingBitmapSaver ) :
	public IRoot
{
public:
	Tr2StreamingBitmapSaver( IRoot* lockobj = nullptr );
	~Tr2StreamingBitmapSaver();

	EXPOSE_TO_BLUE();

	ALResult StartSaving( const wchar_t* path, uint32_t width, uint32_t height, Tr2RenderContextEnum::PixelFormat pixelFormat );
	ALResult StartBatch( uint32_t rowsPerBatch );
	ALResult CopyFromRenderTargetRegion( Tr2RenderTarget * rt, int left, int top, int right, int bottom, int offsetX, int offsetY );
	ALResult FlushBatch();
	ALResult EndSaving();

	bool IsSaving() const;
	bool HasStartedBatch() const;

private:
	uint32_t m_width;
	uint32_t m_height;
	Tr2RenderContextEnum::PixelFormat m_format;
	uint32_t m_bytesPerPixel;
	uint32_t m_rowsPerBatch;
	// current Y offset of the batch in the image
	uint32_t m_currentOffset;
	IBlueStreamPtr m_output;
	CcpMallocBuffer m_data;
};

TYPEDEF_BLUECLASS( Tr2StreamingBitmapSaver );

#endif