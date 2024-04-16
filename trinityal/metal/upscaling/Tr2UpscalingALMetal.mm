////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once
#include "StdAfx.h"
#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2UpscalingALMetal.h"

namespace TrinityALImpl
{
	Tr2UpscalingTechniqueAL* CreateUpscalingTechnique( Tr2Upscaling::Technique technique, Tr2Upscaling::Setting setting, bool frameGeneration )
	{
		return nullptr;
	}
}


#endif