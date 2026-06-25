// Copyright © 2019 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <cstdint>
#include <memory>

#include "FreeList.h"

/** A single page within a global descriptor heap */
class GlobalDescriptorHeapPage
{
public:
	/** Init args for DescriptorEntry */
	struct DescriptorInitArgs
	{
		D3D12_CPU_DESCRIPTOR_HANDLE m_baseOffsetCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE m_baseOffsetGPU;
		UINT m_entrySize;

		DescriptorInitArgs( D3D12_CPU_DESCRIPTOR_HANDLE baseOffsetCPU, D3D12_GPU_DESCRIPTOR_HANDLE baseOffsetGPU, UINT entrySize ) :
			m_baseOffsetCPU( baseOffsetCPU ),
			m_baseOffsetGPU( baseOffsetGPU ),
			m_entrySize( entrySize )
		{
		}
	};

	/** A single entry in the global descriptor heap */
	struct DescriptorEntry
	{
		/** Page owner and tag (for fast deletion) */
		GlobalDescriptorHeapPage* m_owner;
		void* m_pageTag;

		/** Offset of the descriptor (CPU) */
		D3D12_CPU_DESCRIPTOR_HANDLE m_offsetCPU;

		/** Offset of the descriptor (GPU) */
		D3D12_GPU_DESCRIPTOR_HANDLE m_offsetGPU;

		/** */
		DescriptorEntry( uint32_t entryIdx, const struct DescriptorInitArgs& initArgs );
	};

	/** */
	GlobalDescriptorHeapPage( CComPtr<ID3D12DescriptorHeap> descriptorHeap, UINT entryCount, UINT entrySize );

	/** Allocate an entry */
	DescriptorEntry* Allocate();

	/** Free an entry */
	void Free( DescriptorEntry* entry );

	/** Get whether this page is full */
	bool IsFull() const
	{
		return m_freeList->IsFull();
	}

private:
	typedef FreeList<DescriptorEntry, DescriptorInitArgs> DescriptorList;
	std::shared_ptr<DescriptorList> m_freeList;

	CComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
};

/** A descriptor allocator which contains a number of pages */
class GlobalDescriptorHeapAllocator
{
public:
	/** Heap stats container */
	struct HeapStats
	{
		/** Number of descriptors allocated by this heap */
		uint32_t m_descriptorsInUse;

		/** Maximum number of descriptors this heap can hold */
		uint32_t m_totalDescriptors;

		/** Number of pages that are full */
		uint32_t m_fullPages;

		/** Number of pages that are actually backed by a D3D Descriptor Heap */
		uint32_t m_backedPages;

		/** Maximum number of pages that this heap can hold */
		uint32_t m_totalPages;
	};

	/** */
	GlobalDescriptorHeapAllocator( CComPtr<ID3D12Device> device, uint32_t maxPages, uint32_t pageEntryCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType );

	/** */
	~GlobalDescriptorHeapAllocator();

	/** Allocate an entry */
	GlobalDescriptorHeapPage::DescriptorEntry* Allocate();

	/** Free an entry */
	void Free( GlobalDescriptorHeapPage::DescriptorEntry* entry );

private:
	/** A freelist page entry */
	struct HeapPageEntry
	{
		std::unique_ptr<GlobalDescriptorHeapPage> m_page;

		HeapPageEntry( uint32_t, const uint32_t& ) :
			m_page( nullptr )
		{
		}
	};

	typedef FreeList<HeapPageEntry, uint32_t> PageList;
	PageList m_pages;
	CcpMutex m_mutex;

	CComPtr<ID3D12Device> m_device;
	D3D12_DESCRIPTOR_HEAP_TYPE m_heapType;
	UINT m_heapIncrement;
	uint32_t m_pageEntryCount;

	uint32_t m_descriptorsInUse;
};



class GpuVisibleDescriptorAllocator
{
public:
	GpuVisibleDescriptorAllocator( ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t initialSize );
	~GpuVisibleDescriptorAllocator();

	ID3D12DescriptorHeap* GetGpuVisibleHeap() const;

	GlobalDescriptorHeapPage::DescriptorEntry* Allocate();
	void Free( GlobalDescriptorHeapPage::DescriptorEntry* entry );

	void SetFrameIndices( uint64_t recordingFrame, uint64_t renderedFrame );

	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorInCpuHeap( GlobalDescriptorHeapPage::DescriptorEntry* entry );
	uint32_t GetIndexInHeap( GlobalDescriptorHeapPage::DescriptorEntry* entry ) const;

private:
	bool Resize();

	CComPtr<ID3D12Device> m_device;
	CComPtr<ID3D12DescriptorHeap> m_gpuHeap;
	CComPtr<ID3D12DescriptorHeap> m_cpuHeap;
	CcpMutex m_mutex;
	uint32_t m_size;
	uint32_t m_sizeIncrement;

	uint64_t m_frameNumber;
	std::vector<std::pair<GlobalDescriptorHeapPage::DescriptorEntry*, uint64_t>> m_pendingFree;
	std::vector<std::pair<CComPtr<ID3D12DescriptorHeap>, uint64_t>> m_pendingReleaseHeaps;


	using DescriptorList = FreeList<GlobalDescriptorHeapPage::DescriptorEntry, GlobalDescriptorHeapPage::DescriptorInitArgs>;
	std::unique_ptr<DescriptorList> m_freeList;
	D3D12_DESCRIPTOR_HEAP_TYPE m_type;
	UINT m_entrySize;
};


#endif
