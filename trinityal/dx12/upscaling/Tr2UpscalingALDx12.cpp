////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2UpscalingALDx12.h"
#include "Tr2DlssUpscaling.h"
#include "Tr2Fsr2Upscaling.h"
#include "Tr2Fsr3Upscaling.h"
#include "Tr2XessUpscaling.h"

namespace TrinityALImpl
{
	TrinityALImpl::Tr2UpscalingTechniqueDx12* CreateUpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter )
	{
		switch( technique )
		{
		case Tr2UpscalingAL::Technique::FSR2:
			return new Tr2Fsr2UpscalingTechnique( setting, frameGeneration );
		case Tr2UpscalingAL::Technique::FSR3:
			return new Tr2Fsr3UpscalingTechnique( setting, frameGeneration );
		case Tr2UpscalingAL::Technique::DLSS:
			return new Tr2DlssUpscalingTechnique( setting, frameGeneration, adapter );
		case Tr2UpscalingAL::Technique::XESS:
			return new Tr2XessUpscalingTechnique( setting, frameGeneration );
		}
		return nullptr;
	}

	Tr2UpscalingTechniqueDx12::Tr2UpscalingTechniqueDx12( Tr2UpscalingAL::Setting setting, bool frameGeneration ) : 
		Tr2UpscalingTechniqueAL( setting, frameGeneration )
	{
	}

	bool Tr2UpscalingTechniqueDx12::OverridesSwapChainCreation() const
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

	HRESULT Tr2UpscalingTechniqueDx12::CreateSwapChainForHwnd( IUnknown* pDevice,
									 HWND hWnd,
									 const DXGI_SWAP_CHAIN_DESC1* pDesc,
									 const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
									 IDXGIOutput* pRestrictToOutput,
							 IDXGISwapChain1** ppSwapChain )
	{
		return S_FALSE;
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