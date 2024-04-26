////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"
#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2UpscalingALMetal.h"
#include "Tr2MetalFxUpscaling.h"

namespace TrinityALImpl
{
	Tr2UpscalingTechniqueAL* CreateUpscalingTechnique( Tr2RenderContextAL &renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter )
	{
        Tr2UpscalingTechniqueAL* tech = nullptr;
        switch( technique ){
            case Tr2UpscalingAL::Technique::METALFX:
                tech = new Tr2MetalFxUpscalingTechnique( technique, setting, frameGeneration );
                break;
            default:
                return nullptr;
        }
        
        if( tech && !tech->IsAvailable( renderContext, adapter) )
        {
            delete tech;
            tech = nullptr;
        }
        return tech;
	}
}


#endif
