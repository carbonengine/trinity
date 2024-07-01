////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "include/upscaling/Tr2UpscalingAL.h"
#include <xess/xess.h>

class Tr2XessUpscalingTechnique : public TrinityALImpl::Tr2UpscalingTechniqueDx12
{
public:
	Tr2XessUpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	~Tr2XessUpscalingTechnique();

	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;
	virtual bool IsTemporal() const override;

	virtual bool IsAvailable( Tr2RenderContextAL& renderContext ) const override;
	virtual void Destroy( Tr2RenderContextAL& renderContext ) override;

private:
	virtual Tr2UpscalingContextAL* CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) override;
};

class Tr2XessUpscalingContext : public Tr2UpscalingContextAL
{
public:
	Tr2XessUpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration, bool isTemporal, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat );
	virtual Tr2UpscalingAL::Result Setup( Tr2RenderContextAL& renderContext ) override;
	virtual void Destroy( Tr2RenderContextAL& renderContext ) override;

	~Tr2XessUpscalingContext();

	virtual bool HasSharpening() const override;

	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:

	xess_context_handle_t m_context;
	_xess_quality_settings_t m_xessSetting;
	Tr2UpscalingAL::JitterSequence m_jitterSequence;
	bool m_setup;
	bool m_setupWithExposure;

	friend class Tr2XessUpscalingTechnique;
};
#endif