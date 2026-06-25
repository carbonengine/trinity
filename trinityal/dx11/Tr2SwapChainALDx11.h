// Copyright © 2012 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

#include "../include/Tr2SwapChainAL.h"
#include "../include/Tr2TextureAL.h"
#include "../Tr2MemoryCounterAL.h"


namespace TrinityALImpl
{
class Tr2SwapChainAL : public Tr2DeviceResourceAL<Tr2SwapChainAL>
{
public:
	Tr2SwapChainAL();

	ALResult Create( Tr2WindowHandle windowHandle, Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;

	ALResult Present( Tr2RenderContextAL& renderContext );

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;

	::Tr2TextureAL m_backBuffer;

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	ALResult SetName( const char* name );

private:
	Tr2SwapChainAL( const Tr2SwapChainAL& ) /* = delete */;
	Tr2SwapChainAL& operator=( const Tr2SwapChainAL& ) /* = delete */;

	DXGI_SWAP_CHAIN_DESC m_description;
	CComPtr<IDXGISwapChain> m_swapChain;
	uint32_t m_width;
	uint32_t m_height;
	std::string m_name;
};

}

#endif