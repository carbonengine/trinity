////////////////////////////////////////////////////////////
//
//    Created:   April 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2SwapChainALDx9_H
#define Tr2SwapChainALDx9_H


#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2RenderTargetAL.h"


class Tr2RenderContextAL;


#if TRINITY_PLATFORM==TRINITY_DIRECTX9

// --------------------------------------------------------------------------------------
// Description:
//   AL wrapper for swap chain. Maintains a swap chain object and associated back buffer
//   and depth stencil buffer.
// See Also:
//   Tr2DepthStencilAL, Tr2RenderTargetAL
// --------------------------------------------------------------------------------------
class Tr2SwapChainAL: 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SWAP_CHAIN>
{
public:
	Tr2SwapChainAL();

	ALResult Create( Tr2WindowHandle windowHandle, Tr2RenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;

	ALResult Present( Tr2RenderContextAL& renderContext );

	int GetWidth() const;
	int GetHeight() const;

	bool operator==( const Tr2SwapChainAL& other ) const { return m_swapChain == other.m_swapChain; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }

	Tr2RenderTargetAL m_backBuffer;
private:
	Tr2SwapChainAL( const Tr2SwapChainAL& ) /* = delete */;
	Tr2SwapChainAL& operator=( const Tr2SwapChainAL& ) /* = delete */;

	D3DPRESENT_PARAMETERS m_presentParam;
	CComPtr<IDirect3DSwapChain9> m_swapChain;
	Tr2MemoryCounterAL m_memory;
};

#endif // TRINITY_PLATFORM==TRINITY_DIRECTX9

#endif // Tr2SwapChainALDx9_H