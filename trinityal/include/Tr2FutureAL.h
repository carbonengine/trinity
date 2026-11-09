// Copyright © 2026 CCP ehf.

#pragma once

#include "../ALResult.h"

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winerror.h>
#endif

#ifndef E_ABORT
#define E_ABORT HRESULT( 0x80004004L )
#endif

#ifndef E_PENDING
#define E_PENDING HRESULT( 0x8000000AL )
#endif


namespace TrinityALImpl
{

// --------------------------------------------------------------------------------------
// Description:
//   Runs work that has to happen on the main thread: callbacks of Tr2FutureAL objects and
//   finishing of asynchronous operations by platform backends. The primary render context
//   owns the dispatcher and runs it from Present and from Tr2FutureAL::Wait calls made on
//   the main thread.
// --------------------------------------------------------------------------------------
class Tr2AsyncDispatcherAL
{
public:
	Tr2AsyncDispatcherAL();

	// Makes the calling thread the main thread
	void SetMainThread();
	bool IsMainThread() const;

	// Sets a function that Dispatch calls before running queued callbacks (platform backends
	// use it to finish asynchronous operations on the main thread)
	void SetPump( std::function<void()> pump );

	// Queues a callback to run on the main thread. Thread-safe.
	void Post( std::function<void()> callback );

	// Wakes up the main thread if it is blocked in WaitForWork. Thread-safe.
	void Notify();

	// Main thread only: runs the pump and then all queued callbacks
	void Dispatch();

	// Main thread only: blocks until Post or Notify is called after GetGeneration returned generation
	uint64_t GetGeneration() const;
	void WaitForWork( uint64_t generation );

private:
	mutable std::mutex m_mutex;
	std::condition_variable m_condition;
	std::thread::id m_mainThread;
	std::function<void()> m_pump;
	std::vector<std::function<void()>> m_callbacks;
	uint64_t m_generation;
};


// --------------------------------------------------------------------------------------
// Description:
//   Shared state of a Tr2FutureAL object. Asynchronous operations complete it with Resolve
//   or Reject from any thread.
// --------------------------------------------------------------------------------------
template <typename T>
class Tr2FutureStateAL
{
public:
	explicit Tr2FutureStateAL( std::shared_ptr<Tr2AsyncDispatcherAL> dispatcher ) :
		m_dispatcher( std::move( dispatcher ) ),
		m_ready( false ),
		m_result( E_PENDING )
	{
	}

	void Resolve( const T& value )
	{
		Complete( S_OK, &value );
	}

	void Reject( HRESULT result )
	{
		Complete( FAILED( result ) ? result : E_FAIL, nullptr );
	}

	bool IsReady() const
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		return m_ready;
	}

	HRESULT GetResult() const
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		return m_result;
	}

	T GetValue() const
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		return m_value;
	}

	HRESULT Wait()
	{
		if( m_dispatcher->IsMainThread() )
		{
			// Completing the operation may need the main thread, so keep dispatching while waiting
			for( ;; )
			{
				auto generation = m_dispatcher->GetGeneration();
				m_dispatcher->Dispatch();
				if( IsReady() )
				{
					break;
				}
				m_dispatcher->WaitForWork( generation );
			}
		}
		else
		{
			std::unique_lock<std::mutex> lock( m_mutex );
			m_condition.wait( lock, [this] { return m_ready; } );
		}
		return GetResult();
	}

	void Then( std::function<void( const T& )> callback )
	{
		std::unique_lock<std::mutex> lock( m_mutex );
		if( !m_ready )
		{
			m_thenCallbacks.push_back( std::move( callback ) );
		}
		else if( SUCCEEDED( m_result ) )
		{
			T value = m_value;
			lock.unlock();
			m_dispatcher->Post( [callback = std::move( callback ), value]() { callback( value ); } );
		}
	}

	void Catch( std::function<void( HRESULT )> callback )
	{
		std::unique_lock<std::mutex> lock( m_mutex );
		if( !m_ready )
		{
			m_catchCallbacks.push_back( std::move( callback ) );
		}
		else if( FAILED( m_result ) )
		{
			HRESULT result = m_result;
			lock.unlock();
			m_dispatcher->Post( [callback = std::move( callback ), result]() { callback( result ); } );
		}
	}

