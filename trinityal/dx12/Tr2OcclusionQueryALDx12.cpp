// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2OcclusionQueryALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Utilities.h"


namespace TrinityALImpl
{
Tr2OcclusionQueryAL::Tr2OcclusionQueryAL() :
	m_owner( nullptr ),
	m_frameIndex( 0 )
{
}

Tr2OcclusionQueryAL::~Tr2OcclusionQueryAL()
{
	Destroy();
}

ALResult Tr2OcclusionQueryAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}

	CComPtr<ID3D12QueryHeap> query;
	CComPtr<ID3D12Resource> result;
	CComPtr<ID3D12Fence> fence;

	D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
	queryHeapDesc.Count = 1;
	queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
	CR_RETURN_HR( renderContext.m_device->CreateQueryHeap( &queryHeapDesc, IID_PPV_ARGS( &query ) ) );

	auto scratchHeap = TrinityALImpl::HeapDesc( D3D12_HEAP_TYPE_READBACK );
	auto bufferDesc = TrinityALImpl::BufferDesc( 8 );
	CR_RETURN_HR( renderContext.m_device->CreateCommittedResource(
		&scratchHeap,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS( &result ) ) );

	m_query = query;
	m_result = result;
	m_owner = &renderContext;

	return S_OK;
}

bool Tr2OcclusionQueryAL::IsValid() const
{
	return m_query != nullptr;
}

void Tr2OcclusionQueryAL::Destroy()
{
	if( m_query )
	{
		RELEASE_LATER( m_owner, m_query );
		m_query = nullptr;
	}
	if( m_result )
	{
		RELEASE_LATER( m_owner, m_result );
		m_result = nullptr;
	}
	m_owner = nullptr;
}

ALResult Tr2OcclusionQueryAL::Begin( Tr2RenderContextAL& renderContext )
{
	if( !m_query )
	{
		return E_INVALIDCALL;
	}
	if( !renderContext.m_commandList )
	{
		return E_INVALIDARG;
	}
	renderContext.m_commandList->BeginQuery( m_query, D3D12_QUERY_TYPE_OCCLUSION, 0 );
	return S_OK;
}

ALResult Tr2OcclusionQueryAL::End( Tr2RenderContextAL& renderContext )
{
	if( !m_query )
	{
		return E_INVALIDCALL;
	}
	if( !renderContext.m_commandList )
	{
		return E_INVALIDARG;
	}
	renderContext.m_commandList->EndQuery( m_query, D3D12_QUERY_TYPE_OCCLUSION, 0 );
	renderContext.m_commandList->ResolveQueryData( m_query, D3D12_QUERY_TYPE_OCCLUSION, 0, 1, m_result, 0 );
	m_frameIndex = m_owner->GetRecordingFrameNumber();
	return S_OK;
}

ALResult Tr2OcclusionQueryAL::GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, ::Tr2OcclusionQueryAL::WaitMode waitMode )
{
	if( !m_result )
	{
		return E_INVALIDCALL;
	}
	if( waitMode == ::Tr2OcclusionQueryAL::WAIT )
	{
		renderContext.FlushAndSyncDx12();
	}
	if( m_owner->GetRenderedFrameNumber() < m_frameIndex )
	{
		return S_FALSE;
	}
	D3D12_RANGE readRange = { 0, 8 };
	void* data;
	CR_RETURN_HR( m_result->Map( 0, &readRange, &data ) );
	count = uint32_t( *static_cast<uint64_t*>( data ) );
	D3D12_RANGE writeRange = { 0, 0 };
	m_result->Unmap( 0, &writeRange );
	return S_OK;
}

Tr2ALMemoryType Tr2OcclusionQueryAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

void Tr2OcclusionQueryAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2OcclusionQueryAL";
	description["name"] = m_name;
}

ALResult Tr2OcclusionQueryAL::SetName( const char* name )
{
	m_name = name;
	SetDebugName( m_query, name );
	SetDebugName( m_result, name );
	return S_OK;
}
}

#endif