////////////////////////////////////////////////////////////
//
//    Created:   April 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2SwapChainALDx11_H
#define Tr2SwapChainALDx11_H

#if TRINITY_PLATFORM==TRINITY_DIRECTX11

class Tr2RenderContextAL;

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

	Tr2RenderTargetAL m_backBuffer;

	bool operator==( const Tr2SwapChainAL& other ) const { return m_swapChain == other.m_swapChain; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }
private:
	Tr2SwapChainAL( const Tr2SwapChainAL& ) /* = delete */;
	Tr2SwapChainAL& operator=( const Tr2SwapChainAL& ) /* = delete */;

	DXGI_SWAP_CHAIN_DESC m_description;
	CComPtr<IDXGISwapChain> m_swapChain;
};

#endif // TRINITY_PLATFORM==TRINITY_DIRECTX11

#endif // Tr2SwapChainALDx11_H