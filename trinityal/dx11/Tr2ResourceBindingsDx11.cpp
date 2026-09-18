// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

#include "Tr2ResourceBindingsDx11.h"

#include "Tr2BufferALDx11.h"
#include "Tr2SamplerStateALDx11.h"
#include "Tr2ShaderProgramALDx11.h"
#include "Tr2TextureALDx11.h"
#include "ALLog.h"

namespace
{
decltype( &ID3D11DeviceContext::VSSetShaderResources ) s_setResources[] = {
	&ID3D11DeviceContext::VSSetShaderResources,
	&ID3D11DeviceContext::PSSetShaderResources,
	&ID3D11DeviceContext::CSSetShaderResources,
	&ID3D11DeviceContext::GSSetShaderResources,
	&ID3D11DeviceContext::HSSetShaderResources,
	&ID3D11DeviceContext::DSSetShaderResources,
};

decltype( &ID3D11DeviceContext::VSSetSamplers ) s_setSamplers[] = {
	&ID3D11DeviceContext::VSSetSamplers,
	&ID3D11DeviceContext::PSSetSamplers,
	&ID3D11DeviceContext::CSSetSamplers,
	&ID3D11DeviceContext::GSSetSamplers,
	&ID3D11DeviceContext::HSSetSamplers,
	&ID3D11DeviceContext::DSSetSamplers,
};

}

// --------------------------------------------------------------------------------------
Tr2ResourceBindings::Tr2ResourceBindings() :
	m_assignedUavOffset( 0 ),
	m_assignedUavCount( 0 ),
	m_assignedPsUavs( false ),
	m_program( nullptr ),
	m_committed( false ),
	m_sealed( false )
{
	memset( m_boundSrvs, 0, sizeof( m_boundSrvs ) );
	memset( m_boundSamplers, 0, sizeof( m_boundSamplers ) );
}

// --------------------------------------------------------------------------------------
void Tr2ResourceBindings::Clear() throw()
{
	if( !m_program )
	{
		return;
	}
	const auto& registerMap = m_program->GetRegisterMap();
	std::fill( m_srvs, m_srvs + registerMap.srvCount, Resource() );
	std::fill( m_uavs, m_uavs + registerMap.uavCount, Resource() );
	std::fill( m_samplers, m_samplers + registerMap.samplerCount, Sampler() );
}

// --------------------------------------------------------------------------------------
void Tr2ResourceBindings::UnbindUnorderedAccessViews( ID3D11DeviceContext* context ) throw()
{
	if( !m_assignedUavCount )
	{
		return;
	}

	ID3D11UnorderedAccessView* nullUavs[Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE] = {};
	if( m_assignedPsUavs )
	{
		context->OMSetRenderTargetsAndUnorderedAccessViews(
			D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL,
			nullptr,
			nullptr,
			m_assignedUavOffset,
			m_assignedUavCount,
			nullUavs,
			nullptr );
	}
	else
	{
		context->CSSetUnorderedAccessViews( m_assignedUavOffset, m_assignedUavCount, nullUavs, nullptr );
	}
	m_assignedUavCount = 0;
	m_assignedPsUavs = false;
}

// --------------------------------------------------------------------------------------
void Tr2ResourceBindings::SetProgram( const TrinityALImpl::Tr2ShaderProgramAL* program ) throw()
{
	if( program == m_program )
	{
		return;
	}
	Clear();
	m_program = program;
	m_committed = false;
}

// --------------------------------------------------------------------------------------
void Tr2ResourceBindings::BeginBatch() throw()
{
	if( !m_sealed )
	{
		return;
	}

	m_sealed = false;
	m_committed = false;

	Clear();
}

// --------------------------------------------------------------------------------------
void Tr2ResourceBindings::Discard() throw()
{
	Clear();

	memset( m_boundSrvs, 0, sizeof( m_boundSrvs ) );
	memset( m_boundSamplers, 0, sizeof( m_boundSamplers ) );
	m_assignedUavOffset = 0;
	m_assignedUavCount = 0;
	m_assignedPsUavs = false;

	m_program = nullptr;
	m_committed = false;
	m_sealed = false;
}

// --------------------------------------------------------------------------------------
ALResult Tr2ResourceBindings::Reset() throw()
{
	Clear();

	m_committed = false;
	m_sealed = false;
	return S_OK;
}

// --------------------------------------------------------------------------------------
void Tr2ResourceBindings::ClearBoundSamplers() throw()
{
	memset( m_boundSamplers, 0, sizeof( m_boundSamplers ) );
}

