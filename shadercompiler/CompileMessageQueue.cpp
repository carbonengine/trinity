////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#include "stdafx.h"
#include "CompileMessageQueue.h"


CompileMessageQueue::CompileMessageQueue()
	:m_stop( false )
{
	m_thread = std::thread( [this] { this->Run(); } );
}

CompileMessageQueue::~CompileMessageQueue()
{
	m_stop = true;

	m_queueEvent.notify_one();
	m_thread.join();
}

#if _WIN32
void CompileMessageQueue::AddMessages( ID3DBlob* buffer )
{
	m_messagesMutex.lock();
	m_messages.push( reinterpret_cast<char*>( buffer->GetBufferPointer() ) );
	m_messagesMutex.unlock();
	m_queueEvent.notify_one();
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

	m_messagesMutex.lock();
	m_messages.push( message );
	m_messagesMutex.unlock();
	m_queueEvent.notify_one();
}

// --------------------------------------------------------------------------------------
// Description:
//   Waits until all messages are printed to stdout.
// --------------------------------------------------------------------------------------
void CompileMessageQueue::Flush()
{
	while( true )
	{
		m_queueEvent.notify_one();
		m_messagesMutex.lock();
		bool empty = m_messages.empty();
		m_messagesMutex.unlock();
		if( empty )
		{
			break;
		}
		std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
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
		{
			std::unique_lock lock( m_messagesMutex );
			m_queueEvent.wait( lock, [this] { return !m_messages.empty() || m_stop; } );
		}
		while( true )
		{
			m_messagesMutex.lock();
			if( !m_messages.empty() )
			{
				std::string buffer = m_messages.front();
				m_messages.pop();
				m_messagesMutex.unlock();

				OutputMessages( buffer.c_str() );
			}
			else
			{
				m_messagesMutex.unlock();
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
