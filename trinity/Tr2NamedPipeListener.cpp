#include "StdAfx.h"

#ifdef _WIN32

#include "Tr2NamedPipeListener.h"
#if BLUE_WITH_PYTHON
#include <BluePyCpp.h>
#endif
#define BUFSIZE 512

void ThreadFunction(void* plistener)
{
	Tr2NamedPipeListener* trilistener = static_cast<Tr2NamedPipeListener*>( plistener );

	BOOL  bResult = ConnectNamedPipe( trilistener->m_pipeHandle, 0 );
	DWORD dwError = GetLastError();
	
	if( bResult || dwError == ERROR_PIPE_CONNECTED )
	{
		BOOL	fSuccess = FALSE;
		TCHAR	chBuf[BUFSIZE]; 
		DWORD	read = 0;		
		while(1)
		{ 									
			fSuccess = ReadFile( trilistener->m_pipeHandle, &chBuf, BUFSIZE*sizeof(TCHAR), &read, 0 );				
			// Exit if an error other than ERROR_MORE_DATA occurs.
			if( !fSuccess && ( GetLastError() != ERROR_MORE_DATA ) ) 
				break;								
			trilistener->m_buffer = (BYTE*)realloc( trilistener->m_buffer, trilistener->m_size + read );
			if( trilistener->m_buffer )
			{
				memcpy_s( (BYTE*)trilistener->m_buffer+trilistener->m_size, read, (BYTE*)&chBuf, read );
				trilistener->m_size += read;
			}
		}	
		DisconnectNamedPipe( trilistener->m_pipeHandle );
		CloseHandle( trilistener->m_pipeHandle );
		InterlockedExchange(&trilistener->m_waiting, false);
		if( trilistener->m_callback )
		{
			//
#if BLUE_WITH_PYTHON
			Ccp::PyGilEnsure gilWrapper;
#endif
			trilistener->m_callback.CallVoid( trilistener );
		}
	}
}

Tr2NamedPipeListener::Tr2NamedPipeListener( IRoot* lockobj ):
	m_pipeHandle(NULL),
	m_buffer(NULL),
	m_size(0),
	m_waiting(0)
{
}

Tr2NamedPipeListener::~Tr2NamedPipeListener()
{
	Clear();
}

void Tr2NamedPipeListener::Clear( )
{
	// Free the memory if there is any
	free( m_buffer );
	m_buffer = NULL;
	m_size = 0;
}

bool Tr2NamedPipeListener::Listen( std::string pipeName )
{
	// Are we still busy waiting for a connection
	if( m_waiting )
	{
		
		return false;
	}

	// Free the memory if there was any before
	Clear();

	m_pipeHandle = ::CreateNamedPipe( (LPCSTR)pipeName.c_str(), 
		PIPE_ACCESS_INBOUND, 
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 
		1, 
		BUFSIZE*sizeof(TCHAR), 
		BUFSIZE*sizeof(TCHAR),
		NMPWAIT_USE_DEFAULT_WAIT, 
		NULL );

	if( m_pipeHandle == INVALID_HANDLE_VALUE )
	{
		return false;
	}
	// We will be busy now
	m_pipeName = pipeName;
	m_waiting = 1L;
	_beginthread(ThreadFunction, 0, (void*)this);
	return true;
}

void Tr2NamedPipeListener::SetCallback( const BlueScriptCallback& callback )
{
	m_callback = callback;
}

#endif
