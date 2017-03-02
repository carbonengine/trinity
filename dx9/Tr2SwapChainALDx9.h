////////////////////////////////////////////////////////////
//
//    Created:   April 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2SwapChainALDx9_H
#define Tr2SwapChainALDx9_H

#if TRINITY_PLATFORM==TRINITY_DIRECTX9

#include "Tr2AutoResetObjectAL.h"

class Tr2RenderContextAL;

// --------------------------------------------------------------------------------------
// Description:
//   AL wrapper for swap chain. Maintains a swap chain object and associated back buffer
//   and depth stencil buffer.
// See Also:
//   Tr2DepthStencilAL, Tr2RenderTargetAL
// --------------------------------------------------------------------------------------
class Tr2SwapChainAL: 
	public Tr2AutoResetObjectAL, 
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

	void ReleaseALResource();
	void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext );

	D3DPRESENT_PARAMETERS m_presentParam;
	CComPtr<IDirect3DSwapChain9> m_swapChain;
};

#endif // TRINITY_PLATFORM==TRINITY_DIRECTX9

#endif // Tr2SwapChainALDx9_H