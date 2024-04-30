////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2SwapChainAL.h"
#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2TextureAL.h"
#include "../Tr2AdapterStructures.h"
#include "util/DescriptorHeapViewDx12.h"

namespace Tr2SwapChainUtils
{
	const uint32_t BACK_BUFFER_COUNT = 3;

	DXGI_FORMAT SafeConvertD3DBackBufferFormat( Tr2RenderContextEnum::PixelFormat bbFormat );
	
	ALResult CreateSwapChain( 
		CComPtr<IDXGISwapChain4>& swapChain,
		Tr2WindowHandle focusWindow,
		const Tr2PresentParametersAL& presentationParameters,
		ID3D12CommandQueue* commandQueue,
		IDXGIOutput* output,
		Tr2PrimaryRenderContextAL& renderContext );
	
	ALResult FillPresentationParameters( Tr2PresentParametersAL& presentationParameters, Tr2WindowHandle windowHandle );
}

namespace TrinityALImpl
{
	class Tr2SwapChainAL : public Tr2DeviceResourceAL<Tr2SwapChainAL>
	{
	public:
		Tr2SwapChainAL();
		~Tr2SwapChainAL();

		ALResult Create( Tr2WindowHandle windowHandle, Tr2PrimaryRenderContextAL &renderContext );
		void Destroy();

		bool IsValid() const;

		ALResult Present( Tr2RenderContextAL& renderContext );

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		::Tr2TextureAL m_backBuffer;

		Tr2ALMemoryType GetMemoryClass() const;

		ALResult CreateDx12( const Tr2PresentParametersAL& presentationParameters, IDXGIOutput* output, ID3D12CommandQueue* commandQueue, Tr2PrimaryRenderContextAL &renderContext );

		static ALResult GetBackBuffers(
			Tr2PrimaryRenderContextAL* primaryContext,
			std::vector<CComPtr<ID3D12Resource>>& backBuffers,
			std::vector<std::shared_ptr<RenderTargetViewDx12>>& rtvs,
			IDXGISwapChain1* swapChain );

		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		Tr2SwapChainAL( const Tr2SwapChainAL& ) /* = delete */;
		Tr2SwapChainAL& operator=( const Tr2SwapChainAL& ) /* = delete */;

		CComPtr<IDXGISwapChain3> m_swapChain;
		Tr2PrimaryRenderContextAL* m_owner;
		Tr2PresentParametersAL m_presentParameters;
		std::string m_name;

		friend class Tr2PrimaryRenderContextAL;
	};
}
#endif
