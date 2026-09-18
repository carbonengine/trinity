// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "ResourceBindingsDx12.h"

#include "../Tr2BufferALDx12.h"
#include "../Tr2PrimaryRenderContextDx12.h"
#include "../Tr2RenderContextDx12.h"
#include "../Tr2SamplerStateALDx12.h"
#include "../Tr2ShaderProgramALDx12.h"
#include "../Tr2TextureALDx12.h"
#include "../Utilities.h"
#include "ALLog.h"

ResourceBindings::ResourceBindings() :
	m_rootSignature( nullptr ),
	m_committed( false ),
	m_sealed( false )
{
	m_outTransitions.reserve( Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE );
	m_usedResources.reserve( Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE );
}

void ResourceBindings::Clear() throw()
{
	if( !m_rootSignature )
	{
		return;
	}
	auto& registerMap = m_rootSignature->m_registerMap;
	std::fill( m_srvs, m_srvs + registerMap.srvCount, Resource() );
	std::fill( m_uavs, m_uavs + registerMap.uavCount, Resource() );
	std::fill( m_samplers, m_samplers + registerMap.samplerCount, Sampler() );
}

void ResourceBindings::FlushOutTransitions( Tr2RenderContextAL& context ) throw()
{
	if( !m_outTransitions.empty() )
	{
		context.ResourceBarrierDx12( m_outTransitions.size(), m_outTransitions.data() );
		m_outTransitions.clear();
	}
	m_usedResources.clear();
}

void ResourceBindings::SetRootSignature( const TrinityALImpl::Tr2RootSignatureAL* rootSignature ) throw()
{
	if( rootSignature == m_rootSignature )
	{
		return;
	}
	Clear();
	m_rootSignature = rootSignature;
	m_committed = false;
}

void ResourceBindings::BeginBatch( Tr2RenderContextAL& context ) throw()
{
	if( !m_sealed )
	{
		return;
	}

	m_sealed = false;
	m_committed = false;

	Clear();
	FlushOutTransitions( context );
}

void ResourceBindings::Discard() throw()
{
	Clear();

	m_outTransitions.clear();
	m_usedResources.clear();

	m_rootSignature = nullptr;
	m_committed = false;
	m_sealed = false;
}

ALResult ResourceBindings::Reset( Tr2RenderContextAL& context ) throw()
{
	Clear();
	FlushOutTransitions( context );

	m_committed = false;
	m_sealed = false;
	return S_OK;
}

ResourceBindings::Resource* ResourceBindings::GetSrvSlot( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( !m_rootSignature )
	{
		return nullptr;
	}
	BeginBatch( context );
	m_committed = false;

	auto& registerMap = m_rootSignature->m_registerMap;
	uint32_t index = registerMap.srvs[stage][registerIndex];
	return index < registerMap.srvCount ? &m_srvs[index] : nullptr;
}

ResourceBindings::Resource* ResourceBindings::GetUavSlot( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( !m_rootSignature )
	{
		return nullptr;
	}
	BeginBatch( context );
	m_committed = false;

	auto& registerMap = m_rootSignature->m_registerMap;
	uint32_t index = registerMap.uavs[stage][registerIndex];
	return index < registerMap.uavCount ? &m_uavs[index] : nullptr;
}

ResourceBindings::Sampler* ResourceBindings::GetSamplerSlot( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( !m_rootSignature )
	{
		return nullptr;
	}
	BeginBatch( context );
	m_committed = false;

	auto& registerMap = m_rootSignature->m_registerMap;
	uint32_t index = registerMap.samplers[stage][registerIndex];
	return index < registerMap.samplerCount ? &m_samplers[index] : nullptr;
}

ALResult ResourceBindings::SetSrv( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSrvSlot( context, stage, registerIndex ) )
	{
		slot->type = Resource::BUFFER;
		slot->buffer = buffer;
		slot->texture = Tr2TextureAL();
	}
	return S_OK;
}

ALResult ResourceBindings::SetSrv( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSrvSlot( context, stage, registerIndex ) )
	{
		slot->type = Resource::TEXTURE;
		slot->texture = texture;
		slot->buffer = Tr2BufferAL();
		slot->colorSpace = colorSpace;
	}
	return S_OK;
}

