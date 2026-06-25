// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2ImageRes_h
#define Tr2ImageRes_h

class Tr2ImageHandler;
BLUE_DECLARE( Tr2ImageRes );

// Tr2ImageRes is a resource class, that loads image files asynchronously from
// png, dds or sdd files. Use a "raw" extra extension with the resource manager
// GetResource call to get a Tr2ImageRes rather than a TriTextureRes instance.
// Tr2ImageRes does not create any D3D resources - it is meant to be used when
// the raw image data is needed on the CPU.
BLUE_CLASS( Tr2ImageRes ) :
	public BlueAsyncRes,
	public ICacheable
{
public:
	EXPOSE_TO_BLUE();
	Tr2ImageRes( IRoot* lockobj = NULL );
	~Tr2ImageRes();

	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	//
	//////////////////////////////////////////////////////////////////////////

	// Returns the width of the image, or 0 if loading hasn't completed
	int GetWidth() const;

	// Returns the height of the image, or 0 if loading hasn't completed
	int GetHeight() const;

	// Checks the opacity of the given pixel
	bool IsPixelOpaque( int x, int y ) const;

	// Get the color value at the given pixel
	Color GetPixelColor( int x, int y ) const;

	const ImageIO::HostBitmap& GetBitmap() const;

protected:
	LoadingResult DoLoad();
	bool DoPrepare();

private:
	ImageIO::HostBitmap m_bitmap;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2ImageRes );
#endif //Tr2ImageRes_h
