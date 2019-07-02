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
		:m_owner( nullptr )
	{

	}

	Tr2ResourceSetAL::~Tr2ResourceSetAL()
	{
		Destroy();
	}

	ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, const Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext )
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


		std::map<ID3D12Resource*, D3D12_RESOURCE_STATES> transitioned;
		uint32_t bufferIndex = renderContext.GetCurrentBackBufferIndex();

		for (auto it = begin(program.m_srvRegisters); it != end(program.m_srvRegisters); ++it)
		{
			auto& reg = *it;
			auto& resource = description.m_srv[reg.stage][reg.index];
			auto stateFlag = reg.stage == PIXEL_SHADER ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			switch (resource.type)
			{
			case Tr2ResourceSetDescriptionAL::TEXTURE:
				if (resource.texture.IsValid())
				{
					m_srv[reg.parameter] = resource.texture.m_texture->m_view[resource.colorSpace];
					m_srvUav[reg.parameter] = m_srv[reg.parameter];

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
				}
				break;
			case Tr2ResourceSetDescriptionAL::BUFFER:
				if (resource.buffer.IsValid())
				{
					auto buffer = resource.buffer.m_buffer.get();
					m_srv[reg.parameter] = buffer->m_srv;
					m_srvUav[reg.parameter] = m_srv[reg.parameter];

					auto res = buffer->GetGpuResource();

					auto found = transitioned.find(res);
					// TODO: verify state
					if ((buffer->m_defaultState & stateFlag) == 0 && found == transitioned.end())
					{
						m_inTransitions.push_back(Transition(res, buffer->m_defaultState, stateFlag));
						m_outTransitions.push_back(Transition(res, stateFlag, buffer->m_defaultState));
						transitioned[res] = stateFlag;
					}
				}
				break;
			default:
				break;
			}
		}

		for (auto it = begin(program.m_uavRegisters); it != end(program.m_uavRegisters); ++it)
		{
			auto& reg = *it;
			auto& resource = description.m_uav[reg.stage][reg.index];
			auto stateFlag = reg.stage == PIXEL_SHADER ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			switch (resource.type)
			{
			case Tr2ResourceSetDescriptionAL::TEXTURE:
				if (resource.texture.IsValid())
				{
					auto texture = resource.texture.m_texture.get();
					auto res = texture->GetResourceDx12();

					m_uav[reg.parameter] = texture->m_uav[resource.mip];
					m_srvUav[reg.parameter] = m_uav[reg.parameter];

					auto found = transitioned.find(res);
					// TODO: verify state
					if ((texture->m_defaultState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) == 0 && found == transitioned.end())
					{
						m_inTransitions.push_back(Transition(res, texture->m_defaultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
						m_outTransitions.push_back(Transition(res, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, texture->m_defaultState));
						transitioned[res] = stateFlag;
					}
				}
				break;
			case Tr2ResourceSetDescriptionAL::BUFFER:
				if (resource.buffer.IsValid())
				{
					auto buffer = resource.buffer.m_buffer.get();
					auto res = buffer->GetGpuResource();

					m_uav[reg.parameter] = buffer->m_uav;
					m_srvUav[reg.parameter] = m_uav[reg.parameter];

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
				}
				break;
			default:
				CCP_AL_LOGWARN("Missing UAV resource in resource set for register %u, stage %u", reg.index, reg.stage);
				break;
			}

		}

		for (auto it = begin(program.m_samplerRegisters); it != end(program.m_samplerRegisters); ++it)
		{
			auto& reg = *it;
			auto& sampler = description.m_samplers[reg.stage][reg.index];
			if (sampler.assigned)
			{
				m_sampler[reg.parameter] = sampler.sampler.m_sampler->m_samplerState;
			}
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
		for( uint32_t idx = 0; idx < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++idx )
		{
			m_srv[idx] = nullptr;
			m_uav[idx] = nullptr;
			m_srvUav[idx] = nullptr;
			m_sampler[idx] = nullptr;
		}

		m_owner = nullptr;
		m_initialCounts.clear();
		m_inTransitions.clear();
		m_outTransitions.clear();
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
		for( size_t i = 0; i < m_initialCounts.size(); ++i )
		{
			barriers[i] = Transition( m_initialCounts[i].buffer.m_buffer->m_counter, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST );
		}

		renderContext.m_commandList->ResourceBarrier( UINT( barriers.size() ), barriers.data() );

		for( size_t i = 0; i < m_initialCounts.size(); ++i )
		{
			renderContext.m_commandList->CopyBufferRegion( m_initialCounts[i].buffer.m_buffer->m_counter, 0, m_initialCountBuffer, i * 4, 4 );
		}

		for( size_t i = 0; i < m_initialCounts.size(); ++i )
		{
			std::swap( barriers[i].Transition.StateAfter, barriers[i].Transition.StateBefore );
		}

		renderContext.m_commandList->ResourceBarrier( UINT( barriers.size() ), barriers.data() );
	}
}

#endif