// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <memory.h>
#include "GlobalDescriptorHeapAllocatorDx12.h"


//////////////////////////////////////////////////////////////////////////
// GlobalDescriptorHeapAllocator
//////////////////////////////////////////////////////////////////////////

/** */
GlobalDescriptorHeapAllocator::GlobalDescriptorHeapAllocator( CComPtr<ID3D12Device> device, uint32_t maxPages, uint32_t pageEntryCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType ) :
	m_mutex( "GlobalDescriptorHeapAllocator", "m_mutex" ),
	m_device( device ),
	m_heapType( heapType ),
	m_pageEntryCount( pageEntryCount ),
	m_descriptorsInUse( 0 ),
	m_pages( maxPages, maxPages )
{
	m_heapIncrement = m_device->GetDescriptorHandleIncrementSize( m_heapType );
}

/** */
GlobalDescriptorHeapAllocator::~GlobalDescriptorHeapAllocator()
{
	// If this isn't 0, then some views haven't been freed yet
	// If necessary, we could add a debug string to DescriptorEntry and dump those
	// if we want better object tracking
	CCP_ASSERT( m_descriptorsInUse == 0 );
}

/** Allocate an entry */
GlobalDescriptorHeapPage::DescriptorEntry* GlobalDescriptorHeapAllocator::Allocate()
{
	CcpAutoMutex lock( m_mutex );

	HeapPageEntry* page = m_pages.GetFirstFree();
	if( page == nullptr )
	{
		// FATAL: Totally out of memory,, is there a better way of handling this?
		CCP_ASSERT( page != nullptr );
		return nullptr;
	}

	// Pages start off uninitialized... we might want to change this behavior in the future
	if( page->m_page == nullptr )
	{
		CComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NumDescriptors = m_pageEntryCount;
		heapDesc.Type = m_heapType;

		HRESULT hr = m_device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &descriptorHeap ) );
		if( hr != S_OK )
		{
			// FATAL: Can't back an allocation with a heap
			CCP_ASSERT( hr == S_OK );
			return nullptr;
		}

		page->m_page = std::make_unique<GlobalDescriptorHeapPage>( descriptorHeap, m_pageEntryCount, m_heapIncrement );
	}

	// m_page->getFirstFree() should never return a full page!
	CCP_ASSERT( !page->m_page->IsFull() );

	// Allocate entry
	GlobalDescriptorHeapPage::DescriptorEntry* entry = page->m_page->Allocate();
	CCP_ASSERT( entry != nullptr );

	// Assign a tag, so we can quickly free this entry later
	entry->m_pageTag = page;

	// If the page is now full, mark it as such
	if( page->m_page->IsFull() )
	{
		// Note: Nothing is allocated, the free list will just move the first entry
		// over into the 'in use' list
		m_pages.Allocate();
	}

	m_descriptorsInUse++;

	return entry;
}

/** Free an entry */
void GlobalDescriptorHeapAllocator::Free( GlobalDescriptorHeapPage::DescriptorEntry* entry )
{
	CcpAutoMutex lock( m_mutex );

	CCP_ASSERT( entry->m_pageTag != nullptr );
	CCP_ASSERT( entry->m_owner != nullptr );

	// Recover page handle
	HeapPageEntry* page = reinterpret_cast<HeapPageEntry*>( entry->m_pageTag );
	CCP_ASSERT( page->m_page != nullptr );
	m_pages.ValidateEntry( page );

	// If the page is currently full, then it'll be marked as InUse
	// it'll definitely not be full after this operation, so we can move it over
	if( page->m_page->IsFull() )
	{
		m_pages.Free( page );
	}

	// Free entry
	page->m_page->Free( entry );

	CCP_ASSERT( m_descriptorsInUse > 0 );
	m_descriptorsInUse--;

	CCP_ASSERT( !page->m_page->IsFull() );
}

//////////////////////////////////////////////////////////////////////////
// GlobalDescriptorHeapPage
//////////////////////////////////////////////////////////////////////////

