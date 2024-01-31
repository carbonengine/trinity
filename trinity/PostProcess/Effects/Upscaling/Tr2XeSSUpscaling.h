////////////////////////////////////////////////////////////////////////////////
//
// Created:		October 2023
// Copyright:	CCP 2023
//
#pragma once

#include "ITr2Upscaling.h"
#include "Tr2UpscalingUtils.h"
#include "Shader/Tr2Effect.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include <xess/xess.h>

BLUE_DECLARE( Tr2Effect );

BLUE_CLASS( Tr2XeSSUpscaling ) :
	public ITr2Upscaling, INotify
{
public:
	EXPOSE_TO_BLUE();
	Tr2XeSSUpscaling( IRoot* lockobj = NULL );
	~Tr2XeSSUpscaling();

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

	bool NeedsExposureTexture() const override;
	bool NeedsReactiveTexture() const override;

	static void Initialize();
	static void Shutdown();

private:
	xess_context_handle_t CreateContext( Tr2RenderContext& renderContext ) const;
	ID3D12Resource* GetTexture( ITr2TextureProvider* textureProvider ) const;

	enum Availability
	{
		XESS_AVAILABILITY_UNKNOWN,
		XESS_AVAILABILITY_AVAILABLE,
		XESS_AVAILABILITY_UNAVAILABLE,
	};

	uint32_t m_renderWidth;
	uint32_t m_renderHeight;
	uint32_t m_displayHeight;
	uint32_t m_displayWidth;

	uint32_t m_jitterIndex;
	Jitter::JitterSequence m_jitterSequence;
	float m_jitterX;
	float m_jitterY;
	float m_jitterXScale;
	float m_jitterYScale;
	float m_jitterOffsetXScale;
	float m_jitterOffsetYScale;
	bool m_reset;

	static xess_context_handle_t s_context;
	_xess_quality_settings_t m_xessSetting;

	const char* ResultToString( xess_result_t result ) const;

	float m_upscaling;
	static Availability s_availability;
	static uint32_t s_creationNodeMask;

	bool m_dirty;
	bool m_setup;
	bool m_initialized;
	bool m_useReactive;
	static bool s_initialized;
};
TYPEDEF_BLUECLASS( Tr2XeSSUpscaling );
#else

#include "Tr2NoopUpscaling.h"
BLUE_CLASS( Tr2XeSSUpscaling ) :
	public Tr2NoopUpscaling
{
public:
	EXPOSE_TO_BLUE();
	Tr2XeSSUpscaling( IRoot* lockobj = NULL ){};
	~Tr2XeSSUpscaling(){};

	static void Initialize(){};
	static void Shutdown(){};
	bool IsApplicable() const
	{
		return false;
	};
};

TYPEDEF_BLUECLASS( Tr2XeSSUpscaling );

#endif
