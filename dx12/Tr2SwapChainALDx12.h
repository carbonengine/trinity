////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2TextureAL.h"
#include "../Tr2AdapterStructures.h"


class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;


class Tr2SwapChainAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SWAP_CHAIN>
{
public:
	Tr2SwapChainAL();
	~Tr2SwapChainAL();

	ALResult Create( Tr2WindowHandle windowHandle, Tr2PrimaryRenderContextAL &renderContext );
	void Destroy();

	bool IsValid() const;

	ALResult Present( Tr2RenderContextAL& renderContext );

	int GetWidth() const;
	int GetHeight() const;

	Tr2TextureAL m_backBuffer;

	bool operator==( const Tr2SwapChainAL& other ) const;

	Tr2ALMemoryType GetMemoryClass() const;

	ALResult CreateDx12( const Tr2PresentParametersAL& presentationParameters, ID3D12Device* device, IDXGIOutput* output, ID3D12CommandQueue* commandQueue, Tr2PrimaryRenderContextAL &renderContext );

	static ALResult GetBackBuffers(
		Tr2PrimaryRenderContextAL* primaryContext,
		std::vector<CComPtr<ID3D12Resource>>& backBuffers,
		std::vector<std::shared_ptr<RenderTargetViewDx12>>& rtvs,
		ID3D12Device* device,
		IDXGISwapChain1* swapChain);

private:
	Tr2SwapChainAL( const Tr2SwapChainAL& ) /* = delete */;
	Tr2SwapChainAL& operator=( const Tr2SwapChainAL& ) /* = delete */;

	CComPtr<IDXGISwapChain3> m_swapChain;
	Tr2PrimaryRenderContextAL* m_owner;
	Tr2PresentParametersAL m_presentParameters;

	friend class Tr2PrimaryRenderContextAL;
};

#endif
