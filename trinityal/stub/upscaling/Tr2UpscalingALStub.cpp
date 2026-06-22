// Copyright © 2024 CCP ehf.

#include "StdAfx.h"
#include "Tr2UpscalingALStub.h"

#if TRINITY_PLATFORM == TRINITY_STUB
namespace TrinityALImpl
{
Tr2UpscalingTechniqueAL* CreateUpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter )
{
	return nullptr;
}
}
#endif
