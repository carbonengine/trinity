// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2GpuTimerALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Utilities.h"


namespace TrinityALImpl
{
Tr2GpuTimerAL::Tr2GpuTimerAL() :
	m_frameIndex( 0 ),
	m_owner( nullptr ),
	m_lastTime( -1 ),
	m_state( UNINITIALIZED )
{
}

Tr2GpuTimerAL::~Tr2GpuTimerAL()
{
	Destroy();
}

ALResult Tr2GpuTimerAL::Create( Tr2PrimaryRenderContextAL& renderContext )
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
	queryHeapDesc.Count = 2;
	queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	CR_RETURN_HR( renderContext.m_device->CreateQueryHeap( &queryHeapDesc, IID_PPV_ARGS( &query ) ) );

	auto scratchHeap = TrinityALImpl::HeapDesc( D3D12_HEAP_TYPE_READBACK );
	auto bufferDesc = TrinityALImpl::BufferDesc( 8 * 2 );
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
	m_state = READY;

	return S_OK;
}

void Tr2GpuTimerAL::Destroy()
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
	m_lastTime = -1;
	m_state = UNINITIALIZED;
}

bool Tr2GpuTimerAL::IsValid() const
{
	return m_query != nullptr;
}

bool Tr2GpuTimerAL::Begin( Tr2RenderContextAL& renderContext )
{
	if( !m_query || m_state != READY )
	{
		return false;
	}
	if( !renderContext.m_commandList )
	{
		return false;
	}
	renderContext.m_commandList->EndQuery( m_query, D3D12_QUERY_TYPE_TIMESTAMP, 0 );
	m_state = BEGIN_ISSUED;
	return true;
}

void Tr2GpuTimerAL::End( Tr2RenderContextAL& renderContext )
{
	if( !m_query || m_state != BEGIN_ISSUED )
	{
		return;
	}
	if( !renderContext.m_commandList )
	{
		return;
	}
	renderContext.m_commandList->EndQuery( m_query, D3D12_QUERY_TYPE_TIMESTAMP, 1 );
	renderContext.m_commandList->ResolveQueryData( m_query, D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, m_result, 0 );
	m_frameIndex = m_owner->GetRecordingFrameNumber();
	m_state = END_ISSUED;
}

float Tr2GpuTimerAL::GetTime( Tr2RenderContextAL& )
{
	if( !m_result )
	{
		return m_lastTime;
	}
	if( m_state != END_ISSUED )
	{
		return m_lastTime;
	}
	if( m_owner->GetRenderedFrameNumber() < m_frameIndex )
	{
		return m_lastTime;
	}
	D3D12_RANGE readRange = { 0, 8 };
	void* data;
	if( FAILED( m_result->Map( 0, &readRange, &data ) ) )
	{
		return m_lastTime;
	}
	auto t0 = static_cast<uint64_t*>( data )[0];
	auto t1 = static_cast<uint64_t*>( data )[1];
	D3D12_RANGE writeRange = { 0, 0 };
	m_result->Unmap( 0, &writeRange );

	UINT64 freq;
	if( FAILED( m_owner->m_commandQueue->GetTimestampFrequency( &freq ) ) )
	{
		return m_lastTime;
	}
	m_lastTime = float( double( t1 - t0 ) / double( freq ) );
	m_state = READY;

	return m_lastTime;
}

bool Tr2GpuTimerAL::operator==( const Tr2GpuTimerAL& other ) const
{
	return this == &other;
}

Tr2ALMemoryType Tr2GpuTimerAL::GetMemoryClass() const
{
	return AL_MEMORY_VIDEO;
}

void Tr2GpuTimerAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2GpuTimerAL";
	description["name"] = m_name;
}

ALResult Tr2GpuTimerAL::SetName( const char* name )
{
	m_name = name;
	SetDebugName( m_query, name );
	SetDebugName( m_result, name );
	return S_OK;
}
}


#endif