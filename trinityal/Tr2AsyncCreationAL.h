// Copyright © 2026 CCP ehf.

#pragma once

#include "include/Tr2FutureAL.h"

#include <shared_mutex>


namespace TrinityALImpl
{

// --------------------------------------------------------------------------------------
// Description:
//   Asynchronous creation request. Platform backends execute the creation part of a job
//   (possibly on a worker thread) and then call Finish on the main thread.
// --------------------------------------------------------------------------------------
class Tr2AsyncJobAL
{
public:
	virtual ~Tr2AsyncJobAL()
	{
	}

	// Main thread: completes the creation and resolves (or rejects) the future
	virtual void Finish() = 0;

	// Rejects the future with the given error and drops anything created so far
	virtual void Cancel( HRESULT result ) = 0;
};


// --------------------------------------------------------------------------------------
// Description:
//   Job that executes its creation part with a platform specific context (Context type).
// --------------------------------------------------------------------------------------
template <typename Context>
class Tr2AsyncContextJobAL : public Tr2AsyncJobAL
{
public:
	virtual void Execute( Context& context ) = 0;

	// Fails a job after Execute (for example, when submitting the work it has recorded fails)
	virtual void SetExecuteError( HRESULT result ) = 0;
};


// --------------------------------------------------------------------------------------
// Description:
//   Creates an implementation object of type Impl and wraps it into public object of type
//   Public when finished. The execute function runs the creation part, the finish function
//   the main thread part.
// --------------------------------------------------------------------------------------
template <typename Public, typename Impl, typename Context>
class Tr2AsyncCreateJobAL : public Tr2AsyncContextJobAL<Context>
{
public:
	typedef std::function<HRESULT( Impl&, Context& )> ExecuteFunction;
	typedef std::function<HRESULT( Impl& )> FinishFunction;
	typedef std::function<Public( std::shared_ptr<Impl> )> WrapFunction;

	Tr2AsyncCreateJobAL(
		std::shared_ptr<Tr2FutureStateAL<Public>> state,
		ExecuteFunction execute,
		FinishFunction finish,
		WrapFunction wrap ) :
		m_state( std::move( state ) ),
		m_execute( std::move( execute ) ),
		m_finish( std::move( finish ) ),
		m_wrap( std::move( wrap ) ),
		m_result( E_PENDING )
	{
	}

	void Execute( Context& context ) override
	{
		m_object = std::make_shared<Impl>();
		m_result = m_execute( *m_object, context );
	}

	void SetExecuteError( HRESULT result ) override
	{
		if( SUCCEEDED( m_result ) )
		{
			m_result = result;
		}
	}

	void Finish() override
	{
		HRESULT result = m_result;
		if( SUCCEEDED( result ) && m_finish )
		{
			result = m_finish( *m_object );
		}
		if( SUCCEEDED( result ) )
		{
			m_state->Resolve( m_wrap( std::move( m_object ) ) );
		}
		else
		{
			Cancel( result );
		}
	}

	void Cancel( HRESULT result ) override
	{
		m_object.reset();
		m_state->Reject( result );
	}

private:
	std::shared_ptr<Tr2FutureStateAL<Public>> m_state;
	ExecuteFunction m_execute;
	FinishFunction m_finish;
	WrapFunction m_wrap;
	std::shared_ptr<Impl> m_object;
	HRESULT m_result;
};


// --------------------------------------------------------------------------------------
// Description:
//   Worker thread that executes asynchronous creation jobs in batches. Executed jobs are
//   finished on the main thread by FinishCompleted (which the owner usually registers as
//   the dispatcher pump). Platform backends derive from this class and implement
//   ExecuteBatch.
// --------------------------------------------------------------------------------------
class Tr2AsyncWorkerAL
{
public:
	explicit Tr2AsyncWorkerAL( std::shared_ptr<Tr2AsyncDispatcherAL> dispatcher );
	virtual ~Tr2AsyncWorkerAL();

	// Starts the worker thread
	void Start();

	// Main thread: stops the worker thread (after it finishes executing the current batch) and
	// cancels all jobs that have not been finished yet with E_ABORT
	void Stop();

	// Thread-safe: queues a job for execution. The job is cancelled if the worker is not running.
	void Submit( std::shared_ptr<Tr2AsyncJobAL> job );

	// Main thread: finishes jobs executed by the worker thread
	void FinishCompleted();

protected:
	// Worker thread: executes the creation part of the jobs
	virtual void ExecuteBatch( const std::vector<std::shared_ptr<Tr2AsyncJobAL>>& jobs ) = 0;

	// Maximum number of jobs in a single batch
	static const size_t MAX_BATCH_SIZE = 32;

private:
	void Run();

	std::shared_ptr<Tr2AsyncDispatcherAL> m_dispatcher;
	std::mutex m_mutex;
	std::condition_variable m_condition;
	std::vector<std::shared_ptr<Tr2AsyncJobAL>> m_pending;
	std::vector<std::shared_ptr<Tr2AsyncJobAL>> m_completed;
	bool m_running;
	bool m_stopRequested;
	std::thread m_thread;
};


// --------------------------------------------------------------------------------------
// Description:
//   Guards a backend against shutting down while other threads are creating resources
//   with it. Creation takes shared access; shutdown takes exclusive access once.
// --------------------------------------------------------------------------------------
class Tr2AsyncShutdownGuardAL
{
public:
	Tr2AsyncShutdownGuardAL() :
		m_isShutDown( false )
	{
	}

	// Calls function with shared access; returns false (without calling it) after shutdown
	template <typename Function>
	bool Use( Function&& function )
	{
		std::shared_lock<std::shared_mutex> lock( m_mutex );
		if( m_isShutDown )
		{
			return false;
		}
		function();
		return true;
	}

	// Waits for all current users to finish and prevents future ones
	void Shutdown()
	{
		std::unique_lock<std::shared_mutex> lock( m_mutex );
		m_isShutDown = true;
	}

private:
	std::shared_mutex m_mutex;
	bool m_isShutDown;
};

}
