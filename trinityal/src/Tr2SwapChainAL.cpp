// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "../include/Tr2SwapChainAL.h"
#include TRINITY_AL_PLATFORM_INCLUDE( Tr2SwapChainAL )

namespace
{
std::shared_ptr<TrinityALImpl::Tr2SwapChainAL> NullSC()
{
	static std::shared_ptr<TrinityALImpl::Tr2SwapChainAL> nullSC = std::make_shared<TrinityALImpl::Tr2SwapChainAL>();
	return nullSC;
}
}

Tr2SwapChainAL::Tr2SwapChainAL() :
	m_swapChain( NullSC() )
{
}

ALResult Tr2SwapChainAL::Create( Tr2WindowHandle windowHandle, Tr2PrimaryRenderContextAL& renderContext )
{
	m_swapChain = std::make_shared<TrinityALImpl::Tr2SwapChainAL>();
	auto hr = m_swapChain->Create( windowHandle, renderContext );
	if( FAILED( hr ) )
	{
		m_swapChain = NullSC();
		return hr;
	}
	return hr;
}

ALResult Tr2SwapChainAL::Present( Tr2RenderContextAL& renderContext )
{
	return m_swapChain->Present( renderContext );
}

bool Tr2SwapChainAL::IsValid() const
{
	return m_swapChain->IsValid();
}

Tr2TextureAL Tr2SwapChainAL::GetBackBuffer() const
{
	return m_swapChain->m_backBuffer;
}

uint32_t Tr2SwapChainAL::GetWidth() const
{
	return m_swapChain->GetWidth();
}

uint32_t Tr2SwapChainAL::GetHeight() const
{
	return m_swapChain->GetHeight();
}

bool Tr2SwapChainAL::operator==( const Tr2SwapChainAL& other ) const
{
	return m_swapChain == other.m_swapChain;
}

Tr2ALMemoryType Tr2SwapChainAL::GetMemoryClass() const
{
	return m_swapChain->GetMemoryClass();
}

ALResult Tr2SwapChainAL::SetName( const char* name )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !name )
	{
		return E_INVALIDARG;
	}
	return m_swapChain->SetName( name );
}
