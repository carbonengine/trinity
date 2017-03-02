#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2FenceALDx11.h"

Tr2FenceAL::Tr2FenceAL()
{
}

Tr2FenceAL::~Tr2FenceAL()
{
}

ALResult Tr2FenceAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();
	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}
	D3D11_QUERY_DESC desc;
	desc.Query = D3D11_QUERY_EVENT;
	desc.MiscFlags = 0;
	return renderContext.m_d3dDevice11->CreateQuery( &desc, &m_query );
}

void Tr2FenceAL::Destroy()
{
	m_query = nullptr;
}

bool Tr2FenceAL::IsValid() const
{
	return m_query != nullptr;
}

ALResult Tr2FenceAL::PutFence( Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	renderContext.m_context->End( m_query );
	return S_OK;
}

ALResult Tr2FenceAL::IsReached( bool& isReached, Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	HRESULT hr = renderContext.m_context->GetData( m_query, nullptr, 0, D3D11_ASYNC_GETDATA_DONOTFLUSH );
	CR_RETURN_HR( hr );
	isReached = hr == S_OK;
	return S_OK;
}

ALResult Tr2FenceAL::Wait( Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	while( true )
	{
		HRESULT hr = renderContext.m_context->GetData( m_query, nullptr, 0, 0 );
		CR_RETURN_HR( hr );
		if( hr == S_OK )
		{
			break;
		}
	}
	return S_OK;
}


#endif