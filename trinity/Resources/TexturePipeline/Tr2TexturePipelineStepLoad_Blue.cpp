// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2TexturePipelineStepLoad.h"

BLUE_DEFINE( Tr2TexturePipelineStepLoad );

const Be::ClassInfo* Tr2TexturePipelineStepLoad::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TexturePipelineStepLoad, "" )
		MAP_INTERFACE( Tr2TexturePipelineStepLoad )
		MAP_INTERFACE( ITr2TexturePipelineStep )

		MAP_ATTRIBUTE( "path", m_path, "Res path for the input texture", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
