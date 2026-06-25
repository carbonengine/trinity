// Copyright © 2023 CCP ehf.

#pragma once


#include "../Tr2DeviceResourceAL.h"
#include "../ALResult.h"

class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;
class Tr2TextureAL;
namespace TrinityALImpl
{
class Tr2SwapChainAL;
}


class Tr2SwapChainAL
{
public:
	Tr2SwapChainAL();

	ALResult Create( Tr2WindowHandle windowHandle, Tr2PrimaryRenderContextAL& renderContext );

	ALResult Present( Tr2RenderContextAL& renderContext );

	bool IsValid() const;
	Tr2TextureAL GetBackBuffer() const;
	uint32_t GetWidth() const;
	uint32_t GetHeight() const;

	bool operator==( const Tr2SwapChainAL& other ) const;

	Tr2ALMemoryType GetMemoryClass() const;

	ALResult SetName( const char* name );

private:
	std::shared_ptr<TrinityALImpl::Tr2SwapChainAL> m_swapChain;
	friend class Tr2RenderContextAL;
};
