// Copyright © 2024 CCP ehf.

#include "StdAfx.h"
#if ( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2UpscalingALMetal.h"
#include "Tr2MetalFxUpscaling.h"
#include "../../include/upscaling/Tr2Fsr1Upscaling.h"

namespace TrinityALImpl
{
Tr2UpscalingTechniqueAL* CreateUpscalingTechnique( Tr2RenderContextAL& renderContext,
												   Tr2UpscalingAL::Technique technique,
												   Tr2UpscalingAL::Setting setting,
												   bool frameGeneration,
												   uint32_t adapter )
{
	Tr2UpscalingTechniqueAL* tech = nullptr;
	switch( technique )
	{
	case Tr2UpscalingAL::Technique::METALFX:
		tech = new Tr2MetalFxUpscalingTechnique( renderContext, technique, setting, frameGeneration, adapter );
		break;
	case Tr2UpscalingAL::Technique::FSR1:
		tech = new Tr2Fsr1UpscalingTechnique( renderContext, technique, setting, frameGeneration, adapter );
		break;
	default:
		break;
	}

	if( tech && !tech->IsAvailable() )
	{
		delete tech;
		tech = nullptr;
	}
	return tech;
}
}


#endif
