#include "StdAfx.h"
#include "Tr2MorphTargetAnimationDataBuffer.h"


BLUE_DEFINE_NONEXPOSED( Tr2MorphTargetAnimationDataBuffer );

const Be::ClassInfo* Tr2MorphTargetAnimationDataBuffer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2MorphTargetAnimationDataBuffer, "" )
		MAP_INTERFACE( Tr2MorphTargetAnimationDataBuffer )
	EXPOSURE_END()
}
