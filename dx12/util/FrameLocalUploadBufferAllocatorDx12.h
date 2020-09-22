////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <cstdint>
#include <memory>
#include <vector>

/** An entry into an upload heap */
struct UploadHeapEntry
{
	UploadHeapEntry() :
		m_cpuAddr(nullptr),
		m_gpuAddr(0),
		m_size(0)
	{
	}

	void* m_cpuAddr;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddr;
	uint32_t m_size;
};

/** A page within a FrameLocalUploadBufferAllocator */
class FrameLocalUploadBufferPage
{
public:

	/** */
	FrameLocalUploadBufferPage() :
		m_primaryContext(nullptr),
		m_buffer(nullptr),
		m_cpuAddr( 0 ),
		m_size(0),
		m_currentOffset( 0 )
	{
	}

	/** */
	FrameLocalUploadBufferPage(class Tr2PrimaryRenderContextAL* primaryContext, CComPtr<ID3D12Resource> buffer, uint32_t size);
	
	/** */
	~FrameLocalUploadBufferPage();

	/** Reset the page write pointer */
	void Reset();

	/** Gets whether the page can accommodate the requested allocation */
	bool HasSpace(uint32_t size) const;

	/** Allocates space within the page */
	UploadHeapEntry Allocate(uint32_t size, uint32_t align);

private:

	Tr2PrimaryRenderContextAL* m_primaryContext;
	CComPtr<ID3D12Resource> m_buffer;
	uintptr_t m_cpuAddr;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddr;
	uint32_t m_size;
	uint32_t m_currentOffset;
};

/** A linear allocator for upload heaps which destroys 'spill' pages on reset */
class FrameLocalUploadBufferAllocator
{
public:

	/** */
	FrameLocalUploadBufferAllocator(class Tr2PrimaryRenderContextAL* primaryContext, uint32_t defaultPageSize, uint32_t spillPageSize);

	/** Resets the page points and frees any spill pages */
	void Reset();

	/** Allocates space within an upload heap */
	UploadHeapEntry Allocate(uint32_t size, uint32_t align);

private:

	/** Add a new page */
	void AddPage(uint32_t size);

	class Tr2PrimaryRenderContextAL* m_primaryContext;
	std::vector<FrameLocalUploadBufferPage> m_pages;
	uint32_t m_spillPageSize;
	uint32_t m_currentPage;
};

#endif