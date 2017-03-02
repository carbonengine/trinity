#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2OcclusionQueryALDx11.h"

ALResult Tr2OcclusionQueryAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();

	if( !renderContext.m_d3dDevice11 )
	{
		return E_FAIL;
	}

	D3D11_QUERY_DESC desc;
	desc.Query = D3D11_QUERY_OCCLUSION;
	desc.MiscFlags = 0;
	return renderContext.m_d3dDevice11->CreateQuery( &desc, &m_query );
}

// -------------------------------------------------------------
// Description:
//	Returns true if the query was successfully created.
// -------------------------------------------------------------
bool Tr2OcclusionQueryAL::IsValid() const
{
	return m_query != nullptr;
}

void Tr2OcclusionQueryAL::Destroy()
{
	m_query = nullptr;
}

// -------------------------------------------------------------
// Description:
//  Begin a rendering query.
// -------------------------------------------------------------
ALResult Tr2OcclusionQueryAL::Begin( Tr2RenderContextAL& renderContext )
{
	if( m_query == nullptr )
	{
		return E_INVALIDARG;
	}
	if( !renderContext.m_context )
	{
		return E_FAIL;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	renderContext.m_context->Begin( m_query );
	return S_OK;
}

// -------------------------------------------------------------
// Description:
//  End a rendering query.
// -------------------------------------------------------------
ALResult Tr2OcclusionQueryAL::End( Tr2RenderContextAL& renderContext )
{
	if( m_query == nullptr )
	{
		return E_INVALIDARG;
	}
	if( !renderContext.m_context )
	{
		return E_FAIL;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	renderContext.m_context->End( m_query );
	return S_OK;
}

// -------------------------------------------------------------
// Description:
//  Get the number of pixels rendering between Begin and End.
//  If WaitMode is DO_NOT_WAIT this checks if the data is available and if not, returns zero.
// -------------------------------------------------------------
ALResult Tr2OcclusionQueryAL::GetPixelCount( Tr2RenderContextAL& renderContext, uint32_t& count, WaitMode waitMode )
{
	if( m_query == nullptr )
	{
		return E_INVALIDARG;
	}
	if( !renderContext.m_context )
	{
		return E_FAIL;
	}
	UINT64 data = 0;
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	HRESULT hr = renderContext.m_context->GetData( m_query, ( void* )&data, sizeof( UINT64 ), waitMode == DO_NOT_WAIT ? D3D11_ASYNC_GETDATA_DONOTFLUSH : 0 );
	if( hr == S_OK )
	{
		count = static_cast<uint32_t>( data );
	}
	return hr;
}

#endif