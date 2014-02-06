////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#include "stdafx.h"
#include "CompileMessageQueue.h"

// --------------------------------------------------------------------------------------
// Description:
//   CompileMessageQueue default constructor
// --------------------------------------------------------------------------------------
CompileMessageQueue::CompileMessageQueue()
{
	InitializeCriticalSectionAndSpinCount( &m_messagesCS, 300 );
	m_queueEvents[0] = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_queueEvents[1] = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_thread = CreateThread( NULL, 0, &ThreadRoutine, this, 0, NULL );
}

// --------------------------------------------------------------------------------------
// Description:
//   CompileMessageQueue destructor
// --------------------------------------------------------------------------------------
CompileMessageQueue::~CompileMessageQueue()
{
	SetEvent( m_queueEvents[1] );
	WaitForSingleObject( m_thread, INFINITE );
	DeleteCriticalSection( &m_messagesCS );
	CloseHandle( m_queueEvents[0] );
	CloseHandle( m_queueEvents[1] );
	CloseHandle( m_thread );
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds new messages to the output queue.
// Arguments:
//   buffer - Buffer with compiler messages
// --------------------------------------------------------------------------------------
void CompileMessageQueue::AddMessages( ID3DXBuffer* buffer )
{
	EnterCriticalSection( &m_messagesCS );
	m_messages.push( buffer );
	LeaveCriticalSection( &m_messagesCS );
	SetEvent( m_queueEvents[0] );
}

void CompileMessageQueue::AddMessages( ID3D10Blob* buffer )
{
	// This is hacky, but it works as interfaces are identical
	AddMessages( (ID3DXBuffer*)buffer );
}

void CompileMessageQueue::AddMessage( const char* format, ... )
{
	va_list args;
	va_start( args, format );
	int count = _vscprintf( format, args ) + 1;
	CComPtr<ID3DXBuffer> buffer;
	D3DXCreateBuffer( count, &buffer );
	vsprintf_s( ( (char*)buffer->GetBufferPointer() ), count, format, args );
	AddMessages( buffer );
}

// --------------------------------------------------------------------------------------
// Description:
//   Waits until all messages are printed to stdout.
// --------------------------------------------------------------------------------------
void CompileMessageQueue::Flush()
{
	while( true )
	{
		SetEvent( m_queueEvents[0] );
		EnterCriticalSection( &m_messagesCS );
		bool empty = m_messages.empty();
		LeaveCriticalSection( &m_messagesCS );
		if( empty )
		{
			break;
		}
		Sleep( 500 );
	}
}

void CompileMessageQueue::SetEntryFileName( const char* fileName )
{
	m_entryFileName = fileName;
}

// --------------------------------------------------------------------------------------
// Description:
//   Output thread routine: prints unique messages from the queue.
// --------------------------------------------------------------------------------------
void CompileMessageQueue::Run()
{
	while( true )
	{
		DWORD result = WaitForMultipleObjects( 2, m_queueEvents, FALSE, INFINITE );
		while( true )
		{
			EnterCriticalSection( &m_messagesCS );
			if( !m_messages.empty() )
			{
				CComPtr<ID3DXBuffer> buffer = m_messages.front();
				m_messages.pop();
				LeaveCriticalSection( &m_messagesCS );

				char* messages = reinterpret_cast<char*>( buffer->GetBufferPointer() );
				OutputMessages( messages );
			}
			else
			{
				LeaveCriticalSection( &m_messagesCS );
				break;
			}
		}
		if( result == WAIT_OBJECT_0 + 1 )
		{
			break;
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Prints unique messages from the input string of several messages.
// Arguments:
//   messages - A string containing several messages. A message is delimed by new line
//              followed by non-space character
// --------------------------------------------------------------------------------------
void CompileMessageQueue::OutputMessages( const char* messages )
{
	const char* start = messages;
	const char* end = messages + 1;
	while( *start )
	{
		if( *end == 0 || end[0] == '\n' && !isspace(end[1]) )
		{
			std::string message( start, end );
			if( m_printedMessages.insert( message ).second )
			{
				const char* memoryRef = "\\memory(";
				auto pos = message.find(memoryRef);
				if( pos != std::string::npos )
				{
					message.replace( 0, pos + 7, m_entryFileName );
				}
				else
				{
					const char* memoryRef2 = "/memory(";
					auto pos = message.find(memoryRef2);
					if( pos != std::string::npos )
					{
						message.replace( 0, pos + 7, m_entryFileName );
					}
				}
				printf( "%s\n", message.c_str() );
				fflush( stdout );
			}
			if( *end == 0 )
			{
				return;
			}
			start = end + 1;
		}
		++end;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Output thread routine. Calls CompileMessageQueue::Run
// --------------------------------------------------------------------------------------
DWORD WINAPI CompileMessageQueue::ThreadRoutine( LPVOID param )
{
	reinterpret_cast<CompileMessageQueue*>( param )->Run(); 
	return 0;
} 
