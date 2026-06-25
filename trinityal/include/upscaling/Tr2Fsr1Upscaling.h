// Copyright © 2024 CCP ehf.

#pragma once

#if TRINITY_PLATFORM != TRINITY_STUB

#include "Tr2UpscalingAL.h"
#include "Tr2RenderContextAL.h"
#include "Tr2ShaderAL.h"
#include "Tr2ResourceSetAL.h"
#include "Tr2ShaderProgramAL.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "dx12/upscaling/Tr2UpscalingALDx12.h"
#define TECHNIQUE_PARENT_CLASS TrinityALImpl::Tr2UpscalingTechniqueDx12
#elif TRINITY_PLATFORM == TRINITY_DIRECTX11
#include "dx11/upscaling/Tr2UpscalingALDx11.h"
#define TECHNIQUE_PARENT_CLASS TrinityALImpl::Tr2UpscalingTechniqueDx11
#else
#define TECHNIQUE_PARENT_CLASS Tr2UpscalingTechniqueAL
#endif

class Tr2Fsr1UpscalingTechnique : public TECHNIQUE_PARENT_CLASS
{
public:
	Tr2Fsr1UpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	~Tr2Fsr1UpscalingTechnique();
	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;
	virtual bool IsTemporal() const override;
	virtual bool IsAvailable() const override;

private:
	virtual Tr2UpscalingContextAL* CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params ) override;
};

class Tr2Fsr1UpscalingContext : public Tr2UpscalingContextAL
{
public:
	Tr2Fsr1UpscalingContext( Tr2UpscalingAL::Setting setting, Tr2UpscalingAL::UpscalingContextParams params );
	~Tr2Fsr1UpscalingContext();

	virtual bool HasSharpening() const override;
	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:
	Tr2SamplerStateAL m_sampler;

	Tr2ShaderProgramAL m_easuProgram;

	Tr2ConstantBufferAL m_constantBuffer;
};

#endif
