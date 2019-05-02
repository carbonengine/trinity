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

using namespace Tr2RenderContextEnum;


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

		CComPtr<ID3D12DescriptorHeap> srvDescriptors;
		CComPtr<ID3D12DescriptorHeap> samplerDescriptors;


		if( program.m_srvUavTableSize )
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			heapDesc.NumDescriptors = program.m_srvUavTableSize;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			FORWARD_HR( renderContext.m_device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &srvDescriptors ) ) );

			auto start = srvDescriptors->GetCPUDescriptorHandleForHeapStart();
			auto inc = renderContext.m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

			for( uint32_t i = 0; i < SHADER_TYPE_COUNT; ++i )
			{
				for( uint32_t j = 0; j < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++j )
				{
					auto& resource = description.m_srv[i][j];
					switch( resource.type )
					{
					case Tr2ResourceSetDescriptionAL::TEXTURE:
					{
						auto index = program.m_srvMap[i][j];
						if( index == 0xffffffff )
						{
							continue;
						}
						auto handle = start;
						handle.ptr += index * inc;
						renderContext.m_device->CreateShaderResourceView(
							resource.texture.m_texture->GetResourceDx12(),
							&resource.texture.m_texture->m_srvDesc[resource.colorSpace],
							handle );
						break;
					}
					case Tr2ResourceSetDescriptionAL::BUFFER:
					{
						auto index = program.m_srvMap[i][j];
						if( index == 0xffffffff )
						{
							continue;
						}
						auto handle = start;
						handle.ptr += index * inc;
						renderContext.m_device->CreateShaderResourceView(
							resource.buffer.m_buffer->GetGpuResource(),
							&resource.buffer.m_buffer->m_srvDesc,
							handle );
						break;
					}
					default:
						break;
					}
				}
				for( uint32_t j = 0; j < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++j )
				{
					auto& resource = description.m_uav[i][j];
					switch( resource.type )
					{
					case Tr2ResourceSetDescriptionAL::TEXTURE:
					{
						auto index = program.m_uavMap[i][j];
						if( index == 0xffffffff )
						{
							continue;
						}
						auto handle = start;
						handle.ptr += index * inc;
						renderContext.m_device->CreateUnorderedAccessView(
							resource.texture.m_texture->GetResourceDx12(),
							nullptr,
							&resource.texture.m_texture->m_uavDesc[resource.mip],
							handle );
						break;
					}
					case Tr2ResourceSetDescriptionAL::BUFFER:
					{
						auto index = program.m_uavMap[i][j];
						if( index == 0xffffffff )
						{
							continue;
						}
						auto handle = start;
						handle.ptr += index * inc;
						renderContext.m_device->CreateUnorderedAccessView(
							resource.buffer.m_buffer->GetGpuResource(),
							resource.buffer.m_buffer->m_counter,
							&resource.buffer.m_buffer->m_uavDesc,
							handle );
						if( resource.initialCount != -1 && resource.buffer.m_buffer->m_counter )
						{
							InitialCount cnt = { resource.buffer, resource.initialCount };
							m_initialCounts.push_back( cnt );
						}
						break;
					}
					default:
						break;
					}
				}
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


		if( program.m_samplerTableSize )
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			heapDesc.NumDescriptors = program.m_samplerTableSize;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			FORWARD_HR( renderContext.m_device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &samplerDescriptors ) ) );

			auto start = samplerDescriptors->GetCPUDescriptorHandleForHeapStart();
			auto inc = renderContext.m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER );

			for( uint32_t i = 0; i < SHADER_TYPE_COUNT; ++i )
			{
				for( uint32_t j = 0; j < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++j )
				{
					auto& sampler = description.m_samplers[i][j];
					if( !sampler.assigned )
					{
						continue;
					}

					auto index = program.m_samplerMap[i][j];
					if( index == 0xffffffff )
					{
						continue;
					}
					auto handle = start;
					handle.ptr += index * inc;
					renderContext.m_device->CreateSampler(
						&sampler.sampler.m_sampler->m_sampler,
						handle );
				}
			}
		}

		m_description = description;
		m_srvUavDescriptors = srvDescriptors;
		m_samplerDescriptors = samplerDescriptors;
		m_owner = &renderContext;
		m_initialCountBuffer = initialCountBuffer;

		return S_OK;
	}

	void Tr2ResourceSetAL::Destroy()
	{
		if( m_owner && m_srvUavDescriptors )
		{
			m_owner->ReleaseLater( m_srvUavDescriptors );
			m_srvUavDescriptors = nullptr;
		}
		if( m_owner && m_samplerDescriptors )
		{
			m_owner->ReleaseLater( m_samplerDescriptors );
			m_samplerDescriptors = nullptr;
		}
		if( m_owner && m_initialCountBuffer )
		{
			m_owner->ReleaseLater( m_initialCountBuffer );
			m_initialCountBuffer = nullptr;
		}
		m_description = Tr2ResourceSetDescriptionAL();
		m_owner = nullptr;
		m_initialCounts.clear();
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