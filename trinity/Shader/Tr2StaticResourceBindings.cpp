// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2StaticResourceBindings.h"

bool Tr2StaticResourceBindings::SetSampler( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler )
{
	return Set( KIND_SAMPLER, stage, registerIndex, sampler );
}

bool Tr2StaticResourceBindings::SetSrvHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex )
{
	return Set( KIND_SRV_HEAP_VIEW, stage, registerIndex, Tr2SamplerStateAL() );
}

bool Tr2StaticResourceBindings::SetUavHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex )
{
	return Set( KIND_UAV_HEAP_VIEW, stage, registerIndex, Tr2SamplerStateAL() );
}

bool Tr2StaticResourceBindings::SetSamplerHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex )
{
	return Set( KIND_SAMPLER_HEAP_VIEW, stage, registerIndex, Tr2SamplerStateAL() );
}

void Tr2StaticResourceBindings::Clear()
{
	m_entries.clear();
}

bool Tr2StaticResourceBindings::IsEmpty() const
{
	return m_entries.empty();
}

bool Tr2StaticResourceBindings::operator==( const Tr2StaticResourceBindings& other ) const
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

ALResult Tr2StaticResourceBindings::Apply( Tr2RenderContextAL& renderContext ) const
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

bool Tr2StaticResourceBindings::SharesRegisterSpace( Kind a, Kind b )
{
	bool aIsSampler = a == KIND_SAMPLER || a == KIND_SAMPLER_HEAP_VIEW;
	bool bIsSampler = b == KIND_SAMPLER || b == KIND_SAMPLER_HEAP_VIEW;
	return a == b || ( aIsSampler && bIsSampler );
}

bool Tr2StaticResourceBindings::Set( Kind kind, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler )
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return false;
	}
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
