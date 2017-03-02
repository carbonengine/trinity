#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "Tr2GpuTimerALDx9.h"


Tr2GpuTimerAL::Tr2GpuTimerAL()
	:m_begin( 0xffffffffffffffff ),
	m_end( 0xffffffffffffffff ),
	m_state( UNINITIALIZED ),
	m_lastTime( -1.f )
{
}

ALResult Tr2GpuTimerAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();
	if( !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
	}

	CComPtr<IDirect3DQuery9> beginQuery;
	CComPtr<IDirect3DQuery9> endQuery;
	CComPtr<IDirect3DQuery9> disjointQuery;
	CComPtr<IDirect3DQuery9> frequencyQuery;

	CR_RETURN_HR( renderContext.m_d3dDevice9->CreateQuery( D3DQUERYTYPE_TIMESTAMP, &beginQuery.p ) );
	CR_RETURN_HR( renderContext.m_d3dDevice9->CreateQuery( D3DQUERYTYPE_TIMESTAMP, &endQuery.p ) );
	CR_RETURN_HR( renderContext.m_d3dDevice9->CreateQuery( D3DQUERYTYPE_TIMESTAMPDISJOINT, &disjointQuery.p ) );
	CR_RETURN_HR( renderContext.m_d3dDevice9->CreateQuery( D3DQUERYTYPE_TIMESTAMPFREQ, &frequencyQuery.p ) );

	m_beginQuery = beginQuery;
	m_endQuery = endQuery;
	m_disjointQuery = disjointQuery;
	m_frequencyQuery = frequencyQuery;
	m_state = READY;
	return S_OK;
}

void Tr2GpuTimerAL::Destroy()
{
	m_beginQuery = nullptr;
	m_endQuery = nullptr;
	m_disjointQuery = nullptr;
	m_frequencyQuery = nullptr;
	m_state = UNINITIALIZED;
	m_lastTime = -1.f;
}

bool Tr2GpuTimerAL::IsValid() const
{
	return m_state != UNINITIALIZED;
}

bool Tr2GpuTimerAL::Begin( Tr2RenderContextAL& renderContext )
{
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	if( !m_beginQuery || !m_endQuery || m_state != READY )
	{
		return false;
	}
	m_disjointQuery->Issue( D3DISSUE_BEGIN );
	m_frequencyQuery->Issue( D3DISSUE_END );
	m_beginQuery->Issue( D3DISSUE_END );
	m_state = BEGIN_ISSUED;
	return true;
}

void Tr2GpuTimerAL::End( Tr2RenderContextAL& renderContext )
{
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	if( !m_beginQuery || !m_endQuery || m_state != BEGIN_ISSUED )
	{
		return;
	}
	m_endQuery->Issue( D3DISSUE_END );
	m_disjointQuery->Issue( D3DISSUE_END );
	m_state = END_ISSUED;
}

float Tr2GpuTimerAL::GetTime( Tr2RenderContextAL& renderContext )
{
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	if( !m_beginQuery || !m_endQuery )
	{
		return m_lastTime;
	}
	if( m_state == END_ISSUED )
	{
		switch( m_beginQuery->GetData( &m_begin, sizeof( m_begin ), 0 ) )
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
		switch( m_endQuery->GetData( &m_end, sizeof( m_end ), 0 ) )
		{
		case S_OK:
			{
				BOOL disjoint;
				if( FAILED( m_disjointQuery->GetData( &disjoint, sizeof( disjoint ), 0 ) ) )
				{
					m_lastTime = -1.f;
				}
				else
				{
					if( !disjoint )
					{
						uint64_t frequency;
						if( FAILED( m_frequencyQuery->GetData( &frequency, sizeof( frequency ), 0 ) ) )
						{
							m_lastTime = -1.f;
						}
						else
						{
							m_lastTime = float( double( m_end - m_begin ) / double( frequency ) );
						}
					}
				}
				m_state = READY;
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

void Tr2GpuTimerAL::ReleaseALResource()
{
	Destroy();
}

void Tr2GpuTimerAL::PrepareALResource( Tr2PrimaryRenderContextAL& renderContext )
{
	Create( renderContext );
}


#endif