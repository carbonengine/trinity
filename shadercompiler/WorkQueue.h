#pragma once


#include "CompileMessageQueue.h"
extern CompileMessageQueue g_messages;

template <typename T, typename Processor>
class WorkQueue
{
public:
	WorkQueue( size_t workerCount, Processor processor )
		:m_processor( processor )
	{
		for( size_t i = 0; i < workerCount; ++i )
		{
			m_workerThreads.emplace_back( std::thread( [this] { WorkerThread(); } ) );
		}
	}

	~WorkQueue()
	{
		{
			std::lock_guard lock( m_mutex );
			while( !m_queue.empty() )
			{
				delete m_queue.front();
				m_queue.pop();
			}
		}
		m_added.notify_all();
		for( auto& thread : m_workerThreads )
		{
			thread.join();
		}
	}

	void Put( const T& item )
	{
		PutPtr( new T( item ) );
	}

	void Join()
	{
		for( unsigned i = 0; i < m_workerThreads.size(); ++i )
		{
			PutPtr( nullptr );
		}
		for( auto& thread : m_workerThreads )
		{
			thread.join();
		}
		m_workerThreads.clear();
	}
private:

	void PutPtr( T* item )
	{
		std::lock_guard scope( m_mutex );
		m_queue.push( item );
		m_added.notify_all();
	}

	T* Get()
	{
		for( ;; )
		{
			std::unique_lock scope( m_mutex );
			if( !m_queue.empty() )
			{
				auto item = m_queue.front();
				m_queue.pop();
				return item;
			}
			m_added.wait( scope, [this] { return !m_queue.empty(); } );
		}
	}

	void WorkerThread()
	{
#if CCP_TELEMETRY_ENABLED
		tracy::SetThreadName( "Compile Worker Thread" );
#endif

		while( true )
		{
			auto item = Get();
			if( !item )
			{
				return;
			}
			if( !m_processor( *item ) )
			{
				return;
			}
			delete item;
		}
	}

	Processor m_processor;
	std::queue<T*> m_queue;
	std::mutex m_mutex;
	std::condition_variable m_added;
	std::vector<std::thread> m_workerThreads;
};


class IWorkQueue
{
public:
	virtual void OnBlocked( size_t i ) = 0;
	virtual void OnUnblocked( size_t i ) = 0;
};



template <typename T, typename Processor>
class WorkQueue2 : IWorkQueue
{
public:
	WorkQueue2( size_t totalWorkerCount, size_t activeWorkersCount, Processor processor ) :
		m_processor( processor )
	{
		//g_messages.AddMessage( "%s", "Testestest" );

		m_totalWorkerCount = totalWorkerCount;
		m_activeWorkersCount = activeWorkersCount;

		m_activeWorkersSemaphore = CreateSemaphore(
			NULL, // default security attributes
			(long)activeWorkersCount, // initial count
			(long)activeWorkersCount, // maximum count
			NULL ); // unnamed semaphore
		if( m_activeWorkersSemaphore == NULL )
		{
			g_messages.AddMessage( "%s %d", "WorkQueue2: Creating m_activeWorkersSemaphore failed!", GetLastError() );
		}

		started = false;
		
		for( size_t i = 0; i < totalWorkerCount; ++i )
		{
			m_workerThreads.emplace_back( std::thread( [this, i] { WorkerThread( i ); } ) );
			//m_workerThreadStates.push_back( ThreadState::Idle );
		}



		//for( size_t i = 0; i < activeWorkersCount; ++i )
		//{
		//	activeWorkersSemaphore.release();
		//}
		//
		//// TODO: intern, is this necessary?
		//for( size_t i = 0; i < workerCount; ++i )
		//{
		//	totalWorkersSemaphore.release();
		//}

		
	}

	virtual ~WorkQueue2()
	{
		{
			std::lock_guard lock( m_queueMutex );
			while( !m_queue.empty() )
			{
				delete m_queue.front();
				m_queue.pop();
			}
		}

		CloseHandle( m_activeWorkersSemaphore );

		//g_messages.AddMessage( "%s", "Destructor Testestest" );

		Join();
	}

	void Put( const T& item )
	{
		// Don't call this after the queue already started working! 
		// TODO: intern, maybe check some flag here?
		PutPtr( new T( item ) );
	}

	void Join()
	{
		{
			std::lock_guard scope( m_startMutex );
			started = true;
			m_waitToStart.notify_all();
		}

		for( auto& thread : m_workerThreads )
		{
			thread.join();
		}
		m_workerThreads.clear();
	}

