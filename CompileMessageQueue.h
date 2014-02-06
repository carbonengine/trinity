////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef CompileMessageQueue_H
#define CompileMessageQueue_H

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

	void AddMessages( ID3DXBuffer* buffer );
	void AddMessages( ID3D10Blob* buffer );
	void AddMessage( const char* format, ... );
	void Flush();
	void SetEntryFileName( const char* fileName );
private:
	void Run();
	void OutputMessages( const char* messages );
	static DWORD WINAPI ThreadRoutine( LPVOID param );

	// Message queue
	std::queue<CComPtr<ID3DXBuffer>> m_messages;
	// Critical section for message queue
	CRITICAL_SECTION m_messagesCS;
	// Events for new messages in a queue and for exiting
	HANDLE m_queueEvents[2];
	// Handle to the output thread
	HANDLE m_thread;
	// A set of already printed messages
	std::set<std::string> m_printedMessages;
	// Entry point file name (for fixing compiler messages)
	std::string m_entryFileName;
};

#endif // CompileMessageQueue_H