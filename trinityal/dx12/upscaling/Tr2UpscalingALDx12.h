////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "../include/upscaling/Tr2UpscalingAL.h"

namespace TrinityALImpl
{
	const std::vector<Tr2UpscalingAL::Technique> AVAILABLE_UPSCALING_TECHNIQUES = {
		Tr2UpscalingAL::Technique::DLSS,
		Tr2UpscalingAL::Technique::FSR2,
		Tr2UpscalingAL::Technique::FSR3,
		Tr2UpscalingAL::Technique::XESS,
	};

	class Tr2UpscalingTechniqueDx12 : public Tr2UpscalingTechniqueAL
	{
	public: 
		Tr2UpscalingTechniqueDx12( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration );

		virtual bool ReplacesSwapchain() const;
		virtual bool OverridesDeviceCreation() const;
		virtual bool OverridesCommandQueueCreation() const;
		virtual bool OverridesFactory2Creation() const;

		virtual void ReplaceSwapchain( CComPtr<IDXGISwapChain4>& swapchain, Tr2WindowHandle hwnd, ID3D12CommandQueue* commandQueue );
		virtual HRESULT D3D12CreateDevice( IUnknown* adapter, D3D_FEATURE_LEVEL featureLevel, CComPtr<ID3D12Device>& device ) ;
		virtual HRESULT CreateCommandQueue( CComPtr<ID3D12Device>& device, D3D12_COMMAND_QUEUE_DESC* desc, CComPtr<ID3D12CommandQueue>& commandQueue ) ;
		virtual HRESULT CreateDXGIFactory2( UINT flags, CComPtr<IDXGIFactory4>& factory ) ;
	};

	TrinityALImpl::Tr2UpscalingTechniqueDx12* CreateUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter );

}

#endif