// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2AsyncCreationAL.h"


namespace TrinityALImpl
{

Tr2AsyncDispatcherAL::Tr2AsyncDispatcherAL() :
	m_mainThread( std::this_thread::get_id() ),
	m_generation( 0 )
{
}

void Tr2AsyncDispatcherAL::SetMainThread()
{
	std::lock_guard<std::mutex> lock( m_mutex );
	m_mainThread = std::this_thread::get_id();
}

bool Tr2AsyncDispatcherAL::IsMainThread() const
{
	std::lock_guard<std::mutex> lock( m_mutex );
	return m_mainThread == std::this_thread::get_id();
}

void Tr2AsyncDispatcherAL::SetPump( std::function<void()> pump )
{
	std::lock_guard<std::mutex> lock( m_mutex );
	m_pump = std::move( pump );
}

void Tr2AsyncDispatcherAL::Post( std::function<void()> callback )
{
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		m_callbacks.push_back( std::move( callback ) );
		++m_generation;
	}
	m_condition.notify_all();
}

void Tr2AsyncDispatcherAL::Notify()
{
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		++m_generation;
	}
	m_condition.notify_all();
}

void Tr2AsyncDispatcherAL::Dispatch()
{
	CCP_ASSERT( IsMainThread() );

	std::function<void()> pump;
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		pump = m_pump;
	}
	if( pump )
	{
		pump();
	}

	// Callbacks may post new callbacks or dispatch recursively (by waiting on a future), so
	// run only what has been queued so far and don't hold the lock while doing it.
	std::vector<std::function<void()>> callbacks;
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		callbacks.swap( m_callbacks );
	}
	for( auto& callback : callbacks )
	{
		callback();
	}
}

uint64_t Tr2AsyncDispatcherAL::GetGeneration() const
{
	std::lock_guard<std::mutex> lock( m_mutex );
	return m_generation;
}

void Tr2AsyncDispatcherAL::WaitForWork( uint64_t generation )
{
	std::unique_lock<std::mutex> lock( m_mutex );
	m_condition.wait( lock, [this, generation] { return m_generation != generation; } );
}


Tr2AsyncWorkerAL::Tr2AsyncWorkerAL( std::shared_ptr<Tr2AsyncDispatcherAL> dispatcher ) :
	m_dispatcher( std::move( dispatcher ) ),
	m_running( false ),
	m_stopRequested( false )
{
}

Tr2AsyncWorkerAL::~Tr2AsyncWorkerAL()
{
	// Derived classes must stop the worker while their ExecuteBatch is still callable
	CCP_ASSERT( !m_thread.joinable() );
}

void Tr2AsyncWorkerAL::Start()
{
	std::lock_guard<std::mutex> lock( m_mutex );
	CCP_ASSERT( !m_running );
	m_running = true;
	m_stopRequested = false;
	m_thread = std::thread( [this] { Run(); } );
}

void Tr2AsyncWorkerAL::Stop()
{
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		m_stopRequested = true;
	}
	m_condition.notify_all();
	if( m_thread.joinable() )
	{
		m_thread.join();
	}

	std::vector<std::shared_ptr<Tr2AsyncJobAL>> pending;
	std::vector<std::shared_ptr<Tr2AsyncJobAL>> completed;
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		m_running = false;
		pending.swap( m_pending );
		completed.swap( m_completed );
	}
	for( auto& job : completed )
	{
		job->Cancel( E_ABORT );
	}
	for( auto& job : pending )
	{
		job->Cancel( E_ABORT );
	}
}

void Tr2AsyncWorkerAL::Submit( std::shared_ptr<Tr2AsyncJobAL> job )
{
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		if( m_running && !m_stopRequested )
		{
			m_pending.push_back( std::move( job ) );
			job = nullptr;
		}
	}
	if( job )
	{
		job->Cancel( E_ABORT );
	}
	else
	{
		m_condition.notify_all();
	}
}

void Tr2AsyncWorkerAL::FinishCompleted()
{
	std::vector<std::shared_ptr<Tr2AsyncJobAL>> completed;
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		completed.swap( m_completed );
	}
	for( auto& job : completed )
	{
		job->Finish();
	}
}

void Tr2AsyncWorkerAL::Run()
{
	for( ;; )
	{
		std::vector<std::shared_ptr<Tr2AsyncJobAL>> batch;
		{
			std::unique_lock<std::mutex> lock( m_mutex );
			m_condition.wait( lock, [this] { return m_stopRequested || !m_pending.empty(); } );
			if( m_stopRequested )
			{
				return;
			}
			if( m_pending.size() <= MAX_BATCH_SIZE )
			{
				batch.swap( m_pending );
			}
			else
			{
				batch.assign( m_pending.begin(), m_pending.begin() + MAX_BATCH_SIZE );
				m_pending.erase( m_pending.begin(), m_pending.begin() + MAX_BATCH_SIZE );
			}
		}

		ExecuteBatch( batch );

		{
			std::lock_guard<std::mutex> lock( m_mutex );
			m_completed.insert( m_completed.end(), batch.begin(), batch.end() );
		}
		m_dispatcher->Notify();
	}
}

}
