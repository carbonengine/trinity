// Copyright © 2026 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "PersistentConstantBufferPoolDx12.h"
#include "../Tr2RenderContextDx12.h"

CCP_STATS_DECLARE( cbPersistentChunks, "Trinity/AL/cbPersistentChunks", false, CST_COUNTER_LOW, "Chunks allocated by the persistent constant buffer pool." );

namespace
{
constexpr uint64_t SLOT_REUSE_FRAME_DELAY = 4;
}

namespace TrinityALImpl
{

void PersistentConstantBufferPool::Initialize( ID3D12Device* device )
{
	Destroy();
	m_device = device;
}

void PersistentConstantBufferPool::Destroy()
{
	for( auto& sizeClass : m_classes )
	{
		for( uint32_t i = 0; i < sizeClass.chunkCount; ++i )
		{
			sizeClass.chunks[i] = Chunk();
		}
		sizeClass.chunkCount = 0;
		sizeClass.nextUnusedSlot = 0;
		sizeClass.freeSlots.clear();
		sizeClass.retiredSlots.clear();
	}
	m_device = nullptr;
	m_pendingCopies.clear();
	m_hasPendingCopies = false;
}

bool PersistentConstantBufferPool::AddChunk( uint32_t classIndex )
{
	auto& sizeClass = m_classes[classIndex];
	if( sizeClass.chunkCount >= MAX_CHUNKS )
	{
		return false;
	}
	auto& chunk = sizeClass.chunks[sizeClass.chunkCount];
	D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
	D3D12_RESOURCE_DESC desc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, uint64_t( SLOTS_PER_CHUNK ) * sizeClass.slotSize, 1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	HRESULT hr = m_device->CreateCommittedResource( &heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, IID_PPV_ARGS( &chunk.buffer ) );
	CCP_ASSERT( hr == S_OK );
	if( hr != S_OK )
	{
		chunk.buffer = nullptr;
		return false;
	}
	char name[64];
	sprintf_s( name, "Persistent constant buffer pool %uB #%u", sizeClass.slotSize, sizeClass.chunkCount );
	chunk.buffer->SetPrivateData( WKPDID_D3DDebugObjectName, UINT( strlen( name ) ), name );
	chunk.address = chunk.buffer->GetGPUVirtualAddress();
	++sizeClass.chunkCount;
	CCP_STATS_INC( cbPersistentChunks );
	return true;
}

uint32_t PersistentConstantBufferPool::AllocateSlot( uint32_t size, uint64_t recordingFrame )
{
	if( !m_device || size > MAX_SLOT_SIZE )
	{
		return INVALID_SLOT;
	}
	const uint32_t classIndex = size > m_classes[0].slotSize ? 1 : 0;
	auto& sizeClass = m_classes[classIndex];

	std::lock_guard lock( m_slotMutex );
	auto& retired = sizeClass.retiredSlots;
	for( size_t i = 0; i < retired.size(); )
	{
		if( retired[i].frame + SLOT_REUSE_FRAME_DELAY <= recordingFrame )
		{
			sizeClass.freeSlots.push_back( retired[i].slot );
			retired[i] = retired.back();
			retired.pop_back();
		}
		else
		{
			++i;
		}
	}
	if( !sizeClass.freeSlots.empty() )
	{
		uint32_t slot = sizeClass.freeSlots.back();
		sizeClass.freeSlots.pop_back();
		return slot;
	}
	if( sizeClass.nextUnusedSlot >= sizeClass.chunkCount * SLOTS_PER_CHUNK && !AddChunk( classIndex ) )
	{
		return INVALID_SLOT;
	}
	return ( classIndex << CLASS_SHIFT ) | sizeClass.nextUnusedSlot++;
}

void PersistentConstantBufferPool::ReleaseSlot( uint32_t slot, uint64_t recordingFrame )
{
	if( slot == INVALID_SLOT )
	{
		return;
	}
	std::lock_guard lock( m_slotMutex );
	m_classes[slot >> CLASS_SHIFT].retiredSlots.push_back( { slot, recordingFrame } );
}

ID3D12Resource* PersistentConstantBufferPool::GetSlotResource( uint32_t slot, uint64_t& offset ) const
{
	auto& sizeClass = m_classes[slot >> CLASS_SHIFT];
	const uint32_t local = slot & LOCAL_MASK;
	offset = uint64_t( local % SLOTS_PER_CHUNK ) * sizeClass.slotSize;
	return sizeClass.chunks[local / SLOTS_PER_CHUNK].buffer;
}

D3D12_GPU_VIRTUAL_ADDRESS PersistentConstantBufferPool::GetSlotAddress( uint32_t slot ) const
{
	auto& sizeClass = m_classes[slot >> CLASS_SHIFT];
	const uint32_t local = slot & LOCAL_MASK;
	return sizeClass.chunks[local / SLOTS_PER_CHUNK].address + uint64_t( local % SLOTS_PER_CHUNK ) * sizeClass.slotSize;
}

bool PersistentConstantBufferPool::IsValid() const
{
	return m_device != nullptr;
}

void PersistentConstantBufferPool::QueueCopy( uint32_t slot, ID3D12Resource* source, uint64_t sourceOffset, uint32_t size )
{
	std::lock_guard lock( m_copyMutex );
	m_pendingCopies.push_back( { slot, source, sourceOffset, std::min( size, m_classes[slot >> CLASS_SHIFT].slotSize ) } );
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

	std::vector<ID3D12Resource*> resources;
	std::vector<D3D12_RESOURCE_BARRIER> barriers;
	for( auto& copy : copies )
	{
		uint64_t offset;
		auto resource = GetSlotResource( copy.slot, offset );
		if( std::find( begin( resources ), end( resources ), resource ) == end( resources ) )
		{
			resources.push_back( resource );
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = resource;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
			barriers.push_back( barrier );
		}
	}
	renderContext.ResourceBarrierDx12( barriers.size(), barriers.data() );
	renderContext.FlushBarriersDx12( resources.size(), resources.data() );

	for( auto& copy : copies )
	{
		uint64_t offset;
		auto resource = GetSlotResource( copy.slot, offset );
		renderContext.m_commandList->CopyBufferRegion( resource, offset, copy.source, copy.sourceOffset, copy.size );
	}

	for( auto& barrier : barriers )
	{
		std::swap( barrier.Transition.StateBefore, barrier.Transition.StateAfter );
	}
	renderContext.ResourceBarrierDx12( barriers.size(), barriers.data() );
	renderContext.FlushBarriersDx12( resources.size(), resources.data() );
}

}

#endif
