////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#pragma once

#include "ITr2TexturePipelineStep.h"

BLUE_CLASS( Tr2TexturePipelineStepGenerateMips ): public ITr2TexturePipelineStep
{
public:
	EXPOSE_TO_BLUE();

	bool Execute( ImageIO::HostBitmap& bitmap, const std::unordered_map<std::wstring, const ImageIO::HostBitmap*>& inputs, const Tr2TexturePipelineParams& params ) const;
};

TYPEDEF_BLUECLASS( Tr2TexturePipelineStepGenerateMips );