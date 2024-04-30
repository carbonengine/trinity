////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
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
