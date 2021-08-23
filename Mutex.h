#pragma once


#if _WIN32

// WinAPI based mutex.
class Mutex
{
public:
	Mutex( uint32_t spinCount = 0 )
	{
		if( spinCount )
		{
			InitializeCriticalSectionAndSpinCount( &m_cs, spinCount );
		}
		else
		{
			InitializeCriticalSection( &m_cs );
		}
	}

	~Mutex()
	{
		DeleteCriticalSection( &m_cs );
	}

	void Lock()
	{
		EnterCriticalSection( &m_cs );
	}

	void Unlock()
	{
		LeaveCriticalSection( &m_cs );
	}

private:
	CRITICAL_SECTION m_cs;
};

#else

// POSIX lightweight mutex.
class Mutex
{
public:
	Mutex( uint32_t = 0 )
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init( &attr );
		pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );

		pthread_mutex_init( &m_mutex, &attr );

		pthread_mutexattr_destroy( &attr );
	}

	~Mutex()
	{
		pthread_mutex_destroy( &m_mutex );
	}

	void Lock()
	{
		pthread_mutex_lock( &m_mutex );
	}

	void Unlock()
	{
		pthread_mutex_unlock( &m_mutex );
	}

	pthread_mutex_t* GetRaw()
	{
		return &m_mutex;
	}

private:
	pthread_mutex_t m_mutex;
};

#endif


class MutexScope
{
public:
	MutexScope( Mutex& mutex )
		:m_mutex( mutex )
	{
		mutex.Lock();
	}

	~MutexScope()
	{
		m_mutex.Unlock();
	}
private:
	Mutex& m_mutex;
};
