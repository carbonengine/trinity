// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2RuntimeGpuBuffer.h"


BLUE_DEFINE_NONEXPOSED( Tr2RuntimeGpuBuffer );

const Be::ClassInfo* Tr2RuntimeGpuBuffer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2RuntimeGpuBuffer, "" )
		MAP_INTERFACE( Tr2RuntimeGpuBuffer )
		MAP_INTERFACE( ITr2GpuBuffer )
	EXPOSURE_END()
}
