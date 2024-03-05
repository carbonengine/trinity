#include "StdAfx.h"

#ifdef _WIN32

#include "TriStepRemoteSync.h"

static std::string appendIdToString( const char* str, int id )
{
	char buffer[33];
	_itoa_s( id, &buffer[0], 33, 10 );
	return std::string( str ).append( std::string( buffer ) );
}

TriStepRemoteSync::TriStepRemoteSync( IRoot* lockobj ):
	m_begin(NULL),
	m_end(NULL),
	m_init(NULL),
	m_id( -1 )
{
	
}

TriStepRemoteSync::~TriStepRemoteSync()
{
	if( m_begin )
	{
		CloseHandle( m_begin );
	}

	if( m_end )
	{
		CloseHandle( m_end );
	}

	if( m_init )
	{
		CloseHandle( m_init );
	}

}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer. 
// Arguments:
//   depthStencil - Initial value of id step attribute
// --------------------------------------------------------------------------------------
void TriStepRemoteSync::py__init__( Be::OptionalWithDefaultValue<int, -1> id )
{
	SetId( id );
}

TriStepResult TriStepRemoteSync::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	// An event to stall the beginning of the rendering til set
	if( m_begin == NULL )
	{		
		std::string beginRenderName;
		if( m_id >= 0 )
		{		
			beginRenderName = appendIdToString( "Trinity_beginRender", m_id );			
		}
		else
		{
			beginRenderName = "Trinity_beginRender";
		}
		m_begin = OpenEventA( EVENT_ALL_ACCESS, TRUE, beginRenderName.c_str() );
		if( m_begin == NULL )
		{
			return RS_FAILED;
		}
	}	

	// An event to indicate that the rendering loop has gone a full rotation
	if( m_end == NULL )
	{
		std::string endRenderName;		
		if( m_id >= 0 )
		{		
			endRenderName = appendIdToString( "Trinity_endRender", m_id );
		}
		else
		{
			endRenderName = "Trinity_endRender";
		}
		m_end = OpenEventA( EVENT_ALL_ACCESS, TRUE, endRenderName.c_str() );
		if( m_end == NULL )
		{
			return RS_FAILED;
		}		
	}

	// Fire of a synchronization event indicating that the renderjob has been created and initialized.
	if( m_init == NULL )
	{
		std::string initRenderName;		
		if( m_id >= 0 )
		{		
			initRenderName = appendIdToString( "Trinity_initRender", m_id );
		}
		else
		{
			initRenderName = "Trinity_initRender";
		}
		m_init = OpenEventA( EVENT_ALL_ACCESS, TRUE, initRenderName.c_str() );
		if( m_init != NULL )
		{
			SetEvent( m_init );
		}		
	}

	PulseEvent( m_end );

	// Wait until we get the signal to begin
	if( WaitForSingleObject( m_begin, 10 ) == WAIT_OBJECT_0 )
	{
		//
	}
	
	return RS_OK;
}

#endif
