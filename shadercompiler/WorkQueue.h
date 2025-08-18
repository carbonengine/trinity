#pragma once

#if( TRINITY_PLATFORM == TRINITY_METAL )
#include <mach/semaphore.h>
#include <mach/mach.h>
#endif

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
	virtual ~IWorkQueue() = default;
	virtual void OnBlocked() = 0;
	virtual void OnUnblocked() = 0;
};

template <typename T, typename Processor>
class WorkQueue2 : IWorkQueue
{
public:
	WorkQueue2( size_t totalWorkerCount, size_t activeWorkersCount, Processor processor ) :
		m_processor( processor )
	{
		m_totalWorkerCount = totalWorkerCount;
		m_activeWorkersCount = activeWorkersCount;

#if( TRINITY_PLATFORM == TRINITY_METAL )
		semaphore_create( current_task(), &m_activeWorkersSemaphore, SYNC_POLICY_FIFO, (int32_t)activeWorkersCount );
#else
		m_activeWorkersSemaphore = CreateSemaphore( NULL, (long)activeWorkersCount, (long)activeWorkersCount, NULL );
		if( m_activeWorkersSemaphore == NULL )
		{
			g_messages.AddMessage( "WorkQueue2: Creating m_activeWorkersSemaphore failed! Error: %d", GetLastError() );
		}
#endif
		started = false;
		
		for( size_t i = 0; i < totalWorkerCount; ++i )
		{
			m_workerThreads.emplace_back( std::thread( [this] { WorkerThread(); } ) );
		}
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
		Join();
#if( TRINITY_PLATFORM == TRINITY_METAL )
		semaphore_destroy( current_task(), m_activeWorkersSemaphore );
#else
		CloseHandle( m_activeWorkersSemaphore );
#endif
	}

	void Put( const T& item )
	{
		assert( ( "Don't call WorkQueue2::Put after WorkQueue2::Join! ", !started ) );
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

	virtual void OnBlocked() override
	{
		FreeSemaphore();
	}

	virtual void OnUnblocked() override
	{
		AquireSemaphore();
	}

private:
	void AquireSemaphore()
	{
#if( TRINITY_PLATFORM == TRINITY_METAL )
		auto waitResult = semaphore_wait( m_activeWorkersSemaphore );
		if ( waitResult != KERN_SUCCESS )
		{
			g_messages.AddMessage( "WorkQueue2: Waiting on m_activeWorkersSemaphore failed! Error: %d", waitResult );
		}
#else
		DWORD waitResult = WaitForSingleObject( m_activeWorkersSemaphore, INFINITE );
		if( waitResult != WAIT_OBJECT_0 )
		{
			if( waitResult == WAIT_FAILED )
			{
				g_messages.AddMessage( "WorkQueue2: Waiting on m_activeWorkersSemaphore failed! Error: %d %d", waitResult, GetLastError() );
			}
			else
			{
				g_messages.AddMessage( "WorkQueue2: Waiting on m_activeWorkersSemaphore failed! Error: %d", waitResult );
			}
		}
#endif
	}

	void FreeSemaphore()
	{
#if( TRINITY_PLATFORM == TRINITY_METAL )
		semaphore_signal( m_activeWorkersSemaphore );
#else
		if( !ReleaseSemaphore( m_activeWorkersSemaphore, 1, NULL ) )
		{
			g_messages.AddMessage( "WorkQueue2::OnBlocked: ReleaseSemaphore m_activeWorkersSemaphore error: %d", GetLastError() );
		}
#endif
	}

	void PutPtr( T* item )
	{
		// mutex guard shouldn't be required. nothing should added to the queue after WorkQueue2::Join
		m_queue.push( item );
	}

	void WorkerThread()
	{
#if CCP_TELEMETRY_ENABLED
		tracy::SetThreadName( "Compile Worker Thread" );
#endif

		{
			std::unique_lock scope( m_startMutex );
			m_waitToStart.wait( scope, [this] { return started; } );
		}

		AquireSemaphore();

		while( true )
		{
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

			if( !m_processor( *item, this ) )
			{
				// some terrible failure?
				delete item;
				break;
			}
			delete item;
		}

		FreeSemaphore();
	}

	Processor m_processor;
	std::queue<T*> m_queue;
	std::mutex m_queueMutex;
	std::vector<std::thread> m_workerThreads;
	size_t m_totalWorkerCount;
	size_t m_activeWorkersCount;
#if( TRINITY_PLATFORM == TRINITY_METAL )
	semaphore_t m_activeWorkersSemaphore;
#else
	HANDLE m_activeWorkersSemaphore;
#endif
	bool started = false;
	std::mutex m_startMutex;
	std::condition_variable m_waitToStart;
};
