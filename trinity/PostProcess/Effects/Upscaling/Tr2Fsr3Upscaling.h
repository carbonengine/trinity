////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#pragma once

#include "ITr2Upscaling.h" 
#include "Tr2UpscalingUtils.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include <FidelityFX/host/ffx_fsr3.h>


BLUE_DECLARE( Tr2Effect );

BLUE_CLASS( Tr2Fsr3Upscaling ) : public ITr2Upscaling, INotify
{
public:
	EXPOSE_TO_BLUE();
	Tr2Fsr3Upscaling( IRoot* lockobj = NULL );
	~Tr2Fsr3Upscaling();

	bool OnModified( Be::Var * value ) override;
	bool IsDirty() const override;

	bool IsApplicable() const;
	Tr2Upscaling::UpscalingType GetUpscalingType() const;
	const std::vector<Tr2Upscaling::Setting> GetAvailableSettings() const;

	void GetJitter( float& x, float& y );
	void GetJitterOffset( float& x, float& y );
	float GetMipLevelBias() const;
	void GetRenderSize( uint32_t& width, uint32_t& height ) const;

	void ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight );
	void Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext );
	void Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation );

	bool NeedsExposureTexture() const;
	bool UsesExposureTexture() const override;

private:
	void ClearFsrResources();

	FfxResource ConvertTextureToFfxResource( Tr2RenderContext& renderContext, ITr2TextureProvider* texture, const wchar_t* textureName, FfxResourceStates state = FfxResourceStates::FFX_RESOURCE_STATE_COMPUTE_READ);
	FfxFsr3ContextDescription m_initializationParameters = {};
	FfxFsr3Context m_context;
	bool m_isSetup;

	uint32_t m_renderWidth;
	uint32_t m_renderHeight;
	uint32_t m_displayHeight;
	uint32_t m_displayWidth;

	uint32_t m_jitterIndex;
	float m_jitterX;
	float m_jitterY;
	float m_jitterXScale;
	float m_jitterYScale;
	bool m_reset;
	Be::Time m_lastTime;

	float m_upscaling;
	float m_sharpness;
	float m_preexposure;

	bool m_useHDR;

	bool m_dirty;
	bool m_usingExposure;

	typedef enum Fsr3BackendTypes : uint32_t
	{
		FSR3_BACKEND_SHARED_RESOURCES,
		FSR3_BACKEND_UPSCALING,
		FSR3_BACKEND_FRAME_INTERPOLATION,
		FSR3_BACKEND_COUNT
	} Fsr3BackendTypes;
	FfxInterface m_ffxFsr3Backends[FSR3_BACKEND_COUNT] = {};

};
TYPEDEF_BLUECLASS( Tr2Fsr3Upscaling );
#else

#include "Tr2NoopUpscaling.h"
BLUE_CLASS( Tr2Fsr3Upscaling ) : public Tr2NoopUpscaling
{
public:
	EXPOSE_TO_BLUE();
	Tr2Fsr3Upscaling( IRoot* lockobj = NULL ){};
	~Tr2Fsr3Upscaling(){};
	bool IsApplicable() const
	{
		return false;
	};
};

TYPEDEF_BLUECLASS( Tr2Fsr3Upscaling );

#endif
