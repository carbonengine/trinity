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

#if _WIN32
	void AddMessages( ID3DBlob* buffer );
#endif
	void AddMessage( const char* format, ... );
	void Flush();
	void SetEntryFileName( const char* fileName );

	const char* GetEntryFileName() const;

private:
	void Run();
	void OutputMessages( const char* messages );

	// Message queue
	std::queue<std::string> m_messages;
	std::mutex m_messagesMutex;
	std::condition_variable m_queueEvent;
	std::thread m_thread;

	// A set of already printed messages
	std::set<std::string> m_printedMessages;
	// Entry point file name (for fixing compiler messages)
	std::string m_entryFileName;
	// Flag to stop the message processing thread
	std::atomic<bool> m_stop;
};

#endif // CompileMessageQueue_H
