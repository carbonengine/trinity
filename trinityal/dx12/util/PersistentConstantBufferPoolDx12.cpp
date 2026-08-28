// Copyright © 2026 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "PersistentConstantBufferPoolDx12.h"
#include "../Tr2RenderContextDx12.h"

namespace
{
constexpr uint64_t SLOT_REUSE_FRAME_DELAY = 4;
}

namespace TrinityALImpl
{

void PersistentConstantBufferPool::Initialize( ID3D12Device* device, uint32_t slotCount )
{
	Destroy();

	D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
	D3D12_RESOURCE_DESC desc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, uint64_t( slotCount ) * SLOT_SIZE, 1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	HRESULT hr = device->CreateCommittedResource( &heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, IID_PPV_ARGS( &m_buffer ) );
	CCP_ASSERT( hr == S_OK );
	if( hr != S_OK )
	{
		return;
	}
	const char* name = "Persistent constant buffer pool";
	m_buffer->SetPrivateData( WKPDID_D3DDebugObjectName, UINT( strlen( name ) ), name );
	m_gpuAddress = m_buffer->GetGPUVirtualAddress();
	m_slotCount = slotCount;
	m_nextUnusedSlot = 0;
}

void PersistentConstantBufferPool::Destroy()
{
	m_buffer = nullptr;
	m_gpuAddress = 0;
	m_slotCount = 0;
	m_nextUnusedSlot = 0;
	m_freeSlots.clear();
	m_retiredSlots.clear();
	m_pendingCopies.clear();
	m_hasPendingCopies = false;
}

uint32_t PersistentConstantBufferPool::AllocateSlot( uint64_t recordingFrame )
{
	std::lock_guard lock( m_slotMutex );
	for( size_t i = 0; i < m_retiredSlots.size(); )
	{
		if( m_retiredSlots[i].frame + SLOT_REUSE_FRAME_DELAY <= recordingFrame )
		{
			m_freeSlots.push_back( m_retiredSlots[i].slot );
			m_retiredSlots[i] = m_retiredSlots.back();
			m_retiredSlots.pop_back();
		}
		else
		{
			++i;
		}
	}
	if( !m_freeSlots.empty() )
	{
		uint32_t slot = m_freeSlots.back();
		m_freeSlots.pop_back();
		return slot;
	}
	if( m_nextUnusedSlot < m_slotCount )
	{
		return m_nextUnusedSlot++;
	}
	return INVALID_SLOT;
}

void PersistentConstantBufferPool::ReleaseSlot( uint32_t slot, uint64_t recordingFrame )
{
	if( slot == INVALID_SLOT )
	{
		return;
	}
	std::lock_guard lock( m_slotMutex );
	m_retiredSlots.push_back( { slot, recordingFrame } );
}

D3D12_GPU_VIRTUAL_ADDRESS PersistentConstantBufferPool::GetSlotAddress( uint32_t slot ) const
{
	return m_gpuAddress + uint64_t( slot ) * SLOT_SIZE;
}

bool PersistentConstantBufferPool::IsValid() const
{
	return m_buffer != nullptr;
}

void PersistentConstantBufferPool::QueueCopy( uint32_t slot, ID3D12Resource* source, uint64_t sourceOffset, uint32_t size )
{
	std::lock_guard lock( m_copyMutex );
	m_pendingCopies.push_back( { slot, source, sourceOffset, std::min( size, SLOT_SIZE ) } );
	m_hasPendingCopies.store( true, std::memory_order_release );
}

void PersistentConstantBufferPool::FlushCopies( Tr2RenderContextAL& renderContext )
{
	if( !m_hasPendingCopies.load( std::memory_order_acquire ) )
	{
		return;
	}
	std::vector<PendingCopy> copies;
	{
		std::lock_guard lock( m_copyMutex );
		copies.swap( m_pendingCopies );
		m_hasPendingCopies.store( false, std::memory_order_release );
	}
	if( copies.empty() )
	{
		return;
	}

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_buffer;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	renderContext.ResourceBarrierDx12( barrier );
	renderContext.FlushBarriersDx12( m_buffer );

	for( auto& copy : copies )
	{
		renderContext.m_commandList->CopyBufferRegion( m_buffer, uint64_t( copy.slot ) * SLOT_SIZE, copy.source, copy.sourceOffset, copy.size );
	}

	std::swap( barrier.Transition.StateBefore, barrier.Transition.StateAfter );
	renderContext.ResourceBarrierDx12( barrier );
	renderContext.FlushBarriersDx12( m_buffer );
}

}

#endif
