////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "FrameLocalUploadBufferAllocatorDx12.h"
#include "../Tr2PrimaryRenderContextDx12.h"

namespace
{
	uintptr_t Align(uintptr_t offset, size_t alignment)
	{
		return (offset + (alignment - 1)) & ~(alignment - 1);
	}
}

//////////////////////////////////////////////////////////////////////////
// FrameLocalUploadBufferPage
//////////////////////////////////////////////////////////////////////////

/** */
FrameLocalUploadBufferPage::FrameLocalUploadBufferPage(Tr2PrimaryRenderContextAL* primaryContext, CComPtr<ID3D12Resource> buffer, uint32_t size) :
	m_primaryContext(primaryContext),
	m_buffer(buffer),
	m_size(size),
	m_currentOffset(0)
{
	D3D12_RANGE range = { 0, 0 };
	m_buffer->Map(0, &range, (void**)&m_cpuAddr);

	m_gpuAddr = m_buffer->GetGPUVirtualAddress();
}

/** */
FrameLocalUploadBufferPage::~FrameLocalUploadBufferPage()
{
}

/** Reset the page write pointer */
void FrameLocalUploadBufferPage::Reset()
{
	m_currentOffset = 0;
}

/** Gets whether the page can accommodate the requested allocation */
bool FrameLocalUploadBufferPage::HasSpace(uint32_t size) const
{
	return (m_size - m_currentOffset) >= size;
}

/** Allocates space within the page */
UploadHeapEntry FrameLocalUploadBufferPage::Allocate(uint32_t size, uint32_t align)
{
	UploadHeapEntry entry;

	if (!HasSpace(size + align))
	{
		CCP_ASSERT(false);
		return entry;
	}

	uintptr_t cpu_base = m_cpuAddr + m_currentOffset;
	uintptr_t cpu_offset = Align(cpu_base, align);
	uintptr_t align_offset = cpu_offset - cpu_base;
	
	entry.m_cpuAddr = reinterpret_cast<void*>(cpu_offset);
	entry.m_gpuAddr =  m_gpuAddr + m_currentOffset + align_offset;
	entry.m_size = size;

	m_currentOffset += uint32_t(size + align_offset);

	return entry;
}


//////////////////////////////////////////////////////////////////////////
// FrameLocalUploadBufferAllocator
//////////////////////////////////////////////////////////////////////////

/** */
FrameLocalUploadBufferAllocator::FrameLocalUploadBufferAllocator(Tr2PrimaryRenderContextAL* primaryContext, uint32_t defaultPageSize, uint32_t spillPageSize) :
	m_primaryContext(primaryContext),
	m_spillPageSize(spillPageSize),
	m_currentPage(0)
{
	// Add default page
	AddPage(defaultPageSize);
}

/** Resets the page points and frees any spill pages */
void FrameLocalUploadBufferAllocator::Reset()
{
	// Remove any unused spill pages
	m_pages.resize( m_currentPage + 1 );

	m_currentPage = 0;
	for( auto it = begin( m_pages ); it != end( m_pages ); ++it )
	{
		it->Reset();
	}
}

/** Allocates space within an upload heap */
UploadHeapEntry FrameLocalUploadBufferAllocator::Allocate(uint32_t size, uint32_t align)
{
	if (!m_pages[m_currentPage].HasSpace(size + align))
	{
		m_currentPage++;
		if( m_currentPage >= m_pages.size() )
		{
			CCP_LOGWARN( "Out of constant buffer memory, allocating a spill page... this is slow and shouldn't happen often!!!" );
			AddPage( m_spillPageSize );
		}
	}
	CCP_ASSERT(m_pages[m_currentPage].HasSpace(size + align));

	return m_pages[m_currentPage].Allocate(size, align);
}

/** Add a new page */
void FrameLocalUploadBufferAllocator::AddPage(uint32_t size)
{
	CComPtr<ID3D12Resource> buffer;

	D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
	D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

	HRESULT hr = m_primaryContext->m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
	CCP_ASSERT(hr == S_OK);
	if (hr != S_OK)
	{
		return;
	}
	
	const char* name = "Constant upload heap";
	buffer->SetPrivateData( WKPDID_D3DDebugObjectName, UINT( strlen( name ) ), name );

	m_pages.push_back( FrameLocalUploadBufferPage( m_primaryContext, buffer, size ) );
}

#endif