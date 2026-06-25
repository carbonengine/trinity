// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "FrameLocalUploadBufferAllocatorDx12.h"
#include "../Tr2PrimaryRenderContextDx12.h"


ConstantBufferAllocator::ConstantBufferAllocator() :
	m_device( nullptr ),
	m_offset( 0 )
{
	m_page.size = CONST_PAGE_SIZE;
	m_spillPage.size = CONST_PAGE_SIZE;
}

void ConstantBufferAllocator::Initialize( ID3D12Device* device )
{
	m_device = device;
	CreatePage( m_page );
	m_offset = 0;
	m_spillOffset = m_page.size;
}

void ConstantBufferAllocator::Reset( std::vector<CComPtr<ID3D12Resource>>& releaseLater )
{
	if( m_spillPage.buffer )
	{
		releaseLater = m_retiredBuffers;
		releaseLater.push_back( m_page.buffer );
		m_retiredBuffers.clear();

		m_page = m_spillPage;
		m_spillPage.buffer = nullptr;
	}
	m_spillOffset = m_page.size;
	m_offset = 0;
}

uint32_t ConstantBufferAllocator::GetTotalUploadedSize() const
{
	return m_offset;
}

ConstantBufferAllocator::Entry ConstantBufferAllocator::Allocate( const void* data, uint32_t size )
{
	auto dataSize = size;
	size = ( size + ( CONST_ALIGNMENT - 1 ) ) & ~( CONST_ALIGNMENT - 1 );

	Entry entry;

	uint64_t offset = m_offset.fetch_add( size, std::memory_order_relaxed );
	if( offset + size <= m_page.size )
	{
		entry.m_cpuAddr = m_page.cpuAddr + offset;
		entry.m_gpuAddr = m_page.gpuAddr + offset;
	}
	else
	{
		std::lock_guard lock( m_spillMutex );

		if( !m_spillPage.buffer || m_spillOffset + size > m_spillPage.size )
		{
			m_spillPage.size *= 2;
			CreatePage( m_spillPage );
		}
		entry.m_cpuAddr = m_spillPage.cpuAddr + m_spillOffset;
		entry.m_gpuAddr = m_spillPage.gpuAddr + m_spillOffset;
		m_spillOffset += size;
	}
	if( data )
	{
		memcpy( entry.m_cpuAddr, data, dataSize );
	}
	return entry;
}

void ConstantBufferAllocator::CreatePage( Page& page )
{
	CComPtr<ID3D12Resource> buffer;

	D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
	D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, page.size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

	HRESULT hr = m_device->CreateCommittedResource( &heap, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS( &buffer ) );
	CCP_ASSERT( hr == S_OK );
	if( hr != S_OK )
	{
		return;
	}

	const char* name = "Constant upload heap";
	buffer->SetPrivateData( WKPDID_D3DDebugObjectName, UINT( strlen( name ) ), name );

	D3D12_RANGE range = { 0, 0 };
	buffer->Map( 0, &range, (void**)&page.cpuAddr );

	page.gpuAddr = buffer->GetGPUVirtualAddress();
	if( page.buffer )
	{
		m_retiredBuffers.push_back( page.buffer );
	}
	page.buffer = buffer;
}

#endif