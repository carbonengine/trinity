// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "../include/Tr2StaticResourceBindingsAL.h"
#include "../include/Tr2RenderContextAL.h"


bool Tr2StaticResourceBindingsAL::SetSampler( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler )
{
	return Set( KIND_SAMPLER, stage, registerIndex, sampler );
}

bool Tr2StaticResourceBindingsAL::SetSrvHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex )
{
	return Set( KIND_SRV_HEAP_VIEW, stage, registerIndex, Tr2SamplerStateAL() );
}

bool Tr2StaticResourceBindingsAL::SetUavHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex )
{
	return Set( KIND_UAV_HEAP_VIEW, stage, registerIndex, Tr2SamplerStateAL() );
}

bool Tr2StaticResourceBindingsAL::SetSamplerHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex )
{
	return Set( KIND_SAMPLER_HEAP_VIEW, stage, registerIndex, Tr2SamplerStateAL() );
}

void Tr2StaticResourceBindingsAL::Clear()
{
	m_entries.clear();
}

bool Tr2StaticResourceBindingsAL::IsEmpty() const
{
	return m_entries.empty();
}

bool Tr2StaticResourceBindingsAL::operator==( const Tr2StaticResourceBindingsAL& other ) const
{
	if( m_entries.size() != other.m_entries.size() )
	{
		return false;
	}
	for( size_t i = 0; i < m_entries.size(); ++i )
	{
		auto& entry = m_entries[i];
		auto& otherEntry = other.m_entries[i];
		if( entry.kind != otherEntry.kind || entry.stage != otherEntry.stage || entry.registerIndex != otherEntry.registerIndex )
		{
			return false;
		}
		if( !( entry.sampler == otherEntry.sampler ) )
		{
			return false;
		}
	}
	return true;
}

ALResult Tr2StaticResourceBindingsAL::Apply( Tr2RenderContextAL& renderContext ) const
{
	for( const auto& entry : m_entries )
	{
		auto stage = Tr2RenderContextEnum::ShaderType( entry.stage );
		switch( entry.kind )
		{
		case KIND_SAMPLER:
			CR_RETURN_HR( renderContext.SetSampler( stage, entry.registerIndex, entry.sampler ) );
			break;
		case KIND_SRV_HEAP_VIEW:
			CR_RETURN_HR( renderContext.SetSrvHeapView( stage, entry.registerIndex ) );
			break;
		case KIND_UAV_HEAP_VIEW:
			CR_RETURN_HR( renderContext.SetUavHeapView( stage, entry.registerIndex ) );
			break;
		case KIND_SAMPLER_HEAP_VIEW:
			CR_RETURN_HR( renderContext.SetSamplerHeapView( stage, entry.registerIndex ) );
			break;
		}
	}
	return S_OK;
}

bool Tr2StaticResourceBindingsAL::SharesRegisterSpace( Kind a, Kind b )
{
	if( a == b )
	{
		return true;
	}
	return ( a == KIND_SAMPLER || a == KIND_SAMPLER_HEAP_VIEW ) && ( b == KIND_SAMPLER || b == KIND_SAMPLER_HEAP_VIEW );
}

bool Tr2StaticResourceBindingsAL::Set( Kind kind, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler )
{
	for( auto& entry : m_entries )
	{
		if( entry.stage != uint8_t( stage ) || entry.registerIndex != uint8_t( registerIndex ) || !SharesRegisterSpace( entry.kind, kind ) )
		{
			continue;
		}
		if( entry.kind == kind && entry.sampler == sampler )
		{
			return false;
		}
		entry.kind = kind;
		entry.sampler = sampler;
		return true;
	}

	Entry entry;
	entry.sampler = sampler;
	entry.stage = uint8_t( stage );
	entry.registerIndex = uint8_t( registerIndex );
	entry.kind = kind;
	m_entries.push_back( entry );
	return true;
}
