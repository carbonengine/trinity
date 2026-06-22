// Copyright © 2019 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <cstdint>
#include <memory>
#include <vector>
#include <mutex>


class ConstantBufferAllocator
{
public:
	struct Entry
	{
		void* m_cpuAddr;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddr;
	};

	ConstantBufferAllocator();
	ConstantBufferAllocator( const ConstantBufferAllocator& ) = delete;

	void Initialize( ID3D12Device* device );
	void Reset( std::vector<CComPtr<ID3D12Resource>>& releaseLater );

	Entry Allocate( const void* data, uint32_t size );

	uint32_t GetTotalUploadedSize() const;

	static const uint32_t CONST_PAGE_SIZE = 2 * 1024 * 1024;
	static const uint32_t CONST_ALIGNMENT = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

private:
	struct Page
	{
		CComPtr<ID3D12Resource> buffer;
		uint8_t* cpuAddr = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = 0;
		uint32_t size = 0;
	};

	void CreatePage( Page& page );

	std::atomic<uint32_t> m_offset;
	uint8_t m_gap[64]; // prevent false sharing
	Page m_page;

	std::mutex m_spillMutex;
	Page m_spillPage;
	uint32_t m_spillOffset;
	std::vector<CComPtr<ID3D12Resource>> m_retiredBuffers;

	CComPtr<ID3D12Device> m_device;
};


#endif