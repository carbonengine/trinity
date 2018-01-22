////////////////////////////////////////////////////////////
//
//    Created:   April 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2SwapChainALDx11_H
#define Tr2SwapChainALDx11_H


#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2TextureAL.h"


class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;


#if TRINITY_PLATFORM==TRINITY_DIRECTX11

class Tr2SwapChainAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SWAP_CHAIN>
{
public:
	Tr2SwapChainAL();

	ALResult Create( Tr2WindowHandle windowHandle, Tr2PrimaryRenderContextAL &renderContext );
	void Destroy();

	bool IsValid() const;

	ALResult Present( Tr2RenderContextAL& renderContext );

	int GetWidth() const;
	int GetHeight() const;

	Tr2TextureAL m_backBuffer;

	bool operator==( const Tr2SwapChainAL& other ) const { return m_swapChain == other.m_swapChain; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }
private:
	Tr2SwapChainAL( const Tr2SwapChainAL& ) /* = delete */;
	Tr2SwapChainAL& operator=( const Tr2SwapChainAL& ) /* = delete */;

	DXGI_SWAP_CHAIN_DESC m_description;
	CComPtr<IDXGISwapChain> m_swapChain;
	uint32_t m_width;
	uint32_t m_height;
};

#endif // TRINITY_PLATFORM==TRINITY_DIRECTX11

#endif // Tr2SwapChainALDx11_H