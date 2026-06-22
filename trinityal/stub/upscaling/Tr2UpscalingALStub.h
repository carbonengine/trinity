// Copyright © 2024 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_STUB

#include "../include/upscaling/Tr2UpscalingAL.h"

namespace TrinityALImpl
{
static const std::vector<Tr2UpscalingAL::Technique> AVAILABLE_UPSCALING_TECHNIQUES = {};
Tr2UpscalingTechniqueAL* CreateUpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );

}
#endif