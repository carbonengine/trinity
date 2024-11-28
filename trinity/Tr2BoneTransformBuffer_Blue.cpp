#include "StdAfx.h"
#include "Tr2BoneTransformBuffer.h"


BLUE_DEFINE_NONEXPOSED( Tr2BoneTransformBuffer );

const Be::ClassInfo* Tr2BoneTransformBuffer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2BoneTransformBuffer, "" )
		MAP_INTERFACE( Tr2BoneTransformBuffer )
	EXPOSURE_END()
}
