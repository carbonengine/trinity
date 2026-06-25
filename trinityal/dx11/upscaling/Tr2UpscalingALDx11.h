// Copyright © 2024 CCP ehf.

#pragma once
#include "../include/upscaling/Tr2UpscalingAL.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

namespace TrinityALImpl
{
constexpr Tr2UpscalingAL::Technique AVAILABLE_UPSCALING_TECHNIQUES[] = {
	Tr2UpscalingAL::FSR1
};

class Tr2UpscalingTechniqueDx11 : public Tr2UpscalingTechniqueAL
{
public:
	Tr2UpscalingTechniqueDx11( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	virtual ~Tr2UpscalingTechniqueDx11();

	virtual void AttachToDevice( CComPtr<ID3D11Device>& device );
};

Tr2UpscalingTechniqueDx11* CreateUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
}

#endif