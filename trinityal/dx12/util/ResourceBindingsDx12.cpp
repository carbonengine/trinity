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
	m_committed( false ),
	m_sealed( false ),
	m_committedRootSignature( nullptr )
{
	m_pendingSRVs.reserve( Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE );
	m_pendingUAVs.reserve( Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE );
	m_pendingSamplers.reserve( Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE );
	m_outTransitions.reserve( Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE );
	m_usedResources.reserve( Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE );

	ClearSorted();
}

void ResourceBindings::ClearSorted() throw()
{
	std::fill( std::begin( m_sortedSRVs ), std::end( m_sortedSRVs ), nullptr );
	std::fill( std::begin( m_sortedUAVs ), std::end( m_sortedUAVs ), nullptr );
	std::fill( std::begin( m_sortedSamplers ), std::end( m_sortedSamplers ), nullptr );
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

void ResourceBindings::BeginBatch( Tr2RenderContextAL& context ) throw()
{
	if( !m_sealed )
	{
		return;
	}

	m_sealed = false;
	m_committed = false;

	m_pendingSRVs.clear();
	m_pendingUAVs.clear();
	m_pendingSamplers.clear();

	FlushOutTransitions( context );
}

void ResourceBindings::Discard() throw()
{
	m_pendingSRVs.clear();
	m_pendingUAVs.clear();
	m_pendingSamplers.clear();

	ClearSorted();

	m_outTransitions.clear();
	m_usedResources.clear();

	m_committed = false;
	m_sealed = false;
	m_committedRootSignature = nullptr;
}

void ResourceBindings::Invalidate() throw()
{
	m_committed = false;
}

ALResult ResourceBindings::Reset( Tr2RenderContextAL& context ) throw()
{
	m_pendingSRVs.clear();
	m_pendingUAVs.clear();
	m_pendingSamplers.clear();

	ClearSorted();

	FlushOutTransitions( context );

	m_committed = false;
	m_sealed = false;
	m_committedRootSignature = nullptr;
	return S_OK;
}

ALResult ResourceBindings::SetSrv( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch( context );

	Resource resource;
	resource.stage = stage;
	resource.registerIndex = registerIndex;
	resource.type = Resource::BUFFER;
	resource.buffer = buffer;
	m_pendingSRVs.push_back( resource );

	m_committed = false;
	return S_OK;
}

ALResult ResourceBindings::SetSrv( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch( context );

	Resource resource;
	resource.stage = stage;
	resource.registerIndex = registerIndex;
	resource.type = Resource::TEXTURE;
	resource.texture = texture;
	resource.colorSpace = colorSpace;
	m_pendingSRVs.push_back( resource );

	m_committed = false;
	return S_OK;
}

ALResult ResourceBindings::SetUav( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch( context );

	Resource resource;
	resource.stage = stage;
	resource.registerIndex = registerIndex;
	resource.type = Resource::BUFFER;
	resource.buffer = buffer;
	m_pendingUAVs.push_back( resource );

	m_committed = false;
	return S_OK;
}

ALResult ResourceBindings::SetUav( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, uint32_t mip ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch( context );

	Resource resource;
	resource.stage = stage;
	resource.registerIndex = registerIndex;
	resource.type = Resource::TEXTURE;
	resource.texture = texture;
	resource.mip = mip;
	m_pendingUAVs.push_back( resource );

	m_committed = false;
	return S_OK;
}

ALResult ResourceBindings::SetSrvHeapView( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch( context );

	Resource resource;
	resource.stage = stage;
	resource.registerIndex = registerIndex;
	resource.type = Resource::HEAP_VIEW;
	m_pendingSRVs.push_back( resource );

	m_committed = false;
	return S_OK;
}

ALResult ResourceBindings::SetUavHeapView( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch( context );

	Resource resource;
	resource.stage = stage;
	resource.registerIndex = registerIndex;
	resource.type = Resource::HEAP_VIEW;
	m_pendingUAVs.push_back( resource );

	m_committed = false;
	return S_OK;
}

ALResult ResourceBindings::SetSampler( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch( context );

	Sampler entry;
	entry.stage = stage;
	entry.registerIndex = registerIndex;
	entry.type = Sampler::SAMPLER;
	entry.sampler = sampler;
	m_pendingSamplers.push_back( entry );

	m_committed = false;
	return S_OK;
}

ALResult ResourceBindings::SetSamplerHeapView( Tr2RenderContextAL& context, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex ) throw()
{
	if( stage >= Tr2RenderContextEnum::SHADER_TYPE_COUNT || registerIndex >= Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE )
	{
		return E_INVALIDARG;
	}
	BeginBatch( context );

	Sampler entry;
	entry.stage = stage;
	entry.registerIndex = registerIndex;
	entry.type = Sampler::HEAP_VIEW;
	m_pendingSamplers.push_back( entry );

	m_committed = false;
	return S_OK;
}

ALResult ResourceBindings::Commit( Tr2RenderContextAL& context, const TrinityALImpl::Tr2RootSignatureAL& rootSignature ) throw()
{
	if( m_committed && m_committedRootSignature == &rootSignature )
	{
		return S_OK;
	}

	Tr2PrimaryRenderContextAL& renderContext = context.GetPrimaryRenderContext();
	auto& registerMap = rootSignature.m_registerMap;

	ClearSorted();

	for( const auto& resource : m_pendingSRVs )
	{
		uint32_t index = registerMap.srvs[resource.stage][resource.registerIndex];
		if( index < registerMap.srvCount )
		{
			m_sortedSRVs[index] = &resource;
		}
	}
	for( const auto& resource : m_pendingUAVs )
	{
		uint32_t index = registerMap.uavs[resource.stage][resource.registerIndex];
		if( index < registerMap.uavCount )
		{
			m_sortedUAVs[index] = &resource;
		}
	}
	for( const auto& sampler : m_pendingSamplers )
	{
		uint32_t index = registerMap.samplers[sampler.stage][sampler.registerIndex];
		if( index < registerMap.samplerCount )
		{
			m_sortedSamplers[index] = &sampler;
		}
	}

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
		const Resource* resource = mapIndex < registerMap.srvCount ? m_sortedSRVs[mapIndex] : nullptr;
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
		const Resource* resource = mapIndex < registerMap.uavCount ? m_sortedUAVs[mapIndex] : nullptr;

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
		const Sampler* sampler = mapIndex < registerMap.samplerCount ? m_sortedSamplers[mapIndex] : nullptr;

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
	m_committedRootSignature = &rootSignature;
	return S_OK;
}

#endif
