// Copyright © 2024 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "include/upscaling/Tr2UpscalingAL.h"
#include <xess/xess.h>

class Tr2XessUpscalingTechnique : public TrinityALImpl::Tr2UpscalingTechniqueDx12
{
public:
	Tr2XessUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	~Tr2XessUpscalingTechnique();

	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;
	virtual bool IsTemporal() const override;

	virtual bool IsAvailable() const override;

private:
	virtual Tr2UpscalingContextAL* CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params ) override;
};

class Tr2XessUpscalingContext : public Tr2UpscalingContextAL
{
public:
	Tr2XessUpscalingContext( Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2UpscalingAL::UpscalingContextParams params );
	void Setup();
	void Destroy();

	~Tr2XessUpscalingContext();

	virtual bool HasSharpening() const override;

	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:
	xess_context_handle_t m_context;
	_xess_quality_settings_t m_xessSetting;
	Tr2UpscalingAL::JitterSequence m_jitterSequence;
	bool m_setup;
	bool m_setupWithExposure;

	friend class Tr2XessUpscalingTechnique;
};
#endif