ALResult ResourceBindings::SetUav( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetUavSlot( context, stage, registerIndex ) )
	{
		slot->type = Resource::BUFFER;
		slot->buffer = buffer;
		slot->texture = Tr2TextureAL();
	}
	return S_OK;
}

ALResult ResourceBindings::SetUav( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, uint32_t mip ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetUavSlot( context, stage, registerIndex ) )
	{
		slot->type = Resource::TEXTURE;
		slot->texture = texture;
		slot->buffer = Tr2BufferAL();
		slot->mip = mip;
	}
	return S_OK;
}

ALResult ResourceBindings::SetSrvHeapView( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSrvSlot( context, stage, registerIndex ) )
	{
		*slot = Resource();
		slot->type = Resource::HEAP_VIEW;
	}
	return S_OK;
}

ALResult ResourceBindings::SetUavHeapView( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetUavSlot( context, stage, registerIndex ) )
	{
		*slot = Resource();
		slot->type = Resource::HEAP_VIEW;
	}
	return S_OK;
}

ALResult ResourceBindings::SetSampler( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSamplerSlot( context, stage, registerIndex ) )
	{
		slot->type = Sampler::SAMPLER;
		slot->sampler = sampler;
	}
	return S_OK;
}

ALResult ResourceBindings::SetSamplerHeapView( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	if( auto* slot = GetSamplerSlot( context, stage, registerIndex ) )
	{
		slot->type = Sampler::HEAP_VIEW;
		slot->sampler = Tr2SamplerStateAL();
	}
	return S_OK;
}

