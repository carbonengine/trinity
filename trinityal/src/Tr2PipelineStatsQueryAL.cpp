#include "StdAfx.h"
#include "include/Tr2PipelineStatsQueryAL.h"


namespace
{

std::shared_ptr<TrinityALImpl::Tr2PipelineStatsQueryAL> NullQuery()
{
	static std::shared_ptr<TrinityALImpl::Tr2PipelineStatsQueryAL> nullQuery = std::make_shared<TrinityALImpl::Tr2PipelineStatsQueryAL>();
	return nullQuery;
}
}



Tr2PipelineStatsQueryAL::Tr2PipelineStatsQueryAL() :
	m_query( NullQuery() )
{
}

ALResult Tr2PipelineStatsQueryAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	m_query = std::make_shared<TrinityALImpl::Tr2PipelineStatsQueryAL>();
	auto result = m_query->Create( renderContext );
	if( FAILED( result ) )
	{
		m_query = NullQuery();
	}
	return result;
}

ALResult Tr2PipelineStatsQueryAL::Begin( Tr2RenderContextAL& renderContext )
{
	return m_query->Begin( renderContext );
}

ALResult Tr2PipelineStatsQueryAL::End( Tr2RenderContextAL& renderContext )
{
	return m_query->End( renderContext );
}

ALResult Tr2PipelineStatsQueryAL::GetStats( Tr2PipelineStatsDataAL& data, Tr2RenderContextAL& renderContext )
{
	return m_query->GetStats( data, renderContext );
}

bool Tr2PipelineStatsQueryAL::IsValid() const
{
	return m_query->IsValid();
}

bool Tr2PipelineStatsQueryAL::operator==( const Tr2PipelineStatsQueryAL& other ) const
{
	return m_query == other.m_query;
}

Tr2ALMemoryType Tr2PipelineStatsQueryAL::GetMemoryClass() const
{
	return m_query->GetMemoryClass();
}

size_t Tr2PipelineStatsQueryAL::GetValueCount( const Tr2PipelineStatsDataAL& data )
{
	return TrinityALImpl::Tr2PipelineStatsQueryAL::GetValueCount( data );
}

const char* Tr2PipelineStatsQueryAL::GetLabel( const Tr2PipelineStatsDataAL& data, size_t index )
{
	return TrinityALImpl::Tr2PipelineStatsQueryAL::GetLabel( data, index );
}

const char* Tr2PipelineStatsQueryAL::GetDescription( const Tr2PipelineStatsDataAL& data, size_t index )
{
	return TrinityALImpl::Tr2PipelineStatsQueryAL::GetDescription( data, index );
}

Tr2PipelineStatsQueryAL::Value Tr2PipelineStatsQueryAL::GetValue( const Tr2PipelineStatsDataAL& data, size_t index )
{
	return TrinityALImpl::Tr2PipelineStatsQueryAL::GetValue( data, index );
}

ALResult Tr2PipelineStatsQueryAL::SetName( const char* name )
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
