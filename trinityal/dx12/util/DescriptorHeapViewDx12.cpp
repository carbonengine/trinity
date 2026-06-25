// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "DescriptorHeapViewDx12.h"
#include "../Tr2PrimaryRenderContextDx12.h"

/** */
DescriptorHeapViewDx12::DescriptorHeapViewDx12( GlobalDescriptorHeapAllocator* allocator, GlobalDescriptorHeapPage::DescriptorEntry* heapEntry ) :
	m_entry( heapEntry ),
	m_allocator( allocator )
{
}

/** */
DescriptorHeapViewDx12::~DescriptorHeapViewDx12()
{
	if( m_allocator != nullptr )
	{
		m_allocator->Free( m_entry );
	}
}

/** */
ShaderResourceViewDx12::ShaderResourceViewDx12( GpuVisibleDescriptorAllocator* allocator, GlobalDescriptorHeapPage::DescriptorEntry* heapEntry ) :
	m_entry( heapEntry ),
	m_allocator( allocator )
{
	m_index = allocator->GetIndexInHeap( heapEntry );
}

/** */
ShaderResourceViewDx12::~ShaderResourceViewDx12()
{
	if( m_allocator != nullptr )
	{
		m_allocator->Free( m_entry );
	}
}

/** */
SamplerStateDx12::SamplerStateDx12( GpuVisibleDescriptorAllocator* allocator, GlobalDescriptorHeapPage::DescriptorEntry* heapEntry ) :
	ShaderResourceViewDx12( allocator, heapEntry )
{
}

/** */
SamplerStateDx12::~SamplerStateDx12()
{
}

/** */
RenderTargetViewDx12::RenderTargetViewDx12( GlobalDescriptorHeapAllocator* allocator, GlobalDescriptorHeapPage::DescriptorEntry* heapEntry ) :
	DescriptorHeapViewDx12( allocator, heapEntry )
{
}

/** */
RenderTargetViewDx12::~RenderTargetViewDx12()
{
}

/** */
DepthStencilViewDx12::DepthStencilViewDx12( GlobalDescriptorHeapAllocator* allocator, GlobalDescriptorHeapPage::DescriptorEntry* heapEntry ) :
	DescriptorHeapViewDx12( allocator, heapEntry )
{
}

/** */
DepthStencilViewDx12::~DepthStencilViewDx12()
{
}

#endif
