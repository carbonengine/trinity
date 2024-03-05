#pragma once
#ifndef Tr2SBit_H
#define Tr2SBit_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_SIZES_H
#include FT_CACHE_H
#include "Tr2FontManager.h"

void SBit_To_RGBABuffer( void* destPtr, int dWidth, int dHeight, int dPitch, FTC_SBit sbit, int left, int top, int col );
void Underline_To_RGBABuffer( void* destPtr, int dWidth, int dHeight, int dPitch, int left, int top, int width, int thickness, int col );

BLUE_DECLARE( Tr2SBitWrapper );

class Tr2SBitWrapper : 
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2SBitWrapper( IRoot* lockobj = NULL );
	~Tr2SBitWrapper();

	int GetWidth() { return sbit->width; }
	int GetHeight() { return sbit->height; }
	int GetXOffset() { return sbit->left; }
	int GetYOffset() { return sbit->top; }
	int GetXAdvance() { return sbit->xadvance; }
	int GetYAdvance() { return sbit->yadvance; }

#if BLUE_WITH_PYTHON
	void ToBuffer( 
		PyObject* dest, int width, int height, int pitch, 
		int x, int y, uint32_t color );
	void ToBufferWithUnderline( 
		PyObject* dest, int width, int height, int pitch, 
		int x, int y, uint32_t color, int extraSpace );
#endif


	FaceID faceId;
	FTC_SBit sbit;
	FTC_Node node;
	FTC_Manager manager;
	int x;
	int y;
};

TYPEDEF_BLUECLASS( Tr2SBitWrapper );

#endif