#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12


#include "Tr2ResourceHelper.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Utilities.h"

namespace TrinityALImpl
{
	Tr2ResourceHelper::Tr2ResourceHelper() :
		m_size( 0 ),
		m_defaultState( D3D12_RESOURCE_STATE_COMMON ),
		m_strategy( STATIC ),
		m_gpuResource(),
		m_mapped()
	{
	}

	ALResult Tr2ResourceHelper::Create( Strategy strategy, size_t size, D3D12_RESOURCE_FLAGS resourceFlags, D3D12_RESOURCE_STATES state, uint32_t initialDataCount, const D3D12_SUBRESOURCE_DATA* initialData, Tr2PrimaryRenderContextAL& renderContext )
	{
		if( strategy != DYNAMIC )
		{
			CComPtr<ID3D12Resource> buffer, scratch;
			auto heap = HeapDesc( D3D12_HEAP_TYPE_DEFAULT );
			auto resourceDesc = BufferDesc( size, resourceFlags );
			auto initialState = initialDataCount ? D3D12_RESOURCE_STATE_COPY_DEST : state;
			CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
				&heap,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				initialState,
				nullptr,
				IID_PPV_ARGS( &buffer ) ) );
			if( initialDataCount )
			{
				auto scratchHeap = HeapDesc( D3D12_HEAP_TYPE_UPLOAD );
				auto scratchDesc = BufferDesc( size );
				CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
					&scratchHeap,
					D3D12_HEAP_FLAG_NONE,
					&scratchDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS( &scratch ) ) );
				if( !UpdateSubresources(
					renderContext.m_commandList,
					buffer,
					scratch,
					0,
					0,
					initialDataCount,
					initialData ) )
				{
					return E_FAIL;
				}
				if( state != initialState )
				{
					renderContext.ResourceBarrierDx12( Transition( buffer, initialState, state ) );
				}
				if( strategy == STATIC )
				{
					RELEASE_LATER( &renderContext, scratch );
				}
				else
				{
					Resource r = { scratch, renderContext.GetCurrentFrameIndexDx12(), nullptr, 0 };
					D3D12_RANGE readRange = { 0, 0 };
					CR_RETURN_HR( scratch->Map( 0, &readRange, &r.cpuAddress ) );

					m_resources.push_back( r );
				}
			}
			m_gpuResource.resource = buffer;
			m_gpuResource.frameIndex = renderContext.GetCurrentFrameIndexDx12();
			m_gpuResource.cpuAddress = nullptr;
			m_gpuResource.gpuAddress = buffer->GetGPUVirtualAddress();
		}
		else
		{
			CComPtr<ID3D12Resource> buffer;
			auto heap = HeapDesc( D3D12_HEAP_TYPE_UPLOAD );
			auto resourceDesc = BufferDesc( size, resourceFlags );
			CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
				&heap,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				state,
				nullptr,
				IID_PPV_ARGS( &buffer ) ) );
			Resource r = { buffer, renderContext.GetCurrentFrameIndexDx12(), nullptr, buffer->GetGPUVirtualAddress() };
			D3D12_RANGE readRange = { 0, 0 };
			CR_RETURN_HR( buffer->Map( 0, &readRange, &r.cpuAddress ) );
			if( initialDataCount )
			{
				memcpy( r.cpuAddress, initialData[0].pData, size );
			}
			m_resources.push_back( r );
			m_gpuResource = r;
		}
		m_size = size;
		m_defaultState = state;
		m_strategy = strategy;
		return S_OK;
	}

	void Tr2ResourceHelper::Destroy( Tr2PrimaryRenderContextAL& renderContext )
	{
		if( m_gpuResource.resource )
		{
			RELEASE_LATER( &renderContext, m_gpuResource.resource );
			m_gpuResource = Resource();
		}
		for( auto it = begin( m_resources ); it != end( m_resources ); ++it )
		{
			RELEASE_LATER( &renderContext, it->resource );
		}
		m_resources.clear();
	}

	ALResult Tr2ResourceHelper::CreateScratch( Resource& resource, Tr2PrimaryRenderContextAL& renderContext )
	{
		CComPtr<ID3D12Resource> buffer;
		auto scratchHeap = HeapDesc( D3D12_HEAP_TYPE_UPLOAD );
		auto scratchDesc = BufferDesc( m_size );
		CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
			&scratchHeap,
			D3D12_HEAP_FLAG_NONE,
			&scratchDesc,
			m_strategy == DYNAMIC ? m_defaultState : D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &buffer ) ) );

		D3D12_RANGE readRange = { 0, 0 };
		CR_RETURN_HR( buffer->Map( 0, &readRange, &resource.cpuAddress ) );
		resource.resource = buffer;
		resource.frameIndex = renderContext.GetCurrentFrameIndexDx12();
		resource.gpuAddress = buffer->GetGPUVirtualAddress();
		if( !m_name.empty() )
		{
			SetDebugName( buffer, m_name.c_str() );
		}
		return S_OK;
	}

	ALResult Tr2ResourceHelper::MapForWriting( void*& data, Tr2LockType::Type lockType, Tr2PrimaryRenderContextAL& renderContext )
	{
		if( lockType == Tr2LockType::NON_SYNCHRONIZED && m_strategy == DYNAMIC )
		{
			for( auto it = begin( m_resources ); it != end( m_resources ); ++it )
			{
				if( it->resource == m_gpuResource.resource )
				{
					it->frameIndex = renderContext.GetCurrentFrameIndexDx12();
					break;
				}
			}
			m_mapped = m_gpuResource;
			data = m_mapped.cpuAddress;
			return S_OK;
		}
		if( m_strategy == STATIC )
		{
			FORWARD_HR( CreateScratch( m_mapped, renderContext ) );
			data = m_mapped.cpuAddress;
			return S_OK;
		}
		else
		{
			auto completed = renderContext.GetCompletedFrameIndexDx12();

			for( auto it = begin( m_resources ); it != end( m_resources ); ++it )
			{
				if( completed >= it->frameIndex )
				{
					it->frameIndex = renderContext.GetCurrentFrameIndexDx12();
					m_mapped = *it;
					data = m_mapped.cpuAddress;
					return S_OK;
				}
			}

			FORWARD_HR( CreateScratch( m_mapped, renderContext ) );
			m_resources.push_back( m_mapped );
			data = m_mapped.cpuAddress;
			return S_OK;
		}
	}

	void Tr2ResourceHelper::UnmapForWriting( Tr2PrimaryRenderContextAL& renderContext )
	{
		if( !m_mapped.resource )
		{
			return;
		}
		if( m_strategy == DYNAMIC )
		{
			m_gpuResource = m_mapped;
		}
		else
		{
			auto barrier = Transition( m_gpuResource.resource, m_defaultState, D3D12_RESOURCE_STATE_COPY_DEST );
			renderContext.ResourceBarrierDx12( barrier );
			renderContext.FlushBarriersDx12( m_gpuResource.resource );

			renderContext.m_commandList->CopyBufferRegion( m_gpuResource.resource, 0, m_mapped.resource, 0, m_size );

			std::swap( barrier.Transition.StateAfter, barrier.Transition.StateBefore );
			renderContext.ResourceBarrierDx12( barrier );

			if( m_strategy == STATIC )
			{
				RELEASE_LATER( renderContext.m_ownerDevice, m_mapped.resource );
			}
		}
		m_mapped = Resource();
	}

	ALResult Tr2ResourceHelper::UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext )
	{
		if( m_strategy == DYNAMIC )
		{
			return E_FAIL;
		}

		void* dst = nullptr;
		FORWARD_HR( MapForWriting( dst, Tr2LockType::SYNCHRONIZED, *renderContext.m_ownerDevice ) );
		memcpy( dst, data, size );

		auto barrier = Transition( m_gpuResource.resource, m_defaultState, D3D12_RESOURCE_STATE_COPY_DEST );
		renderContext.ResourceBarrierDx12( barrier );
		renderContext.FlushBarriersDx12( m_gpuResource.resource );

		renderContext.m_commandList->CopyBufferRegion( m_gpuResource.resource, offset, m_mapped.resource, 0, size );

		std::swap( barrier.Transition.StateAfter, barrier.Transition.StateBefore );
		renderContext.ResourceBarrierDx12( barrier );

		if( m_strategy == STATIC )
		{
			RELEASE_LATER( renderContext.m_ownerDevice, m_mapped.resource );
		}
		m_mapped = Resource();
		return S_OK;
	}

	ID3D12Resource* Tr2ResourceHelper::GetResource() const
	{
		return m_gpuResource.resource;
	}

	D3D12_GPU_VIRTUAL_ADDRESS Tr2ResourceHelper::GetGpuView() const
	{
		return m_gpuResource.gpuAddress;
	}

	void Tr2ResourceHelper::SetName( const char* name )
	{
		m_name = name;
		for( auto& r : m_resources )
		{
			SetDebugName( r.resource, name );
		}
		if( m_gpuResource.resource )
		{
			SetDebugName( m_gpuResource.resource, name );
		}
	}

	const std::string& Tr2ResourceHelper::GetName() const
	{
		return m_name;
	}
}

#endif