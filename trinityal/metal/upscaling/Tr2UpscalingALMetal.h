////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#if( TRINITY_PLATFORM == TRINITY_METAL )
#include "../../include/upscaling/Tr2UpscalingAL.h"

namespace TrinityALImpl
{
	static const std::vector<Tr2UpscalingAL::Technique> AVAILABLE_UPSCALING_TECHNIQUES = {
        Tr2UpscalingAL::Technique::METALFX
    };
    Tr2UpscalingTechniqueAL* CreateUpscalingTechnique( Tr2RenderContextAL &renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );

	Tr2UpscalingTechniqueAL* CreateUpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration );

}
#endif
