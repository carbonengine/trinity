////////////////////////////////////////////////////////////////////////////////
//
// Created:		May 2011
// Copyright:	CCP 2011
//

#include "StdAfx.h"
#include "Tr2SBit.h"

BLUE_DEFINE( Tr2SBitWrapper );

const Be::ClassInfo* Tr2SBitWrapper::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SBitWrapper, "" )
		MAP_INTERFACE( Tr2SBitWrapper )

		MAP_ATTRIBUTE
		(
			"x",
			x,
			"x position of this glyph",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"y",
			y,
			"y position of this glyph",
			Be::READ
		)

		MAP_PROPERTY_READONLY
		(
			"xOffset",
			GetXOffset,
			"x offset of bitmap for this glyph"
		)

		MAP_PROPERTY_READONLY
		(
			"yOffset",
			GetYOffset,
			"y offset of bitmap for this glyph"
		)

		MAP_PROPERTY_READONLY
		(
			"width",
			GetWidth,
			"Width of the bitmap for this glyph"
		)

		MAP_PROPERTY_READONLY
		(
			"height",
			GetHeight,
			"Height of the bitmap for this glyph"
		)

		MAP_PROPERTY_READONLY
		(
			"xadvance",
			GetXAdvance,
			"xadvance indicates how much to advance the cursor in the x direction"
			"\nwhen rendering this glyph horizontally."
		)

		MAP_PROPERTY_READONLY
		(
			"yadvance",
			GetYAdvance,
			"yadvance indicates how much to advance the cursor in the y direction"
			"\nwhen rendering this glyph vertically."
		)

#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP
		(
			"ToBuffer",
			ToBuffer,
			"ToBuffer(dest, width, height, pitch, x, y, color)"
			"\nCopies the glyph bitmap to the destination buffer."
			"\n:param dest: the destination buffer"
			"\n:param width: width of the destination buffer"
			"\n:param height: height of the destination buffer"
			"\n:param pitch: pitch of the destination buffer"
			"\n:param x: where to draw the bitmap within the destination buffer"
			"\n:param y: where to draw the bitmap within the destination buffer"
			"\n:param color: color to apply to the bitmap"
		)

		MAP_METHOD_AND_WRAP
		(
			"ToBufferWithUnderline",
			ToBufferWithUnderline,
			"ToBufferWithUnderline(dest, width, height, pitch, x, y, color)"
			"\nCopies the glyph bitmap to the destination buffer, with an added underline."
			"\n:param dest: the destination buffer"
			"\n:param width: width of the destination buffer"
			"\n:param height: height of the destination buffer"
			"\n:param pitch: pitch of the destination buffer"
			"\n:param x: where to draw the bitmap within the destination buffer"
			"\n:param y: where to draw the bitmap within the destination buffer"
			"\n:param color: color to apply to the bitmap"
			"\n:param letterspace: extra space after this letter"
		)
#endif


	EXPOSURE_END()

};