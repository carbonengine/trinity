////////////////////////////////////////////////////////////
//
//    Created:   April 2012
//    Copyright: CCP 2012
//

#pragma once

#if TRINITY_PLATFORM==TRINITY_DIRECTX9

#include "../include/Tr2SwapChainAL.h"
#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2TextureAL.h"

namespace TrinityALImpl
{
	class Tr2SwapChainAL :
		public Tr2DeviceResourceAL<Tr2SwapChainAL>
	{
	public:
		Tr2SwapChainAL();

		ALResult Create( Tr2WindowHandle windowHandle, Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();

		bool IsValid() const;

		ALResult Present( Tr2RenderContextAL& renderContext );

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

		::Tr2TextureAL m_backBuffer;
	private:
		Tr2SwapChainAL( const Tr2SwapChainAL& ) /* = delete */;
		Tr2SwapChainAL& operator=( const Tr2SwapChainAL& ) /* = delete */;

		D3DPRESENT_PARAMETERS m_presentParam;
		CComPtr<IDirect3DSwapChain9> m_swapChain;
		Tr2MemoryCounterAL m_memory;
	};
}

#endif