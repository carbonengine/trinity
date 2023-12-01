////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <memory.h>
#include "GlobalDescriptorHeapAllocatorDx12.h"


//////////////////////////////////////////////////////////////////////////
// GlobalDescriptorHeapAllocator
//////////////////////////////////////////////////////////////////////////

/** */
GlobalDescriptorHeapAllocator::GlobalDescriptorHeapAllocator(CComPtr<ID3D12Device> device, uint32_t maxPages, uint32_t pageEntryCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType) :
	m_mutex("GlobalDescriptorHeapAllocator", "m_mutex"),
	m_device(device),
	m_heapType(heapType),
	m_pageEntryCount(pageEntryCount),
	m_descriptorsInUse(0),
	m_pages( std::make_shared<PageList>( maxPages, maxPages ) )
{
	m_heapIncrement = m_device->GetDescriptorHandleIncrementSize(m_heapType);
}

/** */
GlobalDescriptorHeapAllocator::~GlobalDescriptorHeapAllocator()
{
	// If this isn't 0, then some views haven't been freed yet
	// If necessary, we could add a debug string to DescriptorEntry and dump those
	// if we want better object tracking
	CCP_ASSERT(m_descriptorsInUse == 0);
}

/** Allocate an entry */
GlobalDescriptorHeapPage::DescriptorEntry* GlobalDescriptorHeapAllocator::Allocate()
{
	CcpAutoMutex lock(m_mutex);

	HeapPageEntry* page = m_pages->GetFirstFree();
	if (page == nullptr)
	{
		// FATAL: Totally out of memory,, is there a better way of handling this?
		CCP_ASSERT(page != nullptr);
		return nullptr;
	}

	// Pages start off uninitialized... we might want to change this behavior in the future
	if (page->m_page == nullptr)
	{
		CComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NumDescriptors = m_pageEntryCount;
		heapDesc.Type = m_heapType;

		HRESULT hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap));
		if (hr != S_OK)
		{
			// FATAL: Can't back an allocation with a heap
			CCP_ASSERT(hr == S_OK);
			return nullptr;
		}

		// #TODO: This shouldn't be a shared_ptr, it should be a unique_ptr but MSVC10 causes it to always be null
		page->m_page = std::make_shared<GlobalDescriptorHeapPage>(descriptorHeap, m_pageEntryCount, m_heapIncrement);
	}

	// m_page->getFirstFree() should never return a full page!
	CCP_ASSERT(!page->m_page->IsFull());

	// Allocate entry
	GlobalDescriptorHeapPage::DescriptorEntry* entry = page->m_page->Allocate();
	CCP_ASSERT(entry != nullptr);

	// Assign a tag, so we can quickly free this entry later
	entry->m_pageTag = page;

	// If the page is now full, mark it as such
	if (page->m_page->IsFull())
	{
		// Note: Nothing is allocated, the free list will just move the first entry
		// over into the 'in use' list
		m_pages->Allocate();
	}

	m_descriptorsInUse++;

	return entry;
}

/** Free an entry */
void GlobalDescriptorHeapAllocator::Free(GlobalDescriptorHeapPage::DescriptorEntry* entry)
{
	CcpAutoMutex lock(m_mutex);

	CCP_ASSERT(entry->m_pageTag != nullptr);
	CCP_ASSERT(entry->m_owner != nullptr);

	// Recover page handle
	HeapPageEntry* page = reinterpret_cast<HeapPageEntry*>(entry->m_pageTag);
	CCP_ASSERT(page->m_page != nullptr);
	m_pages->ValidateEntry(page);

	// If the page is currently full, then it'll be marked as InUse
	// it'll definitely not be full after this operation, so we can move it over
	if (page->m_page->IsFull())
	{
		m_pages->Free(page);
	}

	// Free entry
	page->m_page->Free(entry);

	CCP_ASSERT(m_descriptorsInUse > 0);
	m_descriptorsInUse--;

	CCP_ASSERT(!page->m_page->IsFull());
}

/** Gather stats for this heap */
GlobalDescriptorHeapAllocator::HeapStats GlobalDescriptorHeapAllocator::GetStats() const
{
	HeapStats stats;

	stats.m_descriptorsInUse = m_descriptorsInUse;
	stats.m_totalDescriptors = m_pageEntryCount * m_pages->EntryCount();
	stats.m_backedPages = 0;

	auto& entries = m_pages->GetAllEntries();
	for (auto it = entries.begin(); it != entries.end(); ++it)
	{
		auto& entry = *it;

		if (entry.m_page != nullptr)
		{
			stats.m_backedPages++;
		}
	}

	stats.m_fullPages = m_pages->InUseCount();
	stats.m_totalPages = m_pages->EntryCount();

	return stats;
}

//////////////////////////////////////////////////////////////////////////
// GlobalDescriptorHeapPage
//////////////////////////////////////////////////////////////////////////

/** */
GlobalDescriptorHeapPage::DescriptorEntry::DescriptorEntry(uint32_t entryIdx, const GlobalDescriptorHeapPage::DescriptorInitArgs& initArgs) :
	m_owner(nullptr),
	m_pageTag(nullptr)
{
	m_offsetCPU.ptr = initArgs.m_baseOffsetCPU.ptr + entryIdx * initArgs.m_entrySize;
	m_offsetGPU.ptr = initArgs.m_baseOffsetGPU.ptr + entryIdx * initArgs.m_entrySize;
}

/** */
GlobalDescriptorHeapPage::GlobalDescriptorHeapPage(CComPtr<ID3D12DescriptorHeap> descriptorHeap, UINT entryCount, UINT entrySize) :
	m_descriptorHeap(descriptorHeap)
{
	D3D12_CPU_DESCRIPTOR_HANDLE baseOffsetCPU = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE baseOffsetGPU = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();

	DescriptorInitArgs init(baseOffsetCPU, baseOffsetGPU, entrySize);
	m_freeList = std::make_shared<DescriptorList>(entryCount, init);
}

/** Allocate an entry */
GlobalDescriptorHeapPage::DescriptorEntry* GlobalDescriptorHeapPage::Allocate()
{
	DescriptorEntry* entry = m_freeList->Allocate();
	entry->m_owner = this;

	return entry;
}

/** Free an entry */
void GlobalDescriptorHeapPage::Free(GlobalDescriptorHeapPage::DescriptorEntry* entry)
{
	CCP_ASSERT(entry->m_owner == this);

	m_freeList->Free(entry);
}

#endif
