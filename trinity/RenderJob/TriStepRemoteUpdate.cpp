#include "StdAfx.h"

#ifdef _WIN32

#include "TriStepRemoteUpdate.h"


static std::string appendIdToString( const char* str, int id )
{
	char buffer[33];
	_itoa_s( id, &buffer[0], 33, 10 );
	return std::string( str ).append( std::string( buffer ) );
}

TriStepRemoteUpdate::TriStepRemoteUpdate( IRoot* lockobj ):
	m_sharedMemory( NULL ),
	m_sharedMemoryHandle( NULL ),
	m_needsHndl( NULL ),
	m_readingHndl( NULL ),
	m_writingHndl( NULL ),
	m_id( -1 )
{


}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer. 
// --------------------------------------------------------------------------------------
void TriStepRemoteUpdate::py__init__( 
	TriView* view, 
	TriProjection* projection, 
	TriViewport* viewport, 
	Be::OptionalWithDefaultValue<int, -1> id )
{
	SetData( view, projection, viewport, id );
}

bool TriStepRemoteUpdate::OpenSharedMemoryAndEvents()
{
	// we will store two 4x4 matrices and the dimensions of the viewport
	// 16*4+16*4+6*4 
	int size = 152;

	std::string sharedMemoryName;
	std::string needsUpdateEventName;
	std::string doneReadingEventName;
	std::string doneWritingEventName;

	if( m_id >= 0 )
	{
		sharedMemoryName = appendIdToString( "TrinityViewData", m_id );
		needsUpdateEventName = appendIdToString( "TrinityViewData_needsUpdate", m_id );
		doneReadingEventName = appendIdToString( "TrinityViewData_donereading", m_id );
		doneWritingEventName = appendIdToString( "TrinityViewData_donewriting", m_id );
	}
	else
	{
		sharedMemoryName = "TrinityViewData";
		needsUpdateEventName = "TrinityViewData_needsUpdate";
		doneReadingEventName = "TrinityViewData_donereading";
		doneWritingEventName = "TrinityViewData_donewriting";
	}

	m_sharedMemoryHandle = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, sharedMemoryName.c_str() );

	if( !m_sharedMemoryHandle )
	{
		CloseHandle( m_sharedMemoryHandle );
		return false;
	}

	m_sharedMemory = (const char*)MapViewOfFile(
		m_sharedMemoryHandle,
		FILE_MAP_ALL_ACCESS,
		0, 0,
		size
		);

	if( m_sharedMemory == NULL )
	{ 
		CloseHandle( m_sharedMemoryHandle );

		return false;
	}

	m_needsHndl = OpenEventA( EVENT_ALL_ACCESS, TRUE, needsUpdateEventName.c_str() );
	m_readingHndl = OpenEventA( EVENT_ALL_ACCESS, TRUE, doneReadingEventName.c_str() );
	m_writingHndl = OpenEventA( EVENT_ALL_ACCESS, TRUE, doneWritingEventName.c_str() );

	return true;
}

void TriStepRemoteUpdate::SetData( TriView* view, TriProjection *proj, TriViewport* viewport, int id )
{
	m_view = view;
	m_projection = proj;
	m_viewport = viewport;
	m_id = id;
}

TriStepRemoteUpdate::~TriStepRemoteUpdate(void)
{
	// Release shared handles
	// shared memory
	
	if( m_sharedMemory )
	{
		UnmapViewOfFile (m_sharedMemory );
	}
	if( m_sharedMemoryHandle )
	{
		CloseHandle( m_sharedMemoryHandle );
	}
	
	// Events
	if( m_needsHndl )
	{
		CloseHandle( m_needsHndl );
	}
	if( m_readingHndl )
	{
		CloseHandle( m_readingHndl );
	}
	if( m_writingHndl )
	{
		CloseHandle( m_writingHndl );
	}
}


namespace
{
	struct Dx9Viewport
	{
		uint32_t X;
		uint32_t Y;
		uint32_t Width;
		uint32_t Height;
		float MinZ;
		float MaxZ;
	};
}

TriStepResult TriStepRemoteUpdate::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	// Lock and read
	if( m_sharedMemory != NULL )
	{
		if( WaitForSingleObject( m_needsHndl, 0 ) == WAIT_OBJECT_0 )
		{
			if( WaitForSingleObject( m_writingHndl, 0 ) == WAIT_OBJECT_0 )
			{
				ResetEvent( m_readingHndl );

				// Read data	
				Matrix projMat;
				Matrix viewMat;
				Dx9Viewport viewport;
				int offset = 0;

				memcpy((void*)&projMat, m_sharedMemory, 64);
				offset += 64;
				memcpy((void*)&viewMat, m_sharedMemory+offset, 64);
				offset += 64;
				memcpy((void*)&viewport, m_sharedMemory+offset, 24);				
				m_projection->CustomProjection( projMat );
				m_view->SetTransform( viewMat );
				m_viewport->width = viewport.Width;
				m_viewport->height = viewport.Height;

				SetEvent( m_readingHndl );
				ResetEvent( m_needsHndl );
			}
		}
	}
	else
	{
		if( !OpenSharedMemoryAndEvents())
		{
			CCP_LOGERR("There is no shared memory to read from");
		}		
	}

	return RS_OK;
}

#endif
