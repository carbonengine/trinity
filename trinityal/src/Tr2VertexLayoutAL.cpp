// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "../include/Tr2VertexLayoutAL.h"
#include TRINITY_AL_PLATFORM_INCLUDE( Tr2VertexLayoutAL )

namespace
{
std::shared_ptr<TrinityALImpl::Tr2VertexLayoutAL> NullLayout()
{
	static std::shared_ptr<TrinityALImpl::Tr2VertexLayoutAL> nullLayout = std::make_shared<TrinityALImpl::Tr2VertexLayoutAL>();
	return nullLayout;
}
}

Tr2VertexLayoutAL::Tr2VertexLayoutAL() :
	m_layout( NullLayout() )
{
}

ALResult Tr2VertexLayoutAL::Create( const Tr2VertexDefinition& definition, Tr2PrimaryRenderContextAL& renderContext )
{
	m_layout = std::make_shared<TrinityALImpl::Tr2VertexLayoutAL>();
	auto result = m_layout->Create( definition, renderContext );
	if( FAILED( result ) )
	{
		m_layout = NullLayout();
	}
	return result;
}

bool Tr2VertexLayoutAL::IsValid() const
{
	return m_layout->IsValid();
}

bool Tr2VertexLayoutAL::operator==( const Tr2VertexLayoutAL& other ) const
{
	return m_layout == other.m_layout;
}

Tr2ALMemoryType Tr2VertexLayoutAL::GetMemoryClass() const
{
	return m_layout->GetMemoryClass();
}

ALResult Tr2VertexLayoutAL::SetName( const char* name )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !name )
	{
		return E_INVALIDARG;
	}
	return m_layout->SetName( name );
}
