// Copyright © 2026 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <atomic>
#include <mutex>
#include <vector>

class Tr2RenderContextAL;

namespace TrinityALImpl
{

// Fixed-size GPU-resident slots for constant buffers whose contents rarely change. A slot address
// stays valid for the life of the buffer; changed data is staged in the frame upload ring and copied
// into the slot on the command list before the next draw, so in-flight frames never see the write.
class PersistentConstantBufferPool
{
public:
	static const uint32_t SLOT_SIZE = 512;
	static const uint32_t INVALID_SLOT = 0xffffffff;

	void Initialize( ID3D12Device* device, uint32_t slotCount );
	void Destroy();

	uint32_t AllocateSlot( uint64_t recordingFrame );
	void ReleaseSlot( uint32_t slot, uint64_t recordingFrame );

	D3D12_GPU_VIRTUAL_ADDRESS GetSlotAddress( uint32_t slot ) const;
	bool IsValid() const;

	void QueueCopy( uint32_t slot, ID3D12Resource* source, uint64_t sourceOffset, uint32_t size );
	void FlushCopies( Tr2RenderContextAL& renderContext );

private:
	struct PendingCopy
	{
		uint32_t slot;
		ID3D12Resource* source;
		uint64_t sourceOffset;
		uint32_t size;
	};
	struct RetiredSlot
	{
		uint32_t slot;
		uint64_t frame;
	};

	CComPtr<ID3D12Resource> m_buffer;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddress = 0;
	uint32_t m_slotCount = 0;
	uint32_t m_nextUnusedSlot = 0;

	std::mutex m_slotMutex;
	std::vector<uint32_t> m_freeSlots;
	std::vector<RetiredSlot> m_retiredSlots;

	std::mutex m_copyMutex;
	std::vector<PendingCopy> m_pendingCopies;
	std::atomic<bool> m_hasPendingCopies{ false };
};

}

#endif
