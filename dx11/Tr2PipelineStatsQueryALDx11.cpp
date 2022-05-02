#include "StdAfx.h"
#if TRINITY_PLATFORM==TRINITY_DIRECTX11
#include "Tr2PipelineStatsQueryALDx11.h"
#include "Tr2PrimaryRenderContextDx11.h"


namespace
{

struct Field
{
	const char* label;
	ptrdiff_t offset;
	const char* description;
};

static const Field s_fields[] = {
	{ "IAVertices", offsetof( D3D11_QUERY_DATA_PIPELINE_STATISTICS, IAVertices ), "Number of vertices read by input assembler" },
	{ "IAPrimitives", offsetof( D3D11_QUERY_DATA_PIPELINE_STATISTICS, IAPrimitives ), "Number of primitives read by the input assembler" },
	{ "VSInvocations", offsetof( D3D11_QUERY_DATA_PIPELINE_STATISTICS, VSInvocations ), "Number of vertex shader invocations" },
	{ "GSInvocations", offsetof( D3D11_QUERY_DATA_PIPELINE_STATISTICS, GSInvocations ), "Number of geometry shader invocations" },
	{ "GSPrimitives", offsetof( D3D11_QUERY_DATA_PIPELINE_STATISTICS, GSPrimitives ), "Number of geometry shader output primitives" },
	{ "CInvocations", offsetof( D3D11_QUERY_DATA_PIPELINE_STATISTICS, CInvocations ), "Number of primitives that were sent to the rasterizer. When the rasterizer is disabled, this will not increment." },
	{ "CPrimitives", offsetof( D3D11_QUERY_DATA_PIPELINE_STATISTICS, CPrimitives ), "Number of primitives that were rendered. This may be larger or smaller than CInvocations because after a primitive is "
																					"clipped sometimes it is either broken up into more than one primitive or completely culled." },
	{ "PSInvocations", offsetof( D3D11_QUERY_DATA_PIPELINE_STATISTICS, PSInvocations ), "Number of pixel shader invocations" },
	{ "HSInvocations", offsetof( D3D11_QUERY_DATA_PIPELINE_STATISTICS, HSInvocations ), "Number of hull shader invocations" },
	{ "DSInvocations", offsetof( D3D11_QUERY_DATA_PIPELINE_STATISTICS, DSInvocations ), "Number of domain shader invocations" },
	{ "CSInvocations", offsetof( D3D11_QUERY_DATA_PIPELINE_STATISTICS, CSInvocations ), "Number of compute shader invocations" },
};
}


namespace TrinityALImpl
{

Tr2PipelineStatsQueryAL::Tr2PipelineStatsQueryAL()
{

}

ALResult Tr2PipelineStatsQueryAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();

	if( !renderContext.m_d3dDevice11 )
	{
		return E_FAIL;
	}

	D3D11_QUERY_DESC desc;
	desc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
	desc.MiscFlags = 0;
	return renderContext.m_d3dDevice11->CreateQuery( &desc, &m_query );
}

bool Tr2PipelineStatsQueryAL::IsValid() const
{
	return m_query != nullptr;
}

void Tr2PipelineStatsQueryAL::Destroy()
{
	m_query = nullptr;
}

ALResult Tr2PipelineStatsQueryAL::Begin( Tr2RenderContextAL& renderContext )
{
	if( m_query == nullptr )
	{
		return E_INVALIDARG;
	}
	if( !renderContext.m_context )
	{
		return E_FAIL;
	}
	renderContext.m_context->Begin( m_query );
	return S_OK;
}

ALResult Tr2PipelineStatsQueryAL::End( Tr2RenderContextAL& renderContext )
{
	if( m_query == nullptr )
	{
		return E_INVALIDARG;
	}
	if( !renderContext.m_context )
	{
		return E_FAIL;
	}
	renderContext.m_context->End( m_query );
	return S_OK;
}

ALResult Tr2PipelineStatsQueryAL::GetStats( Tr2PipelineStatsDataAL& data, Tr2RenderContextAL& renderContext )
{
	if( m_query == nullptr )
	{
		return E_INVALIDARG;
	}
	if( !renderContext.m_context )
	{
		return E_FAIL;
	}
	return renderContext.m_context->GetData( m_query, (void*)&data.data, sizeof( data.data ), D3D11_ASYNC_GETDATA_DONOTFLUSH );
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
	return S_OK;
}

}

#endif