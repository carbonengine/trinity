////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#include "stdafx.h"
#include "CompileMessageQueue.h"

#if !_WIN32
#include <pthread.h>
#include <unistd.h>
#endif

// --------------------------------------------------------------------------------------
// Description:
//   CompileMessageQueue default constructor
// --------------------------------------------------------------------------------------
CompileMessageQueue::CompileMessageQueue()
	:m_messagesMutex( 300 )
	,m_stop( false )
{
#if _WIN32
	m_queueEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_thread = CreateThread( NULL, 0, &ThreadRoutine, this, 0, NULL );
#elif __APPLE__
	pthread_cond_init( &m_queueEvent, nullptr );
	pthread_create( &m_thread, nullptr, ThreadRoutine, this );
#else
	#error "Unsupported platform."
#endif
}

// --------------------------------------------------------------------------------------
// Description:
//   CompileMessageQueue destructor
// --------------------------------------------------------------------------------------
CompileMessageQueue::~CompileMessageQueue()
{
	m_stop = true;

#if _WIN32
	SetEvent( m_queueEvent );
	WaitForSingleObject( m_thread, INFINITE );
	CloseHandle( m_queueEvent );
	CloseHandle( m_thread );
#else
	m_messagesMutex.Lock();
	pthread_cond_signal( &m_queueEvent );
	m_messagesMutex.Unlock();

	pthread_join( m_thread, nullptr );
	pthread_cond_destroy( &m_queueEvent );
#endif
}

// TODO MACOS
#if _WIN32
// --------------------------------------------------------------------------------------
// Description:
//   Adds new messages to the output queue.
// Arguments:
//   buffer - Buffer with compiler messages
// --------------------------------------------------------------------------------------
void CompileMessageQueue::AddMessages( ID3DXBuffer* buffer )
{
	m_messagesMutex.Lock();
	m_messages.push( reinterpret_cast<char*>( buffer->GetBufferPointer() ) );
	m_messagesMutex.Unlock();
	SetEvent( m_queueEvent );
}

void CompileMessageQueue::AddMessages( ID3D10Blob* buffer )
{
	// This is hacky, but it works as interfaces are identical
	AddMessages( (ID3DXBuffer*)buffer );
}
#endif

void CompileMessageQueue::AddMessage( const char* format, ... )
{
	va_list args;

	va_start( args, format );
	int count = vsnprintf( nullptr, 0, format, args ) + 1;
	va_end( args );

	std::string message;
	message.resize(count);

	va_start( args, format );
	count = vsnprintf( &message[0], message.size(), format, args );
	va_end( args );

#if _WIN32
	m_messagesMutex.Lock();
	m_messages.push( message );
	m_messagesMutex.Unlock();
	SetEvent( m_queueEvent );
#else
	m_messagesMutex.Lock();
	m_messages.push( message );
	pthread_cond_signal( &m_queueEvent );
	m_messagesMutex.Unlock();
#endif
}

// --------------------------------------------------------------------------------------
// Description:
//   Waits until all messages are printed to stdout.
// --------------------------------------------------------------------------------------
void CompileMessageQueue::Flush()
{
	while( true )
	{
#if _WIN32
		SetEvent( m_queueEvent );
		m_messagesMutex.Lock();
		bool empty = m_messages.empty();
		m_messagesMutex.Unlock();
#else
		m_messagesMutex.Lock();
		bool empty = m_messages.empty();
		pthread_cond_signal( &m_queueEvent );
		m_messagesMutex.Unlock();
#endif
		if( empty )
		{
			break;
		}
#if _WIN32
		Sleep( 500 );
#else
		sleep( 0.5 );
#endif
	}
}

void CompileMessageQueue::SetEntryFileName( const char* fileName )
{
	m_entryFileName = fileName;
}

const char* CompileMessageQueue::GetEntryFileName() const
{
	return m_entryFileName.c_str();
}

// --------------------------------------------------------------------------------------
// Description:
//   Output thread routine: prints unique messages from the queue.
// --------------------------------------------------------------------------------------
void CompileMessageQueue::Run()
{
	while( true )
	{
#if _WIN32
		WaitForSingleObject( m_queueEvent, INFINITE );
#else
		m_messagesMutex.Lock();
		while ( m_messages.empty() && !m_stop )
			pthread_cond_wait( &m_queueEvent, m_messagesMutex.GetRaw() );
		m_messagesMutex.Unlock();
#endif
		while( true )
		{
			m_messagesMutex.Lock();
			if( !m_messages.empty() )
			{
				std::string buffer = m_messages.front();
				m_messages.pop();
				m_messagesMutex.Unlock();

				OutputMessages( buffer.c_str() );
			}
			else
			{
				m_messagesMutex.Unlock();
				break;
			}
		}
		if( m_stop )
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
		if( *end == 0 || ( end[0] == '\n' && !isspace(end[1]) ) )
		{
			std::string message( start, end );
			if( m_printedMessages.insert( message ).second )
			{
				const char* memoryRefs[] = { "\\memory(", "/memory(", "/memory:" };
				for( auto memoryRef : memoryRefs )
				{
					auto pos = message.find( memoryRef );
					if( pos != std::string::npos )
					{
						message.replace( 0, pos + strlen( memoryRef ) - 1, m_entryFileName );
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
#if _WIN32
DWORD WINAPI CompileMessageQueue::ThreadRoutine( LPVOID param )
{
	reinterpret_cast<CompileMessageQueue*>( param )->Run(); 
	return 0;
} 
#else
void* CompileMessageQueue::ThreadRoutine( void* param )
{
	reinterpret_cast<CompileMessageQueue*>( param )->Run(); 
	return nullptr;
} 
#endif
