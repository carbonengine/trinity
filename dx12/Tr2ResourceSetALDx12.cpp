////////////////////////////////////////////////////////////
//
//    Created:   March 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2ResourceSetALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Tr2ShaderProgramALDx12.h"
#include "Tr2BufferALDx12.h"
#include "Tr2TextureALDx12.h"
#include "Tr2SamplerStateALDx12.h"
#include "Utilities.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

namespace
{
	D3D12_CPU_DESCRIPTOR_HANDLE operator+( const D3D12_CPU_DESCRIPTOR_HANDLE& handle, uint32_t offset )
	{
		D3D12_CPU_DESCRIPTOR_HANDLE result = { handle.ptr + offset };
		return result;
	}
}

namespace TrinityALImpl
{
	Tr2ResourceSetAL::Tr2ResourceSetAL()
		:m_owner( nullptr ),
		m_samplerCount( 0 ),
		m_resourceCount( 0 )
	{

	}

	Tr2ResourceSetAL::~Tr2ResourceSetAL()
	{
		Destroy();
	}

	ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, const ::Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}
		if( !program.IsValid() )
		{
			return E_INVALIDARG;
		}

		if( program.m_program->m_registerMap != description.m_registerMap )
		{
			return E_INVALIDARG;
		}

		std::map<ID3D12Resource*, D3D12_RESOURCE_STATES> transitioned;

		for (auto it = begin(program.m_program->m_srvRegisters); it != end(program.m_program->m_srvRegisters); ++it)
		{
			auto& reg = *it;
			auto& resource = description.m_srv[description.m_registerMap.srvs[reg.stage][reg.index]];
			auto stateFlag = reg.stage == PIXEL_SHADER ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			m_resourceCount = std::max( reg.parameter + 1, m_resourceCount );
			switch (resource.type)
			{
			case Tr2ResourceSetDescriptionAL::TEXTURE:
				if (resource.texture.IsValid())
				{
					m_srv[reg.parameter] = resource.texture.m_texture->m_view[resource.colorSpace];

					auto texture = resource.texture.m_texture.get();
					auto res = texture->GetResourceDx12();
					auto found = transitioned.find(res);
					// TODO: verify state
					if ((texture->m_defaultState & stateFlag) == 0 && found == transitioned.end())
					{
						m_inTransitions.push_back(Transition(res, texture->m_defaultState, stateFlag));
						m_outTransitions.push_back(Transition(res, stateFlag, texture->m_defaultState));
						transitioned[res] = stateFlag;
					}
					m_usedResources.push_back( res );
				}
				break;
			case Tr2ResourceSetDescriptionAL::BUFFER:
				if (resource.buffer.IsValid())
				{
					auto buffer = resource.buffer.m_buffer.get();
					m_srv[reg.parameter] = buffer->m_srv;

					auto res = buffer->GetGpuResource();

					auto found = transitioned.find(res);
					// TODO: verify state
					if ((buffer->m_defaultState & stateFlag) == 0 && found == transitioned.end())
					{
						m_inTransitions.push_back(Transition(res, buffer->m_defaultState, stateFlag));
						m_outTransitions.push_back(Transition(res, stateFlag, buffer->m_defaultState));
						transitioned[res] = stateFlag;
					}
					m_usedResources.push_back( res );
				}
				break;
			default:
				break;
			}
		}

		for (auto it = begin(program.m_program->m_uavRegisters); it != end(program.m_program->m_uavRegisters); ++it)
		{
			auto& reg = *it;
			auto& resource = description.m_uav[description.m_registerMap.uavs[reg.stage][reg.index]];
			auto stateFlag = reg.stage == PIXEL_SHADER ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			m_resourceCount = std::max( reg.parameter + 1, m_resourceCount );
			switch (resource.type)
			{
			case Tr2ResourceSetDescriptionAL::TEXTURE:
				if (resource.texture.IsValid())
				{
					auto texture = resource.texture.m_texture.get();
					auto res = texture->GetResourceDx12();

					m_uav[reg.parameter] = texture->m_uav[resource.mip];

					auto found = transitioned.find(res);
					// TODO: verify state
					if ((texture->m_defaultState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) == 0 && found == transitioned.end())
					{
						m_inTransitions.push_back(Transition(res, texture->m_defaultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
						m_outTransitions.push_back(Transition(res, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, texture->m_defaultState));
						transitioned[res] = stateFlag;
					}
					m_usedResources.push_back( res );
				}
				break;
			case Tr2ResourceSetDescriptionAL::BUFFER:
				if (resource.buffer.IsValid())
				{
					auto buffer = resource.buffer.m_buffer.get();
					auto res = buffer->GetGpuResource();

					m_uav[reg.parameter] = buffer->m_uav;

					if (resource.initialCount != -1 && resource.buffer.m_buffer->m_counter)
					{
						InitialCount cnt = { resource.buffer, resource.initialCount };
						m_initialCounts.push_back(cnt);
					}
					auto found = transitioned.find(res);
					// TODO: verify state
					if ((buffer->m_defaultState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) == 0 && found == transitioned.end())
					{
						m_inTransitions.push_back(Transition(res, buffer->m_defaultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
						m_outTransitions.push_back(Transition(res, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, buffer->m_defaultState));
						transitioned[res] = stateFlag;
					}
					m_usedResources.push_back( res );
				}
				break;
			default:
				CCP_AL_LOGWARN("Missing UAV resource in resource set for register %u, stage %u", reg.index, reg.stage);
				break;
			}

		}

		for (auto it = begin(program.m_program->m_samplerRegisters); it != end(program.m_program->m_samplerRegisters); ++it)
		{
			auto& reg = *it;
			auto& sampler = description.m_samplers[description.m_registerMap.samplers[reg.stage][reg.index]];
			if( sampler.assigned )
			{
				m_sampler[reg.parameter] = sampler.sampler.m_sampler->m_samplerState;
			}
			m_samplerCount = std::max( reg.parameter + 1, m_samplerCount );
		}

		CComPtr<ID3D12Resource> initialCountBuffer;
		if( !m_initialCounts.empty() )
		{
			auto scratchHeap = HeapDesc( D3D12_HEAP_TYPE_UPLOAD );
			auto scratchDesc = BufferDesc( uint32_t( m_initialCounts.size() ) * 4 );
			CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
				&scratchHeap,
				D3D12_HEAP_FLAG_NONE,
				&scratchDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS( &initialCountBuffer ) ) );
			D3D12_RANGE range = { 0, 0 };
			uint32_t* data = nullptr;
			CR_RETURN_HR( initialCountBuffer->Map( 0, &range, reinterpret_cast<void**>( &data ) ) );
			for( auto it = begin( m_initialCounts ); it != end( m_initialCounts ); ++it )
			{
				*data++ = it->initialCount;
			}
			initialCountBuffer->Unmap( 0, nullptr );
		}

		m_owner = &renderContext;
		m_initialCountBuffer = initialCountBuffer;

		return S_OK;
	}

	void Tr2ResourceSetAL::Destroy()
	{
		if( m_owner && m_initialCountBuffer )
		{
			RELEASE_LATER( m_owner, m_initialCountBuffer );
			m_initialCountBuffer = nullptr;
		}
		for( uint32_t idx = 0; idx < m_resourceCount; ++idx )
		{
			m_srv[idx] = nullptr;
			m_uav[idx] = nullptr;
		}
		m_resourceCount = 0;
		for( uint32_t idx = 0; idx < m_samplerCount; ++idx )
		{
			m_sampler[idx] = nullptr;
		}
		m_samplerCount = 0;

		m_owner = nullptr;
		m_initialCounts.clear();
		m_inTransitions.clear();
		m_outTransitions.clear();
		m_usedResources.clear();
	}

	bool Tr2ResourceSetAL::IsValid() const
	{
		return m_owner != nullptr;
	}

	Tr2ALMemoryType Tr2ResourceSetAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	void Tr2ResourceSetAL::UploadInitialCounts( Tr2RenderContextAL& renderContext )
	{
		if( m_initialCounts.empty() )
		{
			return;
		}
		std::vector<D3D12_RESOURCE_BARRIER> barriers( m_initialCounts.size() );
		std::vector<ID3D12Resource*> resources( m_initialCounts.size() );
		for( size_t i = 0; i < m_initialCounts.size(); ++i )
		{
			barriers[i] = Transition( m_initialCounts[i].buffer.m_buffer->m_counter, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST );
			resources[i] = m_initialCounts[i].buffer.m_buffer->m_counter;
		}

		renderContext.ResourceBarrierDx12( barriers.size(), barriers.data() );
		renderContext.FlushBarriersDx12( resources.size(), resources.data() );

		for( size_t i = 0; i < m_initialCounts.size(); ++i )
		{
			renderContext.m_commandList->CopyBufferRegion( m_initialCounts[i].buffer.m_buffer->m_counter, 0, m_initialCountBuffer, i * 4, 4 );
		}

		for( size_t i = 0; i < m_initialCounts.size(); ++i )
		{
			std::swap( barriers[i].Transition.StateAfter, barriers[i].Transition.StateBefore );
		}

		renderContext.ResourceBarrierDx12( barriers.size(), barriers.data() );
	}

	void Tr2ResourceSetAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ResourceSetAL";
	}
}

#endif