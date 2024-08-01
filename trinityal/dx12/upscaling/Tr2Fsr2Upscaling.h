////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "include/upscaling/Tr2UpscalingAL.h"
#include <FidelityFX/host/ffx_fsr2.h>
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>
#include "../Tr2TextureALDx12.h"

namespace Fsr2Utils
{
void LogFsr2Message( FfxMsgType type, const wchar_t* message );
FfxResource ConvertTextureToFfxResource( Tr2TextureAL* texture, const wchar_t* textureName, FfxResourceStates state = FFX_RESOURCE_STATE_COMPUTE_READ );
}

class Tr2Fsr2UpscalingTechnique : public TrinityALImpl::Tr2UpscalingTechniqueDx12
{
public:
	Tr2Fsr2UpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	~Tr2Fsr2UpscalingTechnique();

	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;
	virtual bool IsTemporal() const override;

private:
	virtual Tr2UpscalingContextAL* CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params ) override;

};

class Tr2Fsr2UpscalingContext : public Tr2UpscalingContextAL
{
public:
	Tr2Fsr2UpscalingContext( Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2UpscalingAL::UpscalingContextParams params );
	~Tr2Fsr2UpscalingContext();

	virtual bool HasSharpening() const override;
	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:
	FfxFsr2ContextDescription m_initializationParameters;
	FfxFsr2Context m_context;
	bool m_setup;
};
#endif