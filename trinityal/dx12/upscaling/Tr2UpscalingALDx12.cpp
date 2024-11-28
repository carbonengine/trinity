////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2UpscalingALDx12.h"
#include "Tr2DlssUpscaling.h"
#include <upscaling/Tr2Fsr1Upscaling.h>
#include "Tr2Fsr3Upscaling.h"
#include "Tr2XessUpscaling.h"

namespace TrinityALImpl
{
	TrinityALImpl::Tr2UpscalingTechniqueDx12* CreateUpscalingTechnique( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) 
	{
		TrinityALImpl::Tr2UpscalingTechniqueDx12* techniqueImpl = nullptr;
		switch( technique )
		{
		case Tr2UpscalingAL::Technique::FSR1:
			techniqueImpl = new Tr2Fsr1UpscalingTechnique( renderContext, technique, setting, frameGeneration, adapter );
			break;
		case Tr2UpscalingAL::Technique::DLSS:
			techniqueImpl = new Tr2DlssUpscalingTechnique( renderContext, technique, setting, frameGeneration, adapter );
			break;
		case Tr2UpscalingAL::Technique::XESS:
			techniqueImpl = new Tr2XessUpscalingTechnique( renderContext, technique, setting, frameGeneration, adapter );
			break;
		default:
			break;
		}

		if( techniqueImpl && techniqueImpl->IsAvailable( ) )
		{
			return techniqueImpl;
		}
		delete techniqueImpl;
		techniqueImpl = nullptr;
		return nullptr;
	}

	Tr2UpscalingTechniqueDx12::Tr2UpscalingTechniqueDx12( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter ) : 
		Tr2UpscalingTechniqueAL( renderContext, technique, setting, frameGeneration, adapter )
	{
	}

	Tr2UpscalingTechniqueDx12::~Tr2UpscalingTechniqueDx12()
	{
	}

	bool Tr2UpscalingTechniqueDx12::ReplacesSwapchain() const
	{
		return false;
	} 

	bool Tr2UpscalingTechniqueDx12::ReplacesDevice() const
	{
		return false;
	}
	
	bool Tr2UpscalingTechniqueDx12::ReplacesCommandQueue() const
	{
		return false;
	}
	
	bool Tr2UpscalingTechniqueDx12::ReplacesFactory() const
	{
		return false;
	}

	void Tr2UpscalingTechniqueDx12::ReplaceSwapchain( CComPtr<IDXGISwapChain4>& swapchain, Tr2WindowHandle hwnd, ID3D12CommandQueue* commandQueue )
	{
	}

	CComPtr<ID3D12Device> Tr2UpscalingTechniqueDx12::ReplaceDevice( CComPtr<ID3D12Device>& device ) 
	{
		return device;
	}

	CComPtr<ID3D12CommandQueue> Tr2UpscalingTechniqueDx12::ReplaceCommandQueue( CComPtr<ID3D12CommandQueue>& commandQueue ) 
	{
		return commandQueue;
	}
	
	CComPtr<IDXGIFactory4> Tr2UpscalingTechniqueDx12::ReplaceFactory( CComPtr<IDXGIFactory4>& factory )
	{
		return factory;
	}
}

#endif
