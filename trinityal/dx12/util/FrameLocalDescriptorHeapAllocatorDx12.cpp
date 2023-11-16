////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "FrameLocalDescriptorHeapAllocatorDx12.h"

//////////////////////////////////////////////////////////////////////////
// FrameLocalDescriptorHeapPage
//////////////////////////////////////////////////////////////////////////

/** */
FrameLocalDescriptorHeapPage::FrameLocalDescriptorHeapPage(CComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t entryCount, uint32_t entrySize) :
	m_entryCount(entryCount),
	m_entrySize(entrySize),
	m_currentEntry(0),
	m_descriptorHeap(descriptorHeap)
{
	m_baseOffsetCpu = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_baseOffsetGpu = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

/** Reset the page write pointer */
void FrameLocalDescriptorHeapPage::Reset()
{
	m_currentEntry = 0;
}

/** Gets whether the page can accomodate the request entry count */
bool FrameLocalDescriptorHeapPage::HasSpace(uint32_t requiredEntries) const
{
	return m_entryCount - m_currentEntry >= requiredEntries;
}

/** Allocates space within the page */
DescriptorHeapEntry FrameLocalDescriptorHeapPage::Allocate(uint32_t requiredEntries)
{
	CCP_ASSERT(HasSpace(requiredEntries));

	DescriptorHeapEntry result;
	result.m_cpuHandle.ptr = m_baseOffsetCpu.ptr + m_currentEntry * m_entrySize;
	result.m_gpuHandle.ptr = m_baseOffsetGpu.ptr + m_currentEntry * m_entrySize;
	result.m_heap = m_descriptorHeap;
	result.m_isDirty = false;

	m_currentEntry += requiredEntries;

	return result;
}

//////////////////////////////////////////////////////////////////////////
// FrameLocalDescriptorHeapAllocator
//////////////////////////////////////////////////////////////////////////

/** */
FrameLocalDescriptorHeapAllocator::FrameLocalDescriptorHeapAllocator(CComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t pageEntryCount) :
	m_device(device),
	m_heapType(heapType),
	m_pageEntryCount(pageEntryCount),
	m_currentPage(0)
{
	m_pageEntrySize = m_device->GetDescriptorHandleIncrementSize(m_heapType);

	// Add default page
	AddPage();
}

/** Resets the current page pointer as well as the page */
void FrameLocalDescriptorHeapAllocator::Reset()
{
	m_currentPage = 0;
	m_pages[0].Reset();
}

/** Allocates space within the heap and grows if necessary */
DescriptorHeapEntry FrameLocalDescriptorHeapAllocator::Allocate(uint32_t requiredEntries)
{
	// What if a page size is too small to fit all the possible resource in?
	CCP_ASSERT(requiredEntries <= m_pageEntryCount);

	// Page is almost full, move to the next one
	bool isDirty = false;
	if (!m_pages[m_currentPage].HasSpace(requiredEntries))
	{
		m_currentPage++;
		isDirty = true;

		// Grow allocator if necessary
		if (m_currentPage >= m_pages.size())
		{
			AddPage();
		}
		else
		{
			m_pages[m_currentPage].Reset();
		}
	}
	CCP_ASSERT(m_pages[m_currentPage].HasSpace(requiredEntries));

	DescriptorHeapEntry result = m_pages[m_currentPage].Allocate(requiredEntries);
	result.m_isDirty = isDirty;
	return result;
}

/** Add a new page */
void FrameLocalDescriptorHeapAllocator::AddPage()
{
	CComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = m_heapType;
	heapDesc.NumDescriptors = m_pageEntryCount;

	HRESULT hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap));
	CCP_ASSERT(hr == S_OK);
	if (hr != S_OK)
	{
		return;
	}

	m_pages.push_back(FrameLocalDescriptorHeapPage(descriptorHeap, m_pageEntryCount, m_pageEntrySize));
}

/** Gets the handle of the currently active page */
ID3D12DescriptorHeap* FrameLocalDescriptorHeapAllocator::GetCurrentHeap() const
{
	return m_pages[m_currentPage].GetHeap();
}

#endif
