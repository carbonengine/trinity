#pragma once

#include "Mutex.h"

#if !_WIN32
#include <dispatch/dispatch.h>
#endif

template <typename T, typename Processor>
class WorkQueue
{
public:
	WorkQueue( size_t workerCount, Processor processor )
		:m_processor( processor ),
		m_mutex( 1000 ),
		m_workerCount( workerCount )
	{
#if _WIN32
		m_added = CreateSemaphore( NULL, 0, LONG_MAX, NULL );

		m_workerThreads = new HANDLE[workerCount];
		for( unsigned i = 0; i < workerCount; ++i )
		{
			m_workerThreads[i] = CreateThread( NULL, 0, &WorkerThread, this, 0, NULL );
		}
#else
		m_added = dispatch_semaphore_create( 0 );

		m_workerThreads = new pthread_t[workerCount];
		for( unsigned i = 0; i < workerCount; ++i )
		{
			pthread_create( &m_workerThreads[i], nullptr, WorkerThread, this );
		}
#endif
	}

	~WorkQueue()
	{
		while( !m_queue.empty() )
		{
			delete m_queue.front();
			m_queue.pop();
		}
#if _WIN32
		for( unsigned i = 0; i < m_workerCount; ++i )
		{
			CloseHandle( m_workerThreads[i] );
		}
		CloseHandle( m_added );
#else
		for( unsigned i = 0; i < m_workerCount; ++i )
		{
			pthread_join( m_workerThreads[i], nullptr );
		}
#endif
		delete [] m_workerThreads;
	}

	void Put( const T& item )
	{
		PutPtr( new T( item ) );
	}

	void Join()
	{
		for( unsigned i = 0; i < m_workerCount; ++i )
		{
			PutPtr( nullptr );
		}

#if _WIN32
		WaitForMultipleObjects( DWORD( m_workerCount ), m_workerThreads, TRUE, INFINITE );
#else
		for( unsigned i = 0; i < m_workerCount; ++i )
		{
			pthread_join( m_workerThreads[i], nullptr );
		}
#endif
	}
private:

	void PutPtr( T* item )
	{
		MutexScope scope( m_mutex );
		m_queue.push( item );
#if _WIN32
		ReleaseSemaphore( m_added, 1, NULL );
#else
		dispatch_semaphore_signal( m_added );
#endif
	}

	T* Get()
	{
		for( ;; )
		{
			{
				MutexScope scope( m_mutex );
				if( !m_queue.empty() )
				{
					auto item = m_queue.front();
					m_queue.pop();
					return item;
				}
			}

#if _WIN32
			WaitForSingleObject( m_added, INFINITE );
#else
			dispatch_semaphore_wait( m_added, DISPATCH_TIME_FOREVER );
#endif
		}
	}

#if _WIN32
	static DWORD WINAPI WorkerThread( LPVOID param )
#else
	static void* WorkerThread( void* param )
#endif
	{
		tmThreadName( 0, 0, "Compile Worker Thread" );

		WorkQueue* self = static_cast<WorkQueue*>( param );
		while( true )
		{
			auto item = self->Get();
			if( !item )
			{
				return 0;
			}
			if( !self->m_processor( *item ) )
			{
				return 0;
			}
			delete item;
		}
		return 0;
	}

	Processor m_processor;
	std::queue<T*> m_queue;
	Mutex m_mutex;
	// Semaphore for blocking Get
#if _WIN32
	HANDLE m_added;
	HANDLE *m_workerThreads;
#else
	dispatch_semaphore_t m_added;
	pthread_t* m_workerThreads;
#endif
	size_t m_workerCount;
};
