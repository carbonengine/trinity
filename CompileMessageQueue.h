////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef CompileMessageQueue_H
#define CompileMessageQueue_H

#include "Mutex.h"

// --------------------------------------------------------------------------------------
// Description:
//   Maintains a queue of output compile error/warning messages and outputs unique messages
//   to stdout. Uses a separate thread for outputting messages.
// --------------------------------------------------------------------------------------
class CompileMessageQueue
{
public:
	CompileMessageQueue();
	~CompileMessageQueue();

#if _WIN32
	void AddMessages( ID3DXBuffer* buffer );
	void AddMessages( ID3D10Blob* buffer );
#endif
	void AddMessage( const char* format, ... );
	void Flush();
	void SetEntryFileName( const char* fileName );

	const char* GetEntryFileName() const;

private:
	void Run();
	void OutputMessages( const char* messages );
#if _WIN32
	static DWORD WINAPI ThreadRoutine( LPVOID param );
#else
	static void* ThreadRoutine( void* param );
#endif

	// Message queue
	std::queue<std::string> m_messages;
	// Mutex for message queue
	Mutex m_messagesMutex;
#if _WIN32
	// Event for new messages in a queue and for exiting
	HANDLE m_queueEvent;
	// Handle to the output thread
	HANDLE m_thread;
#else
	// Event for new messages in a queue and for exiting
	pthread_cond_t m_queueEvent;
	// Handle to the output thread
	pthread_t m_thread;
#endif
	// A set of already printed messages
	std::set<std::string> m_printedMessages;
	// Entry point file name (for fixing compiler messages)
	std::string m_entryFileName;
	// Flag to stop the message processing thread
	volatile bool m_stop;
};

#endif // CompileMessageQueue_H
