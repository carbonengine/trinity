// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2TexturePipelineStepLimitSize.h"

BLUE_DEFINE( Tr2TexturePipelineStepLimitSize );

const Be::ClassInfo* Tr2TexturePipelineStepLimitSize::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TexturePipelineStepLimitSize, "" )
		MAP_INTERFACE( Tr2TexturePipelineStepLimitSize )
		MAP_INTERFACE( ITr2TexturePipelineStep )

		MAP_ATTRIBUTE( "maxWidth", m_maxWidth, "Maximum texture width (0 if don\'t care)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxHeight", m_maxHeight, "Maximum texture height (0 if don\'t care)", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
