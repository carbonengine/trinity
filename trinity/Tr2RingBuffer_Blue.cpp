// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2RingBuffer.h"


BLUE_DEFINE_NONEXPOSED( Tr2RingBuffer );

const Be::ClassInfo* Tr2RingBuffer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2RingBuffer, "" )
		MAP_INTERFACE( Tr2RingBuffer )
	EXPOSURE_END()
}
