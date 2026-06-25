// Copyright © 2024 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "../Tr2TextureALDx12.h"
#include "include/upscaling/Tr2UpscalingAL.h"
#include <ffx_api/ffx_api.hpp>
#include <ffx_api/ffx_api_types.h>
#include <ffx_api/dx12/ffx_api_dx12.hpp>
#include <ffx_api/ffx_upscale.hpp>
#include <ffx_api/ffx_framegeneration.hpp>

namespace Fsr3Utils
{
void LogFsr3Message( uint32_t type, const wchar_t* message );
FfxApiResource ConvertTextureToFfxResource( const Tr2TextureAL* texture, const wchar_t* textureName );
FfxApiSurfaceFormat GetFfxSurfaceFormat( Tr2RenderContextEnum::PixelFormat format );
}

class Tr2Fsr3UpscalingTechnique : public TrinityALImpl::Tr2UpscalingTechniqueDx12
{
public:
	Tr2Fsr3UpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	~Tr2Fsr3UpscalingTechnique();

	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;
	virtual bool IsTemporal() const override;

	virtual void ReleaseResources() override;

	virtual bool SupportsFrameGeneration() const override;
	virtual void MarkFrameEvent( Tr2RenderContextEnum::FrameEvent& frameEvent ) override;

	virtual bool ReplacesSwapchain() const override;
	virtual void ReplaceSwapchain( CComPtr<IDXGISwapChain4>& swapchain, Tr2WindowHandle hwnd, ID3D12CommandQueue* commandQueue ) override;

private:
	virtual Tr2UpscalingContextAL* CreateContextInstance( Tr2UpscalingAL::UpscalingContextParams params ) override;
	bool m_supportsFrameGeneration;

	ffx::Context m_swapChainContext;
};


class Tr2Fsr3UpscalingContext : public Tr2UpscalingContextAL
{
public:
	Tr2Fsr3UpscalingContext( Tr2UpscalingAL::Setting setting, bool frameGeneration, ffx::Context swapchainContext, Tr2UpscalingAL::UpscalingContextParams params );
	~Tr2Fsr3UpscalingContext();

	virtual bool HasSharpening() const override;
	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;
	virtual void SetHudLessTexture( const Tr2TextureAL* texture ) override;
	virtual void SetupForReuse() override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:
	Tr2UpscalingAL::Result SetupFrameGen();
	void TearDownFrameGen();
	Tr2UpscalingAL::Result SetupUpscaling();
	Tr2UpscalingAL::Result DispatchUpscaling( Tr2UpscalingAL::DispatchParameters& dispatchParameters );
	Tr2UpscalingAL::Result DispatchFrameGen( Tr2UpscalingAL::DispatchParameters& dispatchParameters );

	ffx::Context m_upscalingFfxContext;
	ffx::Context m_framegenerationFfxContext;
	ffx::Context m_swapchainFfxContext;
	ffx::ConfigureDescFrameGeneration m_frameGenerationConfig{};
	ffx::CreateBackendDX12Desc m_backendDesc;

	FfxApiFrameGenerationDispatchFunc m_frameGenerationCallback;

	bool m_setup;
	std::unique_ptr<Tr2TextureAL> m_reactiveMask;

	friend class Tr2Fsr3UpscalingTechnique;
};

#endif