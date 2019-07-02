#pragma once

#include "Mutex.h"

template <typename T, typename Processor>
class WorkQueue
{
public:
	WorkQueue( size_t workerCount, Processor processor )
		:m_workerCount( workerCount ),
		m_processor( processor ),
		m_cs( 1000 )
	{
		m_added = CreateSemaphore( NULL, 0, LONG_MAX, NULL );

		m_workerThreads = new HANDLE[workerCount];
		for( unsigned i = 0; i < workerCount; ++i )
		{
			m_workerThreads[i] = CreateThread( NULL, 0, &WorkerThread, this, 0, NULL );
		}
	}

	~WorkQueue()
	{
		while( !m_queue.empty() )
		{
			delete m_queue.front();
			m_queue.pop();
		}
		for( unsigned i = 0; i < m_workerCount; ++i )
		{
			CloseHandle( m_workerThreads[i] );
		}
		CloseHandle( m_added );
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

		WaitForMultipleObjects( DWORD( m_workerCount ), m_workerThreads, TRUE, INFINITE );
	}
private:

	void PutPtr( T* item )
	{
		MutexScope scope( m_cs );
		m_queue.push( item );
		ReleaseSemaphore( m_added, 1, NULL );
	}

	T* Get()
	{
		for( ;; )
		{
			{
				MutexScope scope( m_cs );
				if( !m_queue.empty() )
				{
					auto item = m_queue.front();
					m_queue.pop();
					return item;
				}
			}

			WaitForSingleObject( m_added, INFINITE );
		}
	}

	static DWORD WINAPI WorkerThread( LPVOID param )
	{
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
	Mutex m_cs;
	// Semaphore for blocking Get
	HANDLE m_added;
	HANDLE *m_workerThreads;
	size_t m_workerCount;
};