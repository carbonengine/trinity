#pragma once


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

	void Enter()
	{
		EnterCriticalSection( &m_cs );
	}

	void Leave()
	{
		LeaveCriticalSection( &m_cs );
	}
private:
	CRITICAL_SECTION m_cs;
};


class MutexScope
{
public:
	MutexScope( Mutex& mutex )
		:m_mutex( mutex )
	{
		mutex.Enter();
	}

	~MutexScope()
	{
		m_mutex.Leave();
	}
private:
	Mutex& m_mutex;
};