	virtual void OnBlocked( size_t ) override
	{
		//m_workerThreadStates[i] = ThreadState::Blocked;
		//activeWorkersSemaphore.release();
		if( !ReleaseSemaphore(
				m_activeWorkersSemaphore, // handle to semaphore
				1, // increase count by one
				NULL ) ) // not interested in previous count
		{
			g_messages.AddMessage( "OnBlocked: ReleaseSemaphore m_activeWorkersSemaphore error: %d", GetLastError() );
		}
	}

	virtual void OnUnblocked( size_t ) override
	{
		//m_workerThreadStates[i] = ThreadState::Ready;
		//activeWorkersSemaphore.acquire();
		DWORD waitResult = WaitForSingleObject(
			m_activeWorkersSemaphore, // handle to semaphore
			INFINITE ); // TODO: intern, changed this from zero to infinite, as documentation is conflicting. test if this works...
		if( waitResult != WAIT_OBJECT_0 )
		{
			if( waitResult == WAIT_FAILED )
			{
				g_messages.AddMessage( "%s %d %d", "OnUnblocked: Waiting on m_activeWorkersSemaphore failed!", waitResult, GetLastError() );
			}
			else
			{
				g_messages.AddMessage( "%s %d", "OnUnblocked: Waiting on m_activeWorkersSemaphore failed!", waitResult );
			}
		}
		//m_workerThreadStates[i] = ThreadState::Active;
	}

private:
	void PutPtr( T* item )
	{
		// TODO: intern, this shouldn't be required if nothing is added to the queue after starting, assuming the caller is not multithreaded
		//std::lock_guard scope( m_queueMutex );
		m_queue.push( item );
	}

	void WorkerThread( size_t i )
	{
#if CCP_TELEMETRY_ENABLED
		tracy::SetThreadName( "Compile Worker Thread" );
#endif

		{
			std::unique_lock scope( m_startMutex );
			m_waitToStart.wait( scope, [this] { return started; } );
		}

		DWORD waitResult = WaitForSingleObject(
			m_activeWorkersSemaphore, // handle to semaphore
			INFINITE ); // TODO: intern, changed this from zero to infinite, as documentation is conflicting. test if this works...
		if( waitResult != WAIT_OBJECT_0 )
		{
			if( waitResult == WAIT_FAILED )
			{
				g_messages.AddMessage( "%s %d %d", "WorkerThread: Waiting on m_activeWorkersSemaphore failed!", waitResult, GetLastError() );
			}
			else
			{
				g_messages.AddMessage( "%s %d", "WorkerThread: Waiting on m_activeWorkersSemaphore failed!", waitResult );
			}
		}

		while( true )
		{
			//m_workerThreadStates[i] = ThreadState::Idle;
			//m_activeWorkersSemaphore.acquire();


			//m_workerThreadStates[i] = ThreadState::Active;

			T* item = nullptr;
			{
				std::unique_lock scope( m_queueMutex );
				if ( m_queue.empty() )
				{
					break;
				}
				item = m_queue.front();
				m_queue.pop();
			}

			//auto blockedCallback = [this]() { OnBlocked( i ); };
			//auto unblockedCallback = [this]() { OnUnBlocked( i ); };
			if( !m_processor( *item, this, i ) ) //blockedCallback, unblockedCallback ) )
			{
				// TODO: intern, what m_processor's return value? some critical error that should kill thread?
				break;
			}
			delete item;

			// TODO:intern, I think addeding this was wrong... think more about it
			// 
			//if( !ReleaseSemaphore(
			//		m_activeWorkersSemaphore, // handle to semaphore
			//		1, // increase count by one
			//		NULL ) ) // not interested in previous count
			//{
			//	g_messages.AddMessage( "WorkerThread: ReleaseSemaphore m_activeWorkersSemaphore error: %d", GetLastError() );
			//}
		}

		if( !ReleaseSemaphore(
				m_activeWorkersSemaphore, // handle to semaphore
				1, // increase count by one
				NULL ) ) // not interested in previous count
		{
			g_messages.AddMessage( "WorkerThread: ReleaseSemaphore m_activeWorkersSemaphore error: %d", GetLastError() );
		}

		//m_workerThreadStates[i] = ThreadState::Finished;
	}

	enum ThreadState
	{
		Idle,
		Active,
		Blocked,
		Ready,
		Finished
	};

	Processor m_processor;
	std::queue<T*> m_queue;
	std::mutex m_queueMutex;
	std::vector<std::thread> m_workerThreads;
	//std::vector<ThreadState> m_workerThreadStates;
	size_t m_totalWorkerCount;
	size_t m_activeWorkersCount;
	HANDLE m_activeWorkersSemaphore;

	bool started = false;
	std::mutex m_startMutex;
	std::condition_variable m_waitToStart;
};
