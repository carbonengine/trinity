#pragma once


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
		tmThreadName( 0, 0, "Compile Worker Thread" );

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
