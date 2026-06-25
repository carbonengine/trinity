// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

#include "Tr2GpuTimerALDx11.h"
#include "Tr2PrimaryRenderContextDx11.h"

namespace TrinityALImpl
{
Tr2GpuTimerAL::Tr2GpuTimerAL() :
	m_begin( 0xffffffffffffffff ),
	m_end( 0xffffffffffffffff ),
	m_lastTime( -1.f ),
	m_state( UNINITIALIZED )
{
}

ALResult Tr2GpuTimerAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();
	if( !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
	}

	CComPtr<ID3D11Query> beginQuery;
	CComPtr<ID3D11Query> endQuery;
	CComPtr<ID3D11Query> disjointQuery;

	D3D11_QUERY_DESC desc;
	desc.Query = D3D11_QUERY_TIMESTAMP;
	desc.MiscFlags = 0;

	CR_RETURN_HR( renderContext.m_d3dDevice11->CreateQuery( &desc, &beginQuery.p ) );
	CR_RETURN_HR( renderContext.m_d3dDevice11->CreateQuery( &desc, &endQuery.p ) );

	desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	desc.MiscFlags = 0;
	CR_RETURN_HR( renderContext.m_d3dDevice11->CreateQuery( &desc, &disjointQuery.p ) );

	m_beginQuery = beginQuery;
	m_endQuery = endQuery;
	m_disjointQuery = disjointQuery;
	m_state = READY;
	return S_OK;
}

void Tr2GpuTimerAL::Destroy()
{
	m_beginQuery = nullptr;
	m_endQuery = nullptr;
	m_disjointQuery = nullptr;
	m_state = UNINITIALIZED;
	m_lastTime = -1.f;
}

bool Tr2GpuTimerAL::IsValid() const
{
	return m_state != UNINITIALIZED;
}

bool Tr2GpuTimerAL::Begin( Tr2RenderContextAL& renderContext )
{
	if( !m_beginQuery || !m_endQuery || m_state != READY )
	{
		return false;
	}
	renderContext.m_context->Begin( m_disjointQuery );
	renderContext.m_context->End( m_beginQuery );
	m_state = BEGIN_ISSUED;
	return true;
}

void Tr2GpuTimerAL::End( Tr2RenderContextAL& renderContext )
{
	if( !m_beginQuery || !m_endQuery || m_state != BEGIN_ISSUED )
	{
		return;
	}
	renderContext.m_context->End( m_endQuery );
	renderContext.m_context->End( m_disjointQuery );
	m_state = END_ISSUED;
}

float Tr2GpuTimerAL::GetTime( Tr2RenderContextAL& renderContext )
{
	if( !m_beginQuery || !m_endQuery )
	{
		return m_lastTime;
	}
	if( m_state == END_ISSUED )
	{
		switch( renderContext.m_context->GetData( m_beginQuery, &m_begin, sizeof( m_begin ), D3D11_ASYNC_GETDATA_DONOTFLUSH ) )
		{
		case S_OK:
			m_state = BEGIN_RECEIVED;
			break;
		case S_FALSE:
			break;
		default:
			m_state = READY;
			m_lastTime = -1.f;
		}
	}
	if( m_state == BEGIN_RECEIVED )
	{
		switch( renderContext.m_context->GetData( m_endQuery, &m_end, sizeof( m_end ), D3D11_ASYNC_GETDATA_DONOTFLUSH ) )
		{
		case S_OK: {
			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;
			if( FAILED( renderContext.m_context->GetData( m_disjointQuery, &disjoint, sizeof( disjoint ), 0 ) ) )
			{
				m_state = READY;
				m_lastTime = -1.f;
			}
			else
			{
				if( !disjoint.Disjoint )
				{
					m_lastTime = float( double( m_end - m_begin ) / double( disjoint.Frequency ) );
				}
				m_state = READY;
			}
		}
		break;
		case S_FALSE:
			break;
		default:
			m_state = READY;
			m_lastTime = -1.f;
		}
	}
	return m_lastTime;
}

void Tr2GpuTimerAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2GpuTimerAL";
	description["name"] = m_name;
}

ALResult Tr2GpuTimerAL::SetName( const char* name )
{
	m_name = name;
	SetDebugName( m_beginQuery, name );
	SetDebugName( m_endQuery, name );
	SetDebugName( m_disjointQuery, name );
	return S_OK;
}
}

#endif