ALResult ResourceBindings::Commit( Tr2RenderContextAL& context ) throw()
{
	if( !m_rootSignature )
	{
		return E_INVALIDCALL;
	}
	if( m_committed )
	{
		return S_OK;
	}

	Tr2PrimaryRenderContextAL& renderContext = context.GetPrimaryRenderContext();
	const TrinityALImpl::Tr2RootSignatureAL& rootSignature = *m_rootSignature;
	auto& registerMap = rootSignature.m_registerMap;

	FlushOutTransitions( context );

	constexpr uint32_t maxTransitions = 2 * Tr2RenderContextEnum::SHADER_TYPE_COUNT * Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE;
	D3D12_RESOURCE_BARRIER inTransitions[maxTransitions];
	ID3D12Resource* transitioned[maxTransitions];
	uint32_t inCount = 0;
	uint32_t transitionedCount = 0;

	auto AddTransition = [&]( ID3D12Resource* res, D3D12_RESOURCE_STATES defaultState, D3D12_RESOURCE_STATES expectedState ) {
		// TODO: verify state
		if( ( defaultState & expectedState ) == 0 && defaultState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE )
		{
			auto found = std::find( transitioned, transitioned + transitionedCount, res );
			if( found == transitioned + transitionedCount && transitionedCount < maxTransitions )
			{
				inTransitions[inCount++] = TrinityALImpl::Transition( res, defaultState, expectedState );
				m_outTransitions.push_back( TrinityALImpl::Transition( res, expectedState, defaultState ) );
				transitioned[transitionedCount++] = res;
			}
		}
		m_usedResources.push_back( res );
	};

	uint32_t bufferIndex = renderContext.GetCurrentBackBufferIndex();
	auto& descriptorCache = context.m_descriptorCache[bufferIndex];

	for( const auto& reg : rootSignature.m_srvRegisters )
	{
		uint32_t mapIndex = registerMap.srvs[reg.stage][reg.index];
		const Resource* resource = mapIndex < registerMap.srvCount ? &m_srvs[mapIndex] : nullptr;
		auto stateFlag = reg.stage == Tr2RenderContextEnum::PIXEL_SHADER ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

		std::shared_ptr<ShaderResourceViewDx12> srv;
		switch( resource ? resource->type : Resource::NONE )
		{
		case Resource::TEXTURE: {
			auto* texture = resource->texture.TrinityALImpl_GetObject();
			if( resource->texture.IsValid() && reg.registerType >= Tr2ShaderRegisterAL::SRV_TEXTURE1D )
			{
				srv = texture->m_view[resource->colorSpace];
			}
			if( !srv )
			{
				srv = renderContext.GetNullSrvDx12( reg.registerType );
			}
			else
			{
				AddTransition( texture->GetResourceDx12(), texture->m_defaultState, stateFlag );
			}
			break;
		}
		case Resource::BUFFER: {
			auto* buffer = resource->buffer.TrinityALImpl_GetObject();
			if( resource->buffer.IsValid() && reg.registerType <= Tr2ShaderRegisterAL::SRV_STRUCTURED_BUFFER )
			{
				srv = buffer->m_srv;
			}
			if( !srv )
			{
				srv = renderContext.GetNullSrvDx12( reg.registerType );
			}
			else
			{
				AddTransition( buffer->GetGpuResource(), buffer->m_defaultState, stateFlag );
			}
			break;
		}
		case Resource::HEAP_VIEW:
			srv = renderContext.GetSrvHeapView();
			break;
		default:
			srv = renderContext.GetNullSrvDx12( reg.registerType );
			break;
		}

		if( srv )
		{
			descriptorCache->SetShaderResources( reg.parameter, 1, &srv );
		}
	}

	for( const auto& reg : rootSignature.m_uavRegisters )
	{
		uint32_t mapIndex = registerMap.uavs[reg.stage][reg.index];
		const Resource* resource = mapIndex < registerMap.uavCount ? &m_uavs[mapIndex] : nullptr;

		std::shared_ptr<UnorderedAccessViewDx12> uav;
		switch( resource ? resource->type : Resource::NONE )
		{
		case Resource::TEXTURE: {
			auto* texture = resource->texture.TrinityALImpl_GetObject();
			if( resource->texture.IsValid() && reg.registerType >= Tr2ShaderRegisterAL::UAV_TEXTURE1D && resource->mip < texture->m_uav.size() )
			{
				uav = texture->m_uav[resource->mip];
			}
			if( uav )
			{
				AddTransition( texture->GetResourceDx12(), texture->m_defaultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
			}
			break;
		}
		case Resource::BUFFER: {
			auto* buffer = resource->buffer.TrinityALImpl_GetObject();
			if( resource->buffer.IsValid() && reg.registerType <= Tr2ShaderRegisterAL::UAV_STRUCTURED_BUFFER )
			{
				uav = buffer->m_uav;
			}
			if( uav )
			{
				AddTransition( buffer->GetGpuResource(), buffer->m_defaultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
			}
			break;
		}
		case Resource::HEAP_VIEW:
			uav = renderContext.GetUavHeapView();
			break;
		default:
			CCP_AL_LOGWARN_LIMITED( "Missing UAV resource binding for register %u, stage %u", reg.index, reg.stage );
			break;
		}

		if( !uav )
		{
			uav = renderContext.GetNullUavDx12( reg.registerType );
		}
		descriptorCache->SetUnorderedAccessViews( reg.parameter, 1, &uav );
	}

	std::shared_ptr<SamplerStateDx12> samplers[Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE];
	uint32_t samplerCount = 0;

	for( const auto& reg : rootSignature.m_samplerRegisters )
	{
		uint32_t mapIndex = registerMap.samplers[reg.stage][reg.index];
		const Sampler* sampler = mapIndex < registerMap.samplerCount ? &m_samplers[mapIndex] : nullptr;

		switch( sampler ? sampler->type : Sampler::NONE )
		{
		case Sampler::SAMPLER:
			samplers[reg.parameter] = sampler->sampler.IsValid() ? sampler->sampler.TrinityALImpl_GetObject()->m_samplerState : renderContext.GetNullSamplerDx12();
			break;
		case Sampler::HEAP_VIEW:
			samplers[reg.parameter] = renderContext.GetSamplerHeapView();
			break;
		default:
			samplers[reg.parameter] = renderContext.GetNullSamplerDx12();
			break;
		}
		samplerCount = std::max( reg.parameter + 1, samplerCount );
	}

	descriptorCache->SetSamplers( 0, samplerCount, samplers );

	if( inCount )
	{
		context.ResourceBarrierDx12( inCount, inTransitions );
	}

	m_committed = true;
	m_sealed = true;
	return S_OK;
}

#endif
