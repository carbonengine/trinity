////////////////////////////////////////////////////////////////////////////////
//
// Created:		May 2011
// Copyright:	CCP 2011
//

#include "StdAfx.h"
#include "Tr2SBit.h"


// cppcheck-suppress uninitMemberVar
Tr2SBitWrapper::Tr2SBitWrapper( IRoot* lockobj /*= NULL */ ) :
	manager( NULL )
{
	// Note that variables other than 'manager' are left uninitialized - caller
	// is responsible for them.
}

Tr2SBitWrapper::~Tr2SBitWrapper()
{
	if( manager )
	{
		FTC_Node_Unref( node, manager );
	}
}

#if BLUE_WITH_PYTHON
void Tr2SBitWrapper::ToBuffer( PyObject* dest, int width, int height, int pitch, int x, int y, uint32_t color )
{
    Py_buffer buf;
    if(PyObject_GetBuffer(dest, &buf, PyBUF_SIMPLE) == -1)
    {
        return;
    }

	SBit_To_RGBABuffer( buf.buf, width, height, pitch, sbit, x, y, color );
    PyBuffer_Release(&buf);
}

void Tr2SBitWrapper::ToBufferWithUnderline( PyObject* dest, int width, int height, int pitch, int x, int y, uint32_t color, int extraSpace )
{
    Py_buffer buf;
    if(PyObject_GetBuffer(dest, &buf, PyBUF_SIMPLE) == -1)
    {
        return;
    }

	FT_Face face = g_fontManager->LookupFace( faceId );

	short underlinePosition = face->underline_position >> 6;
	short underlineThickness = face->underline_thickness >> 6;
	if( underlineThickness < 1 )
	{
		underlineThickness = 1;
	}

	SBit_To_RGBABuffer(
		buf.buf, width, height, pitch,
		sbit, x, y, color );
	Underline_To_RGBABuffer( 
		buf.buf, width, height, pitch,
		x, y + underlinePosition, sbit->xadvance + extraSpace, underlineThickness, color );
    PyBuffer_Release(&buf);
}
#endif
