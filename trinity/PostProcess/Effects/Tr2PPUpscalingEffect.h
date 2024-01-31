////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//

#pragma once

#include <memory>
#include "Tr2PPEffect.h"
#include "Upscaling/ITr2Upscaling.h"
#include "Sharpening/ITr2Sharpening.h"
#include "../Tr2PostProcessRenderInfo.h"

BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE_INTERFACE( ITr2Sharpening );
BLUE_DECLARE_INTERFACE( ITr2Upscaling );

BLUE_DECLARE( Tr2MetalFxSpatialUpscaling );
BLUE_DECLARE( Tr2MetalFxTemporalUpscaling );
BLUE_DECLARE( Tr2Fsr2Upscaling );
BLUE_DECLARE( Tr2Fsr1Upscaling );
BLUE_DECLARE( Tr2NoopUpscaling );

BLUE_DECLARE( Tr2FidelityFxCas );
BLUE_DECLARE( Tr2NoopSharpening );

BLUE_CLASS( Tr2PPUpscalingEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPUpscalingEffect( IRoot* lockobj = NULL );
	~Tr2PPUpscalingEffect();

	bool IsDirty() override;
	bool IsActive() override;

	float GetMipLodBias();
	float GetUpscalingAmount() const;
	Tr2Upscaling::UpscalingType GetUpscalingType();

	void GetRenderSize( uint32_t& width, uint32_t& height );
	void GetDisplaySize( uint32_t& width, uint32_t& height );

	void GetJitter( float& x, float& y );
	void GetJitterOffset( float& x, float& y );

	void Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext );

	void ApplySetting( Tr2Upscaling::Setting setting, uint32_t displayWidth, uint32_t displayHeight );
	void SetUpscalingTechnique( Tr2Upscaling::Technique technique, bool frameGeneration );
	void SetSharpeningTechnique( Tr2Sharpening::Technique technique );

	bool IsTemporal() const;
	bool HasUpscaling() const;
	bool HasSharpening() const;

	void Render( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation );
	void SetHudlessResource( Tr2TextureAL* hudlessResource, Tr2RenderContext& renderContext );

	bool NeedsExposureTexture() const;
	bool NeedsReactiveTexture() const;

private:
	ITr2UpscalingPtr m_upscalingEffect;
	ITr2SharpeningPtr m_sharpeningEffect;
	Tr2Upscaling::Technique m_currentUpscalingTechnique;
	Tr2Sharpening::Technique m_currentSharpeningTechnique;
	Tr2Upscaling::Setting m_currentSetting;
	bool m_frameGeneration;

	uint32_t m_renderWidth;
	uint32_t m_renderHeight;
	uint32_t m_displayWidth;
	uint32_t m_displayHeight;

	bool m_debugRenderSize;
};

TYPEDEF_BLUECLASS( Tr2PPUpscalingEffect );