private:
	void Complete( HRESULT result, const T* value )
	{
		std::vector<std::function<void( const T& )>> thenCallbacks;
		std::vector<std::function<void( HRESULT )>> catchCallbacks;
		{
			std::lock_guard<std::mutex> lock( m_mutex );
			if( m_ready )
			{
				return;
			}
			m_ready = true;
			m_result = result;
			if( value )
			{
				m_value = *value;
			}
			thenCallbacks.swap( m_thenCallbacks );
			catchCallbacks.swap( m_catchCallbacks );
		}
		m_condition.notify_all();

		if( value )
		{
			for( auto& callback : thenCallbacks )
			{
				m_dispatcher->Post( [callback = std::move( callback ), v = *value]() { callback( v ); } );
			}
		}
		else
		{
			for( auto& callback : catchCallbacks )
			{
				m_dispatcher->Post( [callback = std::move( callback ), result]() { callback( result ); } );
			}
		}
		// Main thread may be waiting for this future
		m_dispatcher->Notify();
	}

	std::shared_ptr<Tr2AsyncDispatcherAL> m_dispatcher;
	mutable std::mutex m_mutex;
	std::condition_variable m_condition;
	bool m_ready;
	HRESULT m_result;
	T m_value;
	std::vector<std::function<void( const T& )>> m_thenCallbacks;
	std::vector<std::function<void( HRESULT )>> m_catchCallbacks;
};

}


// --------------------------------------------------------------------------------------
// Description:
//   Result of an asynchronous operation (see Tr2PrimaryRenderContextAL::CreateBufferAsync
//   and Tr2PrimaryRenderContextAL::CreateTextureAsync). The object is cheap to copy; all
//   copies refer to the same operation. Methods are thread-safe.
//
//   Callbacks attached with Then and Catch are always called on the main thread, as part
//   of Tr2PrimaryRenderContextAL::Present or Wait called on the main thread, even if the
//   operation has already completed when they are attached. Callbacks of operations that
//   are still in flight when the render context is destroyed are called during the
//   destruction with E_ABORT error.
// --------------------------------------------------------------------------------------
template <typename T>
class Tr2FutureAL
{
public:
	Tr2FutureAL()
	{
	}

	explicit Tr2FutureAL( std::shared_ptr<TrinityALImpl::Tr2FutureStateAL<T>> state ) :
		m_state( std::move( state ) )
	{
	}

	// Returns true if the future is associated with an operation
	bool IsValid() const
	{
		return m_state != nullptr;
	}

	// Returns true if the operation has completed, successfully or not
	bool IsReady() const
	{
		return m_state && m_state->IsReady();
	}

	// Blocks the calling thread until the operation completes and returns its result. When called
	// on the main thread, runs pending main thread work (including callbacks) while waiting.
	ALResult Wait() const
	{
		if( !m_state )
		{
			return E_INVALIDCALL;
		}
		return m_state->Wait();
	}

	// Returns the result of the operation: S_OK if it succeeded, E_PENDING if it hasn't completed yet
	// and an error if it failed.
	ALResult GetResult() const
	{
		if( !m_state )
		{
			return E_INVALIDCALL;
		}
		return m_state->GetResult();
	}

	// Returns the operation output; default constructed object if the operation hasn't completed or failed
	T Get() const
	{
		if( !m_state )
		{
			return T();
		}
		return m_state->GetValue();
	}

	// Attaches a callback called on the main thread with the operation output once the operation succeeds
	const Tr2FutureAL& Then( std::function<void( const T& )> callback ) const
	{
		if( m_state && callback )
		{
			m_state->Then( std::move( callback ) );
		}
		return *this;
	}

	// Attaches a callback called on the main thread with the error code if the operation fails
	const Tr2FutureAL& Catch( std::function<void( HRESULT )> callback ) const
	{
		if( m_state && callback )
		{
			m_state->Catch( std::move( callback ) );
		}
		return *this;
	}

private:
	std::shared_ptr<TrinityALImpl::Tr2FutureStateAL<T>> m_state;
};
