// Copyright © 2019 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "GlobalDescriptorHeapAllocatorDx12.h"

/** Base for *View objects which point to a descriptor heap */
class DescriptorHeapViewDx12
{
public:
	/** */
	DescriptorHeapViewDx12( GlobalDescriptorHeapAllocator* allocator, GlobalDescriptorHeapPage::DescriptorEntry* heapEntry );

	/** */
	virtual ~DescriptorHeapViewDx12();

	/** Get the underlying CPU descriptor handle */
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const
	{
		CCP_ASSERT( m_entry != nullptr );
		return m_entry->m_offsetCPU;
	};

	/** Get the underlying GPU descriptor handle */
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const
	{
		CCP_ASSERT( m_entry != nullptr );
		return m_entry->m_offsetGPU;
	};

protected:
	GlobalDescriptorHeapPage::DescriptorEntry* m_entry;
	GlobalDescriptorHeapAllocator* m_allocator;
};

/** ShaderResourceView object */
class ShaderResourceViewDx12
{
public:
	ShaderResourceViewDx12( GpuVisibleDescriptorAllocator* allocator, GlobalDescriptorHeapPage::DescriptorEntry* heapEntry );
	virtual ~ShaderResourceViewDx12();


	/** Get the underlying CPU descriptor handle */
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const
	{
		CCP_ASSERT( m_entry != nullptr );
		return m_entry->m_offsetCPU;
	};

	/** Get the underlying GPU descriptor handle */
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const
	{
		CCP_ASSERT( m_entry != nullptr );
		return m_entry->m_offsetGPU;
	};

	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorInCpuHeap() const
	{
		return m_allocator->GetDescriptorInCpuHeap( m_entry );
	}

	uint32_t GetIndexInHeap() const
	{
		return m_index;
	}

private:
	GlobalDescriptorHeapPage::DescriptorEntry* m_entry;
	GpuVisibleDescriptorAllocator* m_allocator;
	uint32_t m_index;
};

class UnorderedAccessViewDx12 : public ShaderResourceViewDx12
{
public:
	UnorderedAccessViewDx12( GpuVisibleDescriptorAllocator* allocator, GlobalDescriptorHeapPage::DescriptorEntry* heapEntry ) :
		ShaderResourceViewDx12( allocator, heapEntry )
	{
	}
};

/** SamplerState object */
class SamplerStateDx12 : public ShaderResourceViewDx12
{
public:
	SamplerStateDx12( GpuVisibleDescriptorAllocator* allocator, GlobalDescriptorHeapPage::DescriptorEntry* heapEntry );
	virtual ~SamplerStateDx12();
};

/** RenderTargetView object */
class RenderTargetViewDx12 : public DescriptorHeapViewDx12
{
public:
	RenderTargetViewDx12( GlobalDescriptorHeapAllocator* allocator, GlobalDescriptorHeapPage::DescriptorEntry* heapEntry );
	virtual ~RenderTargetViewDx12();
};

/** DepthStencilView object */
class DepthStencilViewDx12 : public DescriptorHeapViewDx12
{
public:
	DepthStencilViewDx12( GlobalDescriptorHeapAllocator* allocator, GlobalDescriptorHeapPage::DescriptorEntry* heapEntry );
	virtual ~DepthStencilViewDx12();
};

#endif
