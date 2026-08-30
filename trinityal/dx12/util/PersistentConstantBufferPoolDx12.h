// Copyright © 2026 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <atomic>
#include <mutex>
#include <vector>

class Tr2RenderContextAL;

namespace TrinityALImpl
{

// GPU-resident slots for constant buffers whose contents rarely change. A slot address stays valid for the
// life of the buffer; changed data is staged in the frame upload ring and copied into the slot on the command
// list before the next draw, so in-flight frames never see the write. Slots come in two size classes, each a
// list of chunks that grows on demand; addresses never move.
class PersistentConstantBufferPool
{
public:
	static const uint32_t INVALID_SLOT = 0xffffffff;
	static const uint32_t MAX_SLOT_SIZE = 512;

	void Initialize( ID3D12Device* device );
	void Destroy();

	uint32_t AllocateSlot( uint32_t size, uint64_t recordingFrame );
	void ReleaseSlot( uint32_t slot, uint64_t recordingFrame );

	D3D12_GPU_VIRTUAL_ADDRESS GetSlotAddress( uint32_t slot ) const;
	bool IsValid() const;

	void QueueCopy( uint32_t slot, ID3D12Resource* source, uint64_t sourceOffset, uint32_t size );
	void FlushCopies( Tr2RenderContextAL& renderContext );

private:
	static const uint32_t SLOTS_PER_CHUNK = 32768;
	static const uint32_t MAX_CHUNKS = 64;
	static const uint32_t CLASS_SHIFT = 24;
	static const uint32_t LOCAL_MASK = ( 1u << CLASS_SHIFT ) - 1;

	struct Chunk
	{
		CComPtr<ID3D12Resource> buffer;
		D3D12_GPU_VIRTUAL_ADDRESS address = 0;
	};
	struct RetiredSlot
	{
		uint32_t slot;
		uint64_t frame;
	};
	struct SizeClass
	{
		uint32_t slotSize;
		Chunk chunks[MAX_CHUNKS];
		uint32_t chunkCount = 0;
		uint32_t nextUnusedSlot = 0;
		std::vector<uint32_t> freeSlots;
		std::vector<RetiredSlot> retiredSlots;
	};
	struct PendingCopy
	{
		uint32_t slot;
		ID3D12Resource* source;
		uint64_t sourceOffset;
		uint32_t size;
	};

	bool AddChunk( uint32_t classIndex );
	ID3D12Resource* GetSlotResource( uint32_t slot, uint64_t& offset ) const;

	CComPtr<ID3D12Device> m_device;
	SizeClass m_classes[2] = { { 256 }, { 512 } };

	std::mutex m_slotMutex;

	std::mutex m_copyMutex;
	std::vector<PendingCopy> m_pendingCopies;
	std::atomic<bool> m_hasPendingCopies{ false };
};

}

#endif
