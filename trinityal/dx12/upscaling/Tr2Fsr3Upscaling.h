////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "include/upscaling/Tr2UpscalingAL.h"
#include <FidelityFX/host/ffx_fsr3.h>
#include "../Tr2TextureALDx12.h"

namespace Fsr3Utils
{
	void LogFsr3Message( FfxMsgType type, const wchar_t* message );
	FfxResource ConvertTextureToFfxResource( Tr2TextureAL* texture, const wchar_t* textureName, FfxResourceStates state );
	FfxSurfaceFormat GetFfxSurfaceFormat( Tr2RenderContextEnum::PixelFormat format );
}

class Tr2Fsr3UpscalingTechnique : public TrinityALImpl::Tr2UpscalingTechniqueDx12
{
public:
	Tr2Fsr3UpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration );
	~Tr2Fsr3UpscalingTechnique();

	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;
	virtual void Destroy( Tr2RenderContextAL& renderContext ) override;
	virtual bool SupportsFrameGeneration() const override;
	virtual void MarkFrameEvent( Tr2RenderContextAL& renderContext, Tr2RenderContextEnum::FrameEvent& frameEvent ) override;

	virtual bool ReplacesSwapchain() const override;
	virtual void ReplaceSwapchain( CComPtr<IDXGISwapChain4>& swapchain, Tr2WindowHandle hwnd, ID3D12CommandQueue* commandQueue ) override;

private:
	virtual Tr2UpscalingContextAL* CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) override;
	FfxSwapchain m_framegenSwapchain;
	bool m_attachedToSwapchain;
};


class Tr2Fsr3UpscalingContext : public Tr2UpscalingContextAL
{
public:
	Tr2Fsr3UpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration, FfxSwapchain frameInterpolationSwapchain, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat );
	~Tr2Fsr3UpscalingContext();

	virtual Tr2UpscalingAL::Result Setup( Tr2RenderContextAL& renderContext ) override;
	virtual void Destroy( Tr2RenderContextAL& renderContext ) override;

	virtual bool IsTemporal() const override;
	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;
	virtual void SetHudLessTexture( Tr2TextureAL* texture ) override;

	void GenerateFrame( Tr2RenderContextAL& renderContext );

	virtual Tr2UpscalingAL::Result Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:
	FfxFsr3ContextDescription m_initializationParameters = {};
	FfxFsr3Context m_context;

	typedef enum Fsr3BackendTypes : uint32_t
	{
		FSR3_BACKEND_SHARED_RESOURCES,
		FSR3_BACKEND_UPSCALING,
		FSR3_BACKEND_FRAME_INTERPOLATION,
		FSR3_BACKEND_COUNT
	} Fsr3BackendTypes;
	FfxInterface m_ffxFsr3Backends[FSR3_BACKEND_COUNT] = {};

	bool m_setup;
	FfxFrameGenerationConfig m_frameGenerationConfig = {};
	FfxSwapchain m_framegenSwapchain;

	friend class Tr2Fsr3UpscalingTechnique;
};

#endif