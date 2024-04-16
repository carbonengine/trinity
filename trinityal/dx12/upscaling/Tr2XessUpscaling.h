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
	Tr2XessUpscalingTechnique( Tr2UpscalingAL::Setting setting, bool frameGeneration );
	~Tr2XessUpscalingTechnique();
	virtual void Destroy( Tr2RenderContextAL& renderContext ) override;

	virtual Tr2UpscalingAL::Result Setup() override;

private:
	virtual Tr2UpscalingContext* CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight ) override;
};

class Tr2XessUpscalingContext : public Tr2UpscalingContext
{
public:
	Tr2XessUpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration );
	virtual Tr2UpscalingAL::Result Setup( Tr2RenderContextAL& renderContext ) override;

	~Tr2XessUpscalingContext();

	virtual bool IsTemporal() const override;
	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:
	void Destroy( Tr2RenderContextAL& renderContext );

	xess_context_handle_t m_context;
	_xess_quality_settings_t m_xessSetting;
	Tr2UpscalingAL::JitterSequence m_jitterSequence;
	bool m_setup;
	bool m_setupWithExposure;

	friend class Tr2XessUpscalingTechnique;
};
#endif