////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#pragma once

#include "ITr2Upscaling.h" 
#include "Tr2UpscalingUtils.h"
#include "Shader/Tr2Effect.h"

BLUE_DECLARE( Tr2Effect );

BLUE_CLASS( Tr2Fsr1Upscaling ) : public ITr2Upscaling
{
public:
	EXPOSE_TO_BLUE();

	Tr2Fsr1Upscaling( IRoot* lockobj = NULL );
	~Tr2Fsr1Upscaling();

	bool IsApplicable() const;
	Tr2Upscaling::UpscalingType GetUpscalingType() const;
	const std::vector<Tr2Upscaling::Setting> GetAvailableSettings() const;

	void GetJitter( float& x, float& y );
	void GetJitterOffset( float& x, float& y );
	float GetMipLevelBias() const;
	void GetRenderSize(uint32_t& width, uint32_t& height) const;

	void ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight );
	void Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext );
	void Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation );

private:
	Tr2EffectPtr m_fidelityFXFsrEASUShader;
	Tr2EffectPtr m_fidelityFXFsrRCASShader;

	AMDUpscaling::FSRConstants m_easuConst;
	AMDUpscaling::FSRConstants m_rcasConst;

	uint32_t m_renderWidth;
	uint32_t m_renderHeight;	
	uint32_t m_displayWidth;
	uint32_t m_displayHeight;	
	float m_upscaling;
};
TYPEDEF_BLUECLASS( Tr2Fsr1Upscaling );
