// Copyright © 2024 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "../include/upscaling/Tr2UpscalingAL.h"

namespace TrinityALImpl
{
constexpr Tr2UpscalingAL::Technique AVAILABLE_UPSCALING_TECHNIQUES[] = {
	Tr2UpscalingAL::Technique::FSR1,
	Tr2UpscalingAL::Technique::FSR3,
	Tr2UpscalingAL::Technique::XESS,
	Tr2UpscalingAL::Technique::DLSS,
};

class Tr2UpscalingTechniqueDx12 : public Tr2UpscalingTechniqueAL
{
public:
	Tr2UpscalingTechniqueDx12( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );
	virtual ~Tr2UpscalingTechniqueDx12();
	virtual bool ReplacesSwapchain() const;
	virtual bool ReplacesDevice() const;
	virtual bool ReplacesCommandQueue() const;
	virtual bool ReplacesFactory() const;

	virtual void ReplaceSwapchain( CComPtr<IDXGISwapChain4>& swapchain, Tr2WindowHandle hwnd, ID3D12CommandQueue* commandQueue );
	virtual CComPtr<ID3D12Device> ReplaceDevice( CComPtr<ID3D12Device>& device );
	virtual CComPtr<ID3D12CommandQueue> ReplaceCommandQueue( CComPtr<ID3D12CommandQueue>& commandQueue );
	virtual CComPtr<IDXGIFactory4> ReplaceFactory( CComPtr<IDXGIFactory4>& factory );
};

TrinityALImpl::Tr2UpscalingTechniqueDx12* CreateUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );

}

#endif