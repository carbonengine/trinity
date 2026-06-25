// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#if __APPLE__

#include "Tr2MouseCursor.h"
#import <Cocoa/Cocoa.h>


bool Tr2MouseCursor::Create_MacOS( const std::vector<Representation>& representations,
								   uint32_t width,
								   uint32_t height,
								   int hotspotX,
								   int hotspotY )
{
	auto image = [[NSImage alloc] initWithSize:NSMakeSize( width, height )];

	for( const auto& repr : representations )
	{
		auto bmp = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nil
														   pixelsWide:repr.width
														   pixelsHigh:repr.height
														bitsPerSample:8
													  samplesPerPixel:4
															 hasAlpha:YES
															 isPlanar:NO
													   colorSpaceName:NSCalibratedRGBColorSpace
														 bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
														  bytesPerRow:repr.width * 4
														 bitsPerPixel:32];
		memcpy( [bmp bitmapData], repr.pixelData.get(), repr.width * repr.height * 4 );
		[image addRepresentation:bmp];
	}

	m_cursor = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint( CGFloat( hotspotX ), CGFloat( hotspotY ) )];

	return true;
}

void Tr2MouseCursor::Apply_MacOS()
{
	[(NSCursor*)m_cursor set];
}

#endif
