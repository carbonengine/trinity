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
bool GetBuf(void **buffer, int *bufLen, PyObject *arrayO)
{
	//Get destination buffer interface
	PyBufferProcs *bp = arrayO->ob_type->tp_as_buffer;
	if (!bp || !bp->bf_getwritebuffer)
	{
		PyErr_SetString( PyExc_TypeError, "destination must support buffer interface" );
		return false;
	}
	*bufLen = (int)(bp->bf_getwritebuffer)(arrayO, 0, buffer);
	return *bufLen != -1;
}

void Tr2SBitWrapper::ToBuffer( PyObject* dest, int width, int height, int pitch, int x, int y, uint32_t color )
{
	void *destPtr;
	int destLen;
	if( !GetBuf(&destPtr, &destLen, dest) )
	{
		return;
	}

	SBit_To_RGBABuffer( destPtr, width, height, pitch, sbit, x, y, color );
}

void Tr2SBitWrapper::ToBufferWithUnderline( PyObject* dest, int width, int height, int pitch, int x, int y, uint32_t color, int extraSpace )
{
	void *destPtr;
	int destLen;
	if( !GetBuf(&destPtr, &destLen, dest) )
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
		destPtr, width, height, pitch, 
		sbit, x, y, color );
	Underline_To_RGBABuffer( 
		destPtr, width, height, pitch, 
		x, y + underlinePosition, sbit->xadvance + extraSpace, underlineThickness, color );
}
#endif

