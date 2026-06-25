// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "../include/Tr2ConstantBufferAL.h"
#include "../include/Tr2CapsAL.h"
#include TRINITY_AL_PLATFORM_INCLUDE( Tr2ConstantBufferAL )

namespace
{
std::shared_ptr<TrinityALImpl::Tr2ConstantBufferAL> NullCB()
{
	static std::shared_ptr<TrinityALImpl::Tr2ConstantBufferAL> nullCB = std::make_shared<TrinityALImpl::Tr2ConstantBufferAL>();
	return nullCB;
}
}


Tr2ConstantBufferAL::Tr2ConstantBufferAL() :
	m_buffer( NullCB() )
{
}

ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2PrimaryRenderContextAL& renderContext )
{
	return Create( size, Tr2ConstantUsageAL::REUSABLE, nullptr, renderContext );
}

ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2PrimaryRenderContextAL& renderContext )
{
	if( size == 0 || size > TRINITY_PLATFORM_MAX_CONSTANT_BUFFER_SIZE )
	{
		m_buffer = NullCB();
		return E_INVALIDARG;
	}
	if( usage == Tr2ConstantUsageAL::IMMUTABLE && !initialData )
	{
		m_buffer = NullCB();
		return E_INVALIDARG;
	}
	m_buffer = std::make_shared<TrinityALImpl::Tr2ConstantBufferAL>();
	auto hr = m_buffer->Create( size, usage, initialData, renderContext );
	if( FAILED( hr ) )
	{
		m_buffer = NullCB();
	}
	return hr;
}

ALResult Tr2ConstantBufferAL::Lock( void** data, Tr2RenderContextAL& renderContext )
{
	return m_buffer->Lock( data, renderContext );
}

ALResult Tr2ConstantBufferAL::Unlock( Tr2RenderContextAL& renderContext )
{
	return m_buffer->Unlock( renderContext );
}

bool Tr2ConstantBufferAL::IsValid() const
{
	return m_buffer->IsValid();
}

uint32_t Tr2ConstantBufferAL::GetSize() const
{
	return m_buffer->GetSize();
}

Tr2ALMemoryType Tr2ConstantBufferAL::GetMemoryClass() const
{
	return m_buffer->GetMemoryClass();
}

bool Tr2ConstantBufferAL::operator==( const Tr2ConstantBufferAL& other ) const
{
	return m_buffer == other.m_buffer;
}

ALResult Tr2ConstantBufferAL::SetName( const char* name )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !name )
	{
		return E_INVALIDARG;
	}
	return m_buffer->SetName( name );
}

TrinityALImpl::Tr2ConstantBufferAL* Tr2ConstantBufferAL::TrinityALImpl_GetObject() const
{
	return m_buffer.get();
}