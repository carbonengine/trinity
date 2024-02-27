////////////////////////////////////////////////////////////////////////////////
//
// Created:		May 2023
// Copyright:	CCP 2023
//
#pragma once

#include "ITr2Upscaling.h"
#include "Tr2UpscalingUtils.h"

BLUE_DECLARE( Tr2Effect );

BLUE_CLASS( Tr2DlssUpscaling ) :
	public ITr2Upscaling, INotify
{
public:
	EXPOSE_TO_BLUE();

	Tr2DlssUpscaling( IRoot* lockobj = NULL );
	~Tr2DlssUpscaling();

	bool OnModified( Be::Var * value ) override;
	bool IsDirty() const override;

	bool IsApplicable() const;
	Tr2Upscaling::UpscalingType GetUpscalingType() const;
	const std::vector<Tr2Upscaling::Setting> GetAvailableSettings() const;

	void GetJitter( float& x, float& y );
	void GetJitterOffset( float& x, float& y );

	float GetMipLevelBias() const;
	void GetRenderSize( uint32_t & width, uint32_t & height ) const;

	void ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight );
	void Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext & renderContext );
	void Dispatch( Tr2RenderContext & renderContext, Tr2PostProcessRenderInfo & renderInfo, Tr2Upscaling::Textures & textures, const Tr2Upscaling::SceneInformation& sceneInformation );
	void SetUseFrameGeneration( bool enable ) override;
	bool GetUseFrameGeneration( ) const override;

	bool NeedsExposureTexture() const override;
	bool UsesExposureTexture() const override;
	bool NeedsReactiveTexture() const override;
	bool SupportsFrameGeneration() const;

private:
	void SetConstants( const Tr2Upscaling::SceneInformation& sceneInformation );

	float m_upscaling;
	float m_sharpeningAmount;
	bool m_useSharpening;

	bool m_useFrameGeneration;

	uint32_t m_displayWidth;
	uint32_t m_displayHeight;
	uint32_t m_renderWidth;
	uint32_t m_renderHeight;
	float m_jitterX;
	float m_jitterY;
	uint32_t m_jitterIndex;
	Jitter::JitterSequence m_jitterSequence;

	DlssUtils::DlssOptimalSetting m_optimalSettings;
	DlssUtils::DlssOptions m_options;
	Tr2Upscaling::Setting m_currentSetting;

	uint64_t m_vramUsage;
	uint32_t m_minWidthHeight;
	uint32_t m_actualFrames;

	bool m_usingExposure;
	bool m_dirty;
};

TYPEDEF_BLUECLASS( Tr2DlssUpscaling );