/** */
GlobalDescriptorHeapPage::DescriptorEntry::DescriptorEntry( uint32_t entryIdx, const GlobalDescriptorHeapPage::DescriptorInitArgs& initArgs ) :
	m_owner( nullptr ),
	m_pageTag( nullptr )
{
	m_offsetCPU.ptr = initArgs.m_baseOffsetCPU.ptr + entryIdx * initArgs.m_entrySize;
	m_offsetGPU.ptr = initArgs.m_baseOffsetGPU.ptr + entryIdx * initArgs.m_entrySize;
}

/** */
GlobalDescriptorHeapPage::GlobalDescriptorHeapPage( CComPtr<ID3D12DescriptorHeap> descriptorHeap, UINT entryCount, UINT entrySize ) :
	m_descriptorHeap( descriptorHeap )
{
	D3D12_CPU_DESCRIPTOR_HANDLE baseOffsetCPU = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE baseOffsetGPU = { 0 };

	DescriptorInitArgs init( baseOffsetCPU, baseOffsetGPU, entrySize );
	m_freeList = std::make_shared<DescriptorList>( entryCount, init );
}

/** Allocate an entry */
GlobalDescriptorHeapPage::DescriptorEntry* GlobalDescriptorHeapPage::Allocate()
{
	DescriptorEntry* entry = m_freeList->Allocate();
	entry->m_owner = this;

	return entry;
}

/** Free an entry */
void GlobalDescriptorHeapPage::Free( GlobalDescriptorHeapPage::DescriptorEntry* entry )
{
	CCP_ASSERT( entry->m_owner == this );

	m_freeList->Free( entry );
}


GpuVisibleDescriptorAllocator::GpuVisibleDescriptorAllocator( ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t initialSize ) :
	m_device( device ),
	m_mutex( "GpuVisibleDescriptorAllocator", "m_mutex" ),
	m_size( initialSize ),
	m_sizeIncrement( initialSize ),
	m_frameNumber( 0 ),
	m_type( type ),
	m_entrySize( device->GetDescriptorHandleIncrementSize( type ) )
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NumDescriptors = initialSize;
	heapDesc.Type = type;

	HRESULT hr = m_device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &m_gpuHeap ) );
	if( hr != S_OK )
	{
		// FATAL: Can't back an allocation with a heap
		CCP_ASSERT( hr == S_OK );
	}
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = m_device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &m_cpuHeap ) );
	if( hr != S_OK )
	{
		// FATAL: Can't back an allocation with a heap
		CCP_ASSERT( hr == S_OK );
		m_gpuHeap = nullptr;
	}

	auto cpuHandleStart = m_gpuHeap->GetCPUDescriptorHandleForHeapStart();
	auto gpuHandleStart = m_gpuHeap->GetGPUDescriptorHandleForHeapStart();

	m_freeList = std::make_unique<DescriptorList>( initialSize, GlobalDescriptorHeapPage::DescriptorInitArgs( cpuHandleStart, gpuHandleStart, m_entrySize ) );
}

GpuVisibleDescriptorAllocator::~GpuVisibleDescriptorAllocator()
{
}

ID3D12DescriptorHeap* GpuVisibleDescriptorAllocator::GetGpuVisibleHeap() const
{
	return m_gpuHeap;
}

GlobalDescriptorHeapPage::DescriptorEntry* GpuVisibleDescriptorAllocator::Allocate()
{
	CcpAutoMutex lock( m_mutex );

	auto entry = m_freeList->Allocate();
	if( !entry )
	{
		if( Resize() )
		{
			entry = m_freeList->Allocate();
		}
	}
	return entry;
}

