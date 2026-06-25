// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "include/Tr2OcclusionQueryAL.h"
#include TRINITY_AL_PLATFORM_INCLUDE( Tr2OcclusionQueryAL )

namespace
{

std::shared_ptr<TrinityALImpl::Tr2OcclusionQueryAL> NullQuery()
{
	static std::shared_ptr<TrinityALImpl::Tr2OcclusionQueryAL> nullQuery = std::make_shared<TrinityALImpl::Tr2OcclusionQueryAL>();
	return nullQuery;
}
}


Tr2OcclusionQueryAL::Tr2OcclusionQueryAL() :
	m_query( NullQuery() )
{
}

ALResult Tr2OcclusionQueryAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	m_query = std::make_shared<TrinityALImpl::Tr2OcclusionQueryAL>();
	auto result = m_query->Create( renderContext );
	if( FAILED( result ) )
	{
		m_query = NullQuery();
	}
	return result;
}

ALResult Tr2OcclusionQueryAL::Begin( Tr2RenderContextAL& renderContext )
{
	return m_query->Begin( renderContext );
}

ALResult Tr2OcclusionQueryAL::End( Tr2RenderContextAL& renderContext )
{
	return m_query->End( renderContext );
}

ALResult Tr2OcclusionQueryAL::GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, WaitMode waitMode )
{
	return m_query->GetPixelCount( renderContext, count, waitMode );
}

bool Tr2OcclusionQueryAL::IsValid() const
{
	return m_query->IsValid();
}

bool Tr2OcclusionQueryAL::operator==( const Tr2OcclusionQueryAL& other ) const
{
	return m_query == other.m_query;
}

Tr2ALMemoryType Tr2OcclusionQueryAL::GetMemoryClass() const
{
	return m_query->GetMemoryClass();
}

ALResult Tr2OcclusionQueryAL::SetName( const char* name )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !name )
	{
		return E_INVALIDARG;
	}
	return m_query->SetName( name );
}
