// Copyright © 2019 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <cstdint>
#include <memory>
#include <vector>

/** An entry into an allocated frame local heap */
struct DescriptorHeapEntry
{
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle;
	ID3D12DescriptorHeap* m_heap;
	bool m_isDirty;
};

/** A page within a FrameLocalDescriptorHeapAllocator */
class FrameLocalDescriptorHeapPage
{
public:
	/** */
	FrameLocalDescriptorHeapPage( CComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t entryCount, uint32_t entrySize );

	/** Reset the page write pointer */
	void Reset();

	/** Gets whether the page can accomodate the request entry count */
	bool HasSpace( uint32_t requiredEntries ) const;

	/** Allocates space within the page */
	DescriptorHeapEntry Allocate( uint32_t requiredEntries );

	/** Gets the handle of this heap */
	ID3D12DescriptorHeap* GetHeap() const
	{
		return m_descriptorHeap;
	}

private:
	uint32_t m_entryCount;
	uint32_t m_entrySize;
	uint32_t m_currentEntry;
	D3D12_CPU_DESCRIPTOR_HANDLE m_baseOffsetCpu;
	D3D12_GPU_DESCRIPTOR_HANDLE m_baseOffsetGpu;
	CComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
};

/** A linear descriptor heap allocator used for live batches */
class FrameLocalDescriptorHeapAllocator
{
public:
	/** */
	FrameLocalDescriptorHeapAllocator( CComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t pageEntryCount );

	/** Resets the current page pointer as well as the page */
	void Reset();

	/** Allocates space within the heap and grows if necessary */
	DescriptorHeapEntry Allocate( uint32_t requiredEntries );

	/** Gets the handle of the currently active page */
	ID3D12DescriptorHeap* GetCurrentHeap() const;

private:
	/** Add a new page */
	void AddPage();

	CComPtr<ID3D12Device> m_device;
	D3D12_DESCRIPTOR_HEAP_TYPE m_heapType;
	uint32_t m_pageEntryCount;
	uint32_t m_pageEntrySize;
	uint32_t m_currentPage;
	std::vector<FrameLocalDescriptorHeapPage> m_pages;
};

#endif
