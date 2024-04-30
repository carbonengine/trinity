////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#pragma once
#include "../Upscaling/ITr2Upscaling.h"

namespace Tr2Sharpening
{

	enum Technique
	{
		SHARPENING_TECHNIQUE_NONE,
		SHARPENING_TECHNIQUE_CAS
	};

}

BLUE_INTERFACE( ITr2Sharpening ) : public IRoot
{
	virtual void Setup( uint32_t renderWidth, uint32_t renderHeight ) = 0;
	virtual void Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures ) = 0;
};
