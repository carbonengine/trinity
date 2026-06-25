// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2FontManager.h"
#include "Tr2SBit.h"

BLUE_DEFINE_ABSTRACT( Tr2FontManager );

const char* TR2_FONT_FALLBACK = "res:/ui/fonts/arialuni.ttf";

#if BLUE_WITH_PYTHON

#if PY_MAJOR_VERSION == 2
static bool GetBuf( void** buffer, Py_ssize_t* bufLen, PyObject* arrayO )
{
	//Get destination buffer interface
	PyBufferProcs* bp = arrayO->ob_type->tp_as_buffer;
	if( !bp || !bp->bf_getwritebuffer )
	{
		PyErr_SetString( PyExc_TypeError, "destination must support buffer interface" );
		return false;
	}
	*bufLen = ( bp->bf_getwritebuffer )( arrayO, 0, buffer );
	return *bufLen != -1;
}
#endif

PyObject* PyClearBuffer( PyObject* self, PyObject* args )
{
	PyObject* arrayO;
	int width, height, pitch;
	int col = 0;
	if( !PyArg_ParseTuple( args, "Oiii|i", &arrayO, &width, &height, &pitch, &col ) )
		return 0;

	void* destPtr = nullptr;
	Py_ssize_t destLen = 0;
#if PY_MAJOR_VERSION == 2
	if( !GetBuf( &destPtr, &destLen, arrayO ) )
	{
		return nullptr;
	}
#else
	Py_buffer buf;
	if( PyObject_GetBuffer( arrayO, &buf, PyBUF_SIMPLE ) == -1 )
	{
		return nullptr;
	}
	ON_BLOCK_EXIT( [&] { PyBuffer_Release( &buf ); } );
	destPtr = buf.buf;
	destLen = buf.len;
#endif
	if( width * 4 > pitch )
	{
		PyErr_SetString( PyExc_RuntimeError, "width larger than pitch!" );
		return nullptr;
	}

	if( height * pitch > destLen )
	{
		PyErr_SetString( PyExc_RuntimeError, "destination too short!" );
		return nullptr;
	}

	for( int i = 0; i < height; i++ )
	{
		for( int j = 0; j < width; j++ )
		{
			( (int*)( (char*)destPtr + i * pitch ) )[j] = col;
		}
	}

	Py_INCREF( Py_None );
	return Py_None;
}
#endif


const Be::ClassInfo* Tr2FontManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2FontManager, "Tr2FontManager is used for font management" )
		MAP_ATTRIBUTE(
			"loadFlag",
			m_loadflag,
			"FreeType loadflag to control loading/rendering.",
			Be::READWRITE )

		MAP_METHOD_AND_WRAP(
			"LookupGlyphIndex",
			LookupFaceIDAndGlyphIndex,
			"fgi = LookupGlyphIndex(font, charCode)"
			"\nLooks up a glyph index from the given font."
			"\n:param font: res path for the font"
			"\n:param charCode:"
			"\n\n:returns:"
			"\n  A tuple, (faceID, glyphIndex)"
			"\n  These values are used by LookupKerningXP and LookupSBit" )

		MAP_METHOD_AND_WRAP(
			"LookupKerningXP",
			PyLookupKerningXP,
			"kern = LookupKerningXP(faceID, glyphIndex1, glyphIndex2)"
			"\nLooks up kerning values for the given face ID and char code pair."
			"\n:param faceID: as returned from LookupGlyphIndex"
			"\n:param glyphIndex1: as returned from LookupGlyphIndex"
			"\n:param glyphIndex2: as returned from LookupGlyphIndex"
			"\n\n:returns:"
			"\n  Kerning value in pixels as an integer." )

		MAP_METHOD_AND_WRAP(
			"LookupSBit",
			LookupSBit,
			"sbit = LookupSBit(faceID, width, height, glyphIndex)"
			"\nLook up an SBit (small bitmap) for the given faceID, dimensions and glyph index."
			"\n:param faceID: as returned from LookupGlyphIndex"
			"\n:param width: "
			"\n:param height: "
			"\n:param glyphIndex: as returned from LookupGlyphIndex"
			"\n\n:returns:"
			"\n  An SBit structure that can be used to render the font" )

		MAP_METHOD_AND_WRAP(
			"LookupMetrics",
			LookupMetricsFromScript,
			"metrics = LookupMetrics(faceID, width, height)"
			"\nLooks up metrics for the given faceID and dimensions."
			"\n:param faceID: as returned from LookupGlyphIndex"
			"\n:param width: "
			"\n:param height: "
			"\n\n:returns:"
			"\n  A tuple, (ascender, descender)" )

		MAP_METHOD(
			"ClearBuffer",
			PyClearBuffer,
			"ClearBuffer(buffer, width, height, pitch)"
			"\nClears a rectangular buffer to 0 values."
			"\n:param buffer:"
			"\n:type buffer: buffer"
			"\n:param width:"
			"\n:type width: int"
			"\n:param height:"
			"\n:type height: int"
			"\n:param pitch:"
			"\n:type pitch: Optional[int]"
			"\n:rtype: None" )

		MAP_ATTRIBUTE(
			"glyphCacheBudget",
			m_glyphCacheBudget,
			"Allowed budget for glyph cache, in bytes.",
			Be::READWRITE )

		MAP_PROPERTY_READONLY(
			"numGlyphsInUse",
			GetNumGlyphsInUse,
			"Returns the number of glyphs currently in use." )

		MAP_PROPERTY_READONLY(
			"numGlyphsCached",
			GetNumGlyphsCached,
			"Returns the number of glyphs cached by the font manager." )

		MAP_METHOD_AND_WRAP(
			"ClearCachedGlyphs",
			ClearCachedGlyphs,
			"Clears any cached glyphs." )

		MAP_METHOD_AND_WRAP(
			"TrimGlyphCache",
			TrimGlyphCache,
			"TrimGlyphCache(size)"
			"\nTrims the glyph cache to size. Discards cached glyphs in LRU order."
			"\n:param size: the desired cache size" )

		MAP_ATTRIBUTE(
			"totalGlyphsCachedSize",
			m_totalGlyphsCachedSize,
			"Total size of all cached glyphs, in bytes.",
			Be::READ )

	EXPOSURE_END()
}
