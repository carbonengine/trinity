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
	Tr2Fsr2UpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration );
	~Tr2Fsr2UpscalingTechnique();

	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;
	virtual Tr2UpscalingAL::Result Setup() override;
	virtual void Destroy( Tr2RenderContextAL& renderContext ) override;

private:
	virtual Tr2UpscalingContextAL* CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) override;

};

class Tr2Fsr2UpscalingContext : public Tr2UpscalingContextAL
{
public:
	Tr2Fsr2UpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat );
	~Tr2Fsr2UpscalingContext();

	virtual Tr2UpscalingAL::Result Setup( Tr2RenderContextAL& renderContext ) override;
	void Destroy( Tr2RenderContextAL& renderContex );
	virtual bool IsTemporal() const override;
	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:
	FfxFsr2ContextDescription m_initializationParameters;
	FfxFsr2Context m_context;
	bool m_setup;
};
#endif