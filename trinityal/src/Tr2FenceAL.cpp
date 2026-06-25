// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "include/Tr2FenceAL.h"
#include TRINITY_AL_PLATFORM_INCLUDE( Tr2FenceAL )

namespace
{

std::shared_ptr<TrinityALImpl::Tr2FenceAL> NullFence()
{
	static std::shared_ptr<TrinityALImpl::Tr2FenceAL> nullFence = std::make_shared<TrinityALImpl::Tr2FenceAL>();
	return nullFence;
}
}

Tr2FenceAL::Tr2FenceAL() :
	m_fence( NullFence() )
{
}

ALResult Tr2FenceAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	m_fence = std::make_shared<TrinityALImpl::Tr2FenceAL>();
	auto result = m_fence->Create( renderContext );
	if( FAILED( result ) )
	{
		m_fence = NullFence();
	}
	return result;
}

ALResult Tr2FenceAL::PutFence( Tr2RenderContextAL& renderContext )
{
	return m_fence->PutFence( renderContext );
}

ALResult Tr2FenceAL::IsReached( bool& isReached, Tr2RenderContextAL& renderContext )
{
	return m_fence->IsReached( isReached, renderContext );
}

ALResult Tr2FenceAL::Wait( Tr2RenderContextAL& renderContext )
{
	return m_fence->Wait( renderContext );
}

bool Tr2FenceAL::IsValid() const
{
	return m_fence->IsValid();
}

bool Tr2FenceAL::operator==( const Tr2FenceAL& other ) const
{
	return m_fence == other.m_fence;
}

Tr2ALMemoryType Tr2FenceAL::GetMemoryClass() const
{
	return m_fence->GetMemoryClass();
}

ALResult Tr2FenceAL::SetName( const char* name )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !name )
	{
		return E_INVALIDARG;
	}
	return m_fence->SetName( name );
}
