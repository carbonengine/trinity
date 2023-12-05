////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#pragma once

#include "ITr2Sharpening.h" 

BLUE_CLASS( Tr2NoopSharpening ) : public ITr2Sharpening
{
public:
	EXPOSE_TO_BLUE();

	Tr2NoopSharpening( IRoot* lockobj = NULL );
	~Tr2NoopSharpening();

	void Setup( uint32_t renderWidth, uint32_t renderHeight );
	void Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures );
};

TYPEDEF_BLUECLASS( Tr2NoopSharpening );