// --------------------------------------------------------------------------------------
void Tr2ResourceBindings::UnbindShaderResources( ID3D11DeviceContext* context, bool unbindUavs ) throw()
{
	ID3D11ShaderResourceView* nullSrvs[Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE] = {};
	for( uint32_t i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
	{
		if( memcmp( m_boundSrvs[i], nullSrvs, sizeof( nullSrvs ) ) != 0 )
		{
			( context->*( s_setResources[i] ) )( 0, Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE, nullSrvs );
			memset( m_boundSrvs[i], 0, sizeof( m_boundSrvs[i] ) );
		}
	}

	if( unbindUavs )
	{
		UnbindUnorderedAccessViews( context );
	}

	m_committed = false;
}

// --------------------------------------------------------------------------------------
Tr2ResourceBindings::Resource* Tr2ResourceBindings::GetSrvSlot( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( !m_program )
	{
		return nullptr;
	}
	BeginBatch();
	m_committed = false;

	const auto& registerMap = m_program->GetRegisterMap();
	uint32_t index = registerMap.srvs[stage][registerIndex];
	return index < registerMap.srvCount ? &m_srvs[index] : nullptr;
}

// --------------------------------------------------------------------------------------
Tr2ResourceBindings::Resource* Tr2ResourceBindings::GetUavSlot( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( !m_program )
	{
		return nullptr;
	}
	BeginBatch();
	m_committed = false;

	const auto& registerMap = m_program->GetRegisterMap();
	uint32_t index = registerMap.uavs[stage][registerIndex];
	return index < registerMap.uavCount ? &m_uavs[index] : nullptr;
}

// --------------------------------------------------------------------------------------
Tr2ResourceBindings::Sampler* Tr2ResourceBindings::GetSamplerSlot( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( !m_program )
	{
		return nullptr;
	}
	BeginBatch();
	m_committed = false;

	const auto& registerMap = m_program->GetRegisterMap();
	uint32_t index = registerMap.samplers[stage][registerIndex];
	return index < registerMap.samplerCount ? &m_samplers[index] : nullptr;
}

// --------------------------------------------------------------------------------------
ALResult Tr2ResourceBindings::SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSrvSlot( stage, registerIndex ) )
	{
		slot->type = Resource::BUFFER;
		slot->buffer = buffer;
		slot->texture = Tr2TextureAL();
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
ALResult Tr2ResourceBindings::SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSrvSlot( stage, registerIndex ) )
	{
		slot->type = Resource::TEXTURE;
		slot->texture = texture;
		slot->buffer = Tr2BufferAL();
		slot->colorSpace = colorSpace;
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
ALResult Tr2ResourceBindings::SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetUavSlot( stage, registerIndex ) )
	{
		slot->type = Resource::BUFFER;
		slot->buffer = buffer;
		slot->texture = Tr2TextureAL();
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
ALResult Tr2ResourceBindings::SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, uint32_t mip ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetUavSlot( stage, registerIndex ) )
	{
		slot->type = Resource::TEXTURE;
		slot->texture = texture;
		slot->buffer = Tr2BufferAL();
		slot->mip = mip;
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
ALResult Tr2ResourceBindings::SetSrvHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSrvSlot( stage, registerIndex ) )
	{
		*slot = Resource();
		slot->type = Resource::HEAP_VIEW;
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
ALResult Tr2ResourceBindings::SetUavHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetUavSlot( stage, registerIndex ) )
	{
		*slot = Resource();
		slot->type = Resource::HEAP_VIEW;
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
ALResult Tr2ResourceBindings::SetSamplerHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSamplerSlot( stage, registerIndex ) )
	{
		slot->type = Sampler::HEAP_VIEW;
		slot->sampler = Tr2SamplerStateAL();
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
ALResult Tr2ResourceBindings::SetSampler( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSamplerSlot( stage, registerIndex ) )
	{
		slot->type = Sampler::SAMPLER;
		slot->sampler = sampler;
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
ALResult Tr2ResourceBindings::Commit( ID3D11DeviceContext* context ) throw()
{
	if( !m_program )
	{
		return E_INVALIDCALL;
	}
	if( m_committed )
	{
		return S_OK;
	}

	const auto& registerMap = m_program->GetRegisterMap();

	ID3D11ShaderResourceView* desiredSrvs[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE] = {};
	ID3D11SamplerState* desiredSamplers[Tr2RenderContextEnum::SHADER_TYPE_COUNT][Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE] = {};
	ID3D11UnorderedAccessView* desiredUavs[Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE] = {};
	uint32_t uavBegin = Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE;
	uint32_t uavEnd = 0;
	bool csUavs = false;

	for( uint32_t stageIndex = 0; stageIndex < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++stageIndex )
	{
		for( uint32_t registerIndex = 0; registerIndex < Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE; ++registerIndex )
		{
			uint32_t srvIndex = registerMap.srvs[stageIndex][registerIndex];
			if( srvIndex < registerMap.srvCount )
			{
				const Resource& resource = m_srvs[srvIndex];
				switch( resource.type )
				{
				case Resource::TEXTURE:
					if( resource.texture.IsValid() )
					{
						desiredSrvs[stageIndex][registerIndex] = resource.texture.TrinityALImpl_GetObject()->m_view[resource.colorSpace];
					}
					break;
				case Resource::BUFFER:
					if( resource.buffer.IsValid() )
					{
						desiredSrvs[stageIndex][registerIndex] = resource.buffer.TrinityALImpl_GetObject()->m_srv;
					}
					break;
				default:
					break;
				}
			}

			uint32_t samplerIndex = registerMap.samplers[stageIndex][registerIndex];
			if( samplerIndex < registerMap.samplerCount )
			{
				const Sampler& sampler = m_samplers[samplerIndex];
				if( sampler.type == Sampler::SAMPLER && sampler.sampler.TrinityALImpl_GetObject() )
				{
					desiredSamplers[stageIndex][registerIndex] = sampler.sampler.TrinityALImpl_GetObject()->m_samplerState;
				}
			}

			uint32_t uavIndex = registerMap.uavs[stageIndex][registerIndex];
			if( uavIndex < registerMap.uavCount &&
				( stageIndex == Tr2RenderContextEnum::PIXEL_SHADER || stageIndex == Tr2RenderContextEnum::COMPUTE_SHADER ) )
			{
				const Resource& resource = m_uavs[uavIndex];
				switch( resource.type )
				{
				case Resource::TEXTURE: {
					auto* texture = resource.texture.TrinityALImpl_GetObject();
					if( resource.texture.IsValid() && resource.mip < texture->m_uav.size() )
					{
						desiredUavs[registerIndex] = texture->m_uav[resource.mip];
					}
					break;
				}
				case Resource::BUFFER:
					if( resource.buffer.IsValid() )
					{
						desiredUavs[registerIndex] = resource.buffer.TrinityALImpl_GetObject()->m_uav;
					}
					break;
				default:
					CCP_AL_LOGWARN_LIMITED( "Missing UAV resource binding for register %u, stage %u", registerIndex, stageIndex );
					break;
				}
				uavBegin = std::min( uavBegin, registerIndex );
				uavEnd = std::max( uavEnd, registerIndex + 1 );
				csUavs = stageIndex == Tr2RenderContextEnum::COMPUTE_SHADER;
			}
		}
	}

	const bool haveUavs = uavEnd > uavBegin;

	if( !haveUavs )
	{
		UnbindUnorderedAccessViews( context );
	}

	if( haveUavs )
	{
		// The resources entering UAV slots may still be bound as SRVs from an earlier draw;
		// release every SRV slot first so the runtime doesn't have to force-unbind them.
		UnbindShaderResources( context, false );

		if( csUavs )
		{
			if( m_assignedUavCount && m_assignedPsUavs )
			{
				ID3D11UnorderedAccessView* nullUavs[Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE] = {};
				context->OMSetRenderTargetsAndUnorderedAccessViews(
					D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL,
					nullptr,
					nullptr,
					m_assignedUavOffset,
					m_assignedUavCount,
					nullUavs,
					nullptr );
				m_assignedUavCount = 0;
			}
			uint32_t begin = m_assignedUavCount ? std::min( uavBegin, m_assignedUavOffset ) : uavBegin;
			uint32_t end = m_assignedUavCount ? std::max( uavEnd, m_assignedUavOffset + m_assignedUavCount ) : uavEnd;
			context->CSSetUnorderedAccessViews( begin, end - begin, desiredUavs + begin, nullptr );
			m_assignedUavOffset = uavBegin;
			m_assignedUavCount = uavEnd - uavBegin;
			m_assignedPsUavs = false;
		}
		else
		{
			if( m_assignedUavCount && !m_assignedPsUavs )
			{
				ID3D11UnorderedAccessView* nullUavs[Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE] = {};
				context->CSSetUnorderedAccessViews( m_assignedUavOffset, m_assignedUavCount, nullUavs, nullptr );
				m_assignedUavCount = 0;
			}
			uint32_t begin = m_assignedUavCount ? std::min( uavBegin, m_assignedUavOffset ) : uavBegin;
			uint32_t end = m_assignedUavCount ? std::max( uavEnd, m_assignedUavOffset + m_assignedUavCount ) : uavEnd;
			context->OMSetRenderTargetsAndUnorderedAccessViews(
				D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL,
				nullptr,
				nullptr,
				begin,
				end - begin,
				desiredUavs + begin,
				nullptr );
			m_assignedUavOffset = uavBegin;
			m_assignedUavCount = uavEnd - uavBegin;
			m_assignedPsUavs = true;
		}
	}

	for( uint32_t stageIndex = 0; stageIndex < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++stageIndex )
	{
		if( memcmp( desiredSrvs[stageIndex], m_boundSrvs[stageIndex], sizeof( desiredSrvs[stageIndex] ) ) != 0 )
		{
			( context->*( s_setResources[stageIndex] ) )( 0, Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE, desiredSrvs[stageIndex] );
			memcpy( m_boundSrvs[stageIndex], desiredSrvs[stageIndex], sizeof( desiredSrvs[stageIndex] ) );
		}
		// Sampler slots are capped at 16 in D3D11 (D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT); binding more is invalid.
		if( memcmp( desiredSamplers[stageIndex], m_boundSamplers[stageIndex], D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT * sizeof( desiredSamplers[stageIndex][0] ) ) != 0 )
		{
			( context->*( s_setSamplers[stageIndex] ) )( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, desiredSamplers[stageIndex] );
			memcpy( m_boundSamplers[stageIndex], desiredSamplers[stageIndex], D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT * sizeof( desiredSamplers[stageIndex][0] ) );
		}
	}

	m_committed = true;
	m_sealed = true;

	return S_OK;
}

#endif