bool GpuVisibleDescriptorAllocator::Resize()
{
	CComPtr<ID3D12DescriptorHeap> gpuHeap;
	CComPtr<ID3D12DescriptorHeap> cpuHeap;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NumDescriptors = m_size + m_sizeIncrement;
	heapDesc.Type = m_type;

	HRESULT hr = m_device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &gpuHeap ) );
	if( hr != S_OK )
	{
		// FATAL: Can't back an allocation with a heap
		CCP_ASSERT( hr == S_OK );
		return false;
	}
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = m_device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &cpuHeap ) );
	if( hr != S_OK )
	{
		// FATAL: Can't back an allocation with a heap
		CCP_ASSERT( hr == S_OK );
		return false;
	}

	auto dest = gpuHeap->GetCPUDescriptorHandleForHeapStart();
	auto source = m_cpuHeap->GetCPUDescriptorHandleForHeapStart();

	m_device->CopyDescriptorsSimple( m_size, dest, source, m_type );
	dest = cpuHeap->GetCPUDescriptorHandleForHeapStart();
	m_device->CopyDescriptorsSimple( m_size, dest, source, m_type );

	auto oldBaseCpu = m_gpuHeap->GetCPUDescriptorHandleForHeapStart();
	auto oldBaseGpu = m_gpuHeap->GetGPUDescriptorHandleForHeapStart();
	auto newBaseCpu = gpuHeap->GetCPUDescriptorHandleForHeapStart();
	auto newBaseGpu = gpuHeap->GetGPUDescriptorHandleForHeapStart();

	m_freeList->EnumerateEntries( [&]( auto& entry ) {
		entry.m_offsetCPU.ptr = ( entry.m_offsetCPU.ptr - oldBaseCpu.ptr ) + newBaseCpu.ptr;
		entry.m_offsetGPU.ptr = ( entry.m_offsetGPU.ptr - oldBaseGpu.ptr ) + newBaseGpu.ptr;
	} );
	m_freeList->AddPage( m_sizeIncrement, GlobalDescriptorHeapPage::DescriptorInitArgs( newBaseCpu, newBaseGpu, m_entrySize ) );

	m_size += m_sizeIncrement;
	m_pendingReleaseHeaps.push_back( { m_gpuHeap, m_frameNumber } );
	m_pendingReleaseHeaps.push_back( { m_cpuHeap, m_frameNumber } );

	m_gpuHeap = gpuHeap;
	m_cpuHeap = cpuHeap;

	return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE GpuVisibleDescriptorAllocator::GetDescriptorInCpuHeap( GlobalDescriptorHeapPage::DescriptorEntry* entry )
{
	m_freeList->ValidateEntry( entry );
	auto gpuBase = m_gpuHeap->GetCPUDescriptorHandleForHeapStart();
	auto cpuBase = m_cpuHeap->GetCPUDescriptorHandleForHeapStart();
	return {
		entry->m_offsetCPU.ptr - gpuBase.ptr + cpuBase.ptr
	};
}

uint32_t GpuVisibleDescriptorAllocator::GetIndexInHeap( GlobalDescriptorHeapPage::DescriptorEntry* entry ) const
{
	auto gpuBase = m_gpuHeap->GetGPUDescriptorHandleForHeapStart();
	return uint32_t( ( entry->m_offsetGPU.ptr - gpuBase.ptr ) / m_entrySize );
}

void GpuVisibleDescriptorAllocator::Free( GlobalDescriptorHeapPage::DescriptorEntry* entry )
{
	CcpAutoMutex lock( m_mutex );

	m_pendingFree.push_back( { entry, m_frameNumber } );
}

void GpuVisibleDescriptorAllocator::SetFrameIndices( uint64_t recordingFrame, uint64_t renderedFrame )
{
	CcpAutoMutex lock( m_mutex );
	for( size_t i = 0; i < m_pendingFree.size(); ++i )
	{
		if( m_pendingFree[i].second < renderedFrame )
		{
			m_freeList->Free( m_pendingFree[i].first );
			m_pendingFree[i] = m_pendingFree.back();
			m_pendingFree.pop_back();
			--i;
		}
	}
	for( size_t i = 0; i < m_pendingReleaseHeaps.size(); ++i )
	{
		if( m_pendingReleaseHeaps[i].second < renderedFrame )
		{
			m_pendingReleaseHeaps[i] = m_pendingReleaseHeaps.back();
			m_pendingReleaseHeaps.pop_back();
			--i;
		}
	}
	m_frameNumber = recordingFrame;
}

#endif
