////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2UpscalingALDx12.h"
#include "Tr2DlssUpscaling.h"
#include "Tr2Fsr2Upscaling.h"
#include "Tr2Fsr3Upscaling.h"
#include "Tr2XessUpscaling.h"

namespace TrinityALImpl
{
	TrinityALImpl::Tr2UpscalingTechniqueDx12* CreateUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter )
	{
		TrinityALImpl::Tr2UpscalingTechniqueDx12* techniqueImpl = nullptr;
		switch( technique )
		{
		case Tr2UpscalingAL::Technique::FSR2:
			techniqueImpl = new Tr2Fsr2UpscalingTechnique( technique, setting, frameGeneration );
			break;
		case Tr2UpscalingAL::Technique::FSR3:
			techniqueImpl = new Tr2Fsr3UpscalingTechnique( technique, setting, frameGeneration );
			break;
		case Tr2UpscalingAL::Technique::DLSS:
			techniqueImpl = new Tr2DlssUpscalingTechnique( technique, setting, frameGeneration, adapter );
			break;
		case Tr2UpscalingAL::Technique::XESS:
			techniqueImpl = new Tr2XessUpscalingTechnique( technique, setting, frameGeneration );
			break;
		}
		if( techniqueImpl && techniqueImpl->IsAvailable( renderContext, adapter ) )
		{
			return techniqueImpl;
		}
		delete techniqueImpl;
		techniqueImpl = nullptr;
		return nullptr;
	}

	Tr2UpscalingTechniqueDx12::Tr2UpscalingTechniqueDx12( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration ) : 
		Tr2UpscalingTechniqueAL( technique, setting, frameGeneration )
	{
	}

	bool Tr2UpscalingTechniqueDx12::ReplacesSwapchain() const
	{
		return false;
	}
	
	bool Tr2UpscalingTechniqueDx12::OverridesDeviceCreation() const
	{
		return false;
	}
	
	bool Tr2UpscalingTechniqueDx12::OverridesCommandQueueCreation() const
	{
		return false;
	}
	
	bool Tr2UpscalingTechniqueDx12::OverridesFactory2Creation() const
	{
		return false;
	}

	void Tr2UpscalingTechniqueDx12::ReplaceSwapchain( CComPtr<IDXGISwapChain4>& swapchain, Tr2WindowHandle hwnd, ID3D12CommandQueue* commandQueue )
	{
	}

	HRESULT Tr2UpscalingTechniqueDx12::D3D12CreateDevice( IUnknown* adapter, D3D_FEATURE_LEVEL featureLevel, CComPtr<ID3D12Device>& device ) 
	{
		return S_FALSE;
	}

	HRESULT Tr2UpscalingTechniqueDx12::CreateCommandQueue( CComPtr<ID3D12Device>& device, D3D12_COMMAND_QUEUE_DESC* desc, CComPtr<ID3D12CommandQueue>& commandQueue ) 
	{
		return S_FALSE;
	}
	
	HRESULT Tr2UpscalingTechniqueDx12::CreateDXGIFactory2( UINT flags, CComPtr<IDXGIFactory4>& factory ) 
	{
		return S_FALSE;
	}
}

#endif
