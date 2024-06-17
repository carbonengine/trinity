////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#if TRINITY_PLATFORM != TRINITY_STUB

#include "Tr2UpscalingAL.h"
#include "Tr2RenderContextAL.h"
#include "Tr2ShaderAL.h"
#include "Tr2ResourceSetAL.h"
#include "Tr2ShaderProgramAL.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	#include "dx12/upscaling/Tr2UpscalingALDx12.h"
	#define PARENT_CLASS TrinityALImpl::Tr2UpscalingTechniqueDx12
#elif TRINITY_PLATFORM == TRINITY_DIRECTX11
	#include "dx11/upscaling/Tr2UpscalingALDx11.h"
	#define PARENT_CLASS TrinityALImpl::Tr2UpscalingTechniqueDx11
#else
	#define PARENT_CLASS Tr2UpscalingTechniqueAL
#endif

class Tr2Fsr1UpscalingTechnique : public PARENT_CLASS
{
public:
	Tr2Fsr1UpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	~Tr2Fsr1UpscalingTechnique();
	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;

	virtual void Destroy( Tr2RenderContextAL& renderContext ) override;

private:
	virtual Tr2UpscalingContextAL* CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) override;
};

class Tr2Fsr1UpscalingContext : public Tr2UpscalingContextAL
{
public:
	Tr2Fsr1UpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat );
	~Tr2Fsr1UpscalingContext();

	virtual Tr2UpscalingAL::Result Setup( Tr2RenderContextAL& renderContext ) override;
	virtual void Destroy( Tr2RenderContextAL& renderContext ) override;
	virtual bool IsTemporal() const override;
	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:

	Tr2SamplerStateAL m_sampler;
	
	Tr2ShaderProgramAL m_easuProgram;

	Tr2ConstantBufferAL m_constantBuffer;
};

#endif