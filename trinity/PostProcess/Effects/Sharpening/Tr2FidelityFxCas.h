////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#pragma once

#include "ITr2Sharpening.h" 
#include "../Upscaling/Tr2UpscalingUtils.h"
#include "Shader/Tr2Effect.h"

BLUE_DECLARE( Tr2Effect );

BLUE_CLASS( Tr2FidelityFxCas ) : public ITr2Sharpening
{
public:
	EXPOSE_TO_BLUE();

	Tr2FidelityFxCas( IRoot* lockobj = NULL );
	~Tr2FidelityFxCas();

	void Setup( uint32_t renderWidth, uint32_t renderHeight );
	void Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures );

private:
	Tr2EffectPtr m_shader;
	AMDUpscaling::CASConstants m_casConst;

	uint32_t m_dispatchX;
	uint32_t m_dispatchY;
};

TYPEDEF_BLUECLASS( Tr2FidelityFxCas );
