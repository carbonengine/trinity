#include "StdAfx.h"
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include "Tr2PipelineStatsQueryALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Utilities.h"


namespace
{
struct Field
{
	const char* label;
	ptrdiff_t offset;
	const char* description;
};

static const Field s_fields[] = {
	{ "IAVertices", offsetof( D3D12_QUERY_DATA_PIPELINE_STATISTICS, IAVertices ), "Number of vertices read by input assembler" },
	{ "IAPrimitives", offsetof( D3D12_QUERY_DATA_PIPELINE_STATISTICS, IAPrimitives ), "Number of primitives read by the input assembler" },
	{ "VSInvocations", offsetof( D3D12_QUERY_DATA_PIPELINE_STATISTICS, VSInvocations ), "Number of vertex shader invocations" },
	{ "GSInvocations", offsetof( D3D12_QUERY_DATA_PIPELINE_STATISTICS, GSInvocations ), "Number of geometry shader invocations" },
	{ "GSPrimitives", offsetof( D3D12_QUERY_DATA_PIPELINE_STATISTICS, GSPrimitives ), "Number of geometry shader output primitives" },
	{ "CInvocations", offsetof( D3D12_QUERY_DATA_PIPELINE_STATISTICS, CInvocations ), "Number of primitives that were sent to the rasterizer. When the rasterizer is disabled, this will not increment." },
	{ "CPrimitives", offsetof( D3D12_QUERY_DATA_PIPELINE_STATISTICS, CPrimitives ), "Number of primitives that were rendered. This may be larger or smaller than CInvocations because after a primitive is "
																					"clipped sometimes it is either broken up into more than one primitive or completely culled." },
	{ "PSInvocations", offsetof( D3D12_QUERY_DATA_PIPELINE_STATISTICS, PSInvocations ), "Number of pixel shader invocations" },
	{ "HSInvocations", offsetof( D3D12_QUERY_DATA_PIPELINE_STATISTICS, HSInvocations ), "Number of hull shader invocations" },
	{ "DSInvocations", offsetof( D3D12_QUERY_DATA_PIPELINE_STATISTICS, DSInvocations ), "Number of domain shader invocations" },
	{ "CSInvocations", offsetof( D3D12_QUERY_DATA_PIPELINE_STATISTICS, CSInvocations ), "Number of compute shader invocations" },
};
}


namespace TrinityALImpl
{

Tr2PipelineStatsQueryAL::Tr2PipelineStatsQueryAL() :
	m_owner( nullptr ),
	m_frameIndex( 0 )
{
}

Tr2PipelineStatsQueryAL::~Tr2PipelineStatsQueryAL()
{
	Destroy();
}

ALResult Tr2PipelineStatsQueryAL::Create( Tr2PrimaryRenderContextAL& renderContext )
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
	queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
	CR_RETURN_HR( renderContext.m_device->CreateQueryHeap( &queryHeapDesc, IID_PPV_ARGS( &query ) ) );

	auto scratchHeap = TrinityALImpl::HeapDesc( D3D12_HEAP_TYPE_READBACK );
	auto bufferDesc = TrinityALImpl::BufferDesc( sizeof( D3D12_QUERY_DATA_PIPELINE_STATISTICS )  );
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

bool Tr2PipelineStatsQueryAL::IsValid() const
{
	return m_query != nullptr;
}

void Tr2PipelineStatsQueryAL::Destroy()
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

ALResult Tr2PipelineStatsQueryAL::Begin( Tr2RenderContextAL& renderContext )
{
	if( !m_query )
	{
		return E_INVALIDCALL;
	}
	if( !renderContext.m_commandList )
	{
		return E_INVALIDARG;
	}
	renderContext.m_commandList->BeginQuery( m_query, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0 );
	return S_OK;
}

ALResult Tr2PipelineStatsQueryAL::End( Tr2RenderContextAL& renderContext )
{
	if( !m_query )
	{
		return E_INVALIDCALL;
	}
	if( !renderContext.m_commandList )
	{
		return E_INVALIDARG;
	}
	renderContext.m_commandList->EndQuery( m_query, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0 );
	renderContext.m_commandList->ResolveQueryData( m_query, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0, 1, m_result, 0 );
	m_frameIndex = m_owner->GetCurrentFrameIndexDx12();
	return S_OK;
}

ALResult Tr2PipelineStatsQueryAL::GetStats( Tr2PipelineStatsDataAL& data, Tr2RenderContextAL& renderContext )
{
	if( !m_result )
	{
		return E_INVALIDCALL;
	}
	if( m_owner->GetCompletedFrameIndexDx12() < m_frameIndex )
	{
		return S_FALSE;
	}
	D3D12_RANGE readRange = { 0, 8 };
	void* contents;
	CR_RETURN_HR( m_result->Map( 0, &readRange, &contents ) );
	data.data = *static_cast<D3D12_QUERY_DATA_PIPELINE_STATISTICS*>( contents );
	D3D12_RANGE writeRange = { 0, 0 };
	m_result->Unmap( 0, &writeRange );
	return S_OK;
}

size_t Tr2PipelineStatsQueryAL::GetValueCount( const Tr2PipelineStatsDataAL& data )
{
	return sizeof( s_fields ) / sizeof( s_fields[0] );
}

const char* Tr2PipelineStatsQueryAL::GetLabel( const Tr2PipelineStatsDataAL& data, size_t index )
{
	return s_fields[index].label;
}

const char* Tr2PipelineStatsQueryAL::GetDescription( const Tr2PipelineStatsDataAL& data, size_t index )
{
	return s_fields[index].description;
}

::Tr2PipelineStatsQueryAL::Value Tr2PipelineStatsQueryAL::GetValue( const Tr2PipelineStatsDataAL& data, size_t index )
{
	return *reinterpret_cast<const ::Tr2PipelineStatsQueryAL::Value*>( reinterpret_cast<const uint8_t*>( &data.data ) + s_fields[index].offset );
}

void Tr2PipelineStatsQueryAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2PipelineStatsQueryAL";
	description["name"] = m_name;
}

ALResult Tr2PipelineStatsQueryAL::SetName( const char* name )
{
	m_name = name;
	SetDebugName( m_query, name );
	SetDebugName( m_result, name );
	return S_OK;
}

}

#endif