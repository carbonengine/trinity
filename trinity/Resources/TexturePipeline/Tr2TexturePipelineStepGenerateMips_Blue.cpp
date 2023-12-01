////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2TexturePipelineStepGenerateMips.h"

BLUE_DEFINE( Tr2TexturePipelineStepGenerateMips );

const Be::ClassInfo* Tr2TexturePipelineStepGenerateMips::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TexturePipelineStepGenerateMips, "" )
		MAP_INTERFACE( Tr2TexturePipelineStepGenerateMips )
		MAP_INTERFACE( ITr2TexturePipelineStep )
	EXPOSURE_END()
}
