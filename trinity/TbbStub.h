////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//

#pragma once
#ifndef TbbStub_H
#define TbbStub_H

#if TBB_ENABLED

#define Tr2ParallelFor tbb::parallel_for
#define Tr2ParallelDo tbb::parallel_for_each
#define Tr2BlockedRange tbb::blocked_range
#define Tr2ParallelSort tbb::parallel_sort
#define Tr2EnumerableThreadSpecific tbb::enumerable_thread_specific
#define Tr2SpinMutex tbb::spin_mutex
#define Tr2ParallelTaskGroup tbb::task_group

#else

struct Tr2SpinMutex
{
	struct scoped_lock
	{
		scoped_lock( Tr2SpinMutex& )
		{
		}
	};
};

#define Tr2ParallelDo std::for_each
#define Tr2ParallelSort std::sort

template <typename T>
struct Tr2BlockedRange
{
	Tr2BlockedRange( T begin, T end, T step = 1 )
		:m_begin( begin ),
		m_end( end ),
		m_step( step )
	{
	}

	T begin() const
	{
		return m_begin;
	}

	T end() const
	{
		return m_end;
	}

	T m_begin;
	T m_end;
	T m_step;
};

template <typename T, typename Func>
inline void Tr2ParallelFor( const Tr2BlockedRange<T>& range, Func func )
{
	for( T begin = range.begin(); begin < range.end(); begin += range.m_step )
	{
		func( Tr2BlockedRange<T>( begin, std::min( begin + range.m_step, range.end() ) ) );
	}
}

template <typename T>
class Tr2EnumerableThreadSpecific
{
public:
	Tr2EnumerableThreadSpecific()
		:m_data()
	{
	}

	T& local()
	{
		return m_data;
	}

	T* begin()
	{
		return &m_data;
	}

	T* end()
	{
		return ( &m_data ) + 1;
	}
private:
	T m_data;
};

struct Tr2ParallelTaskGroup
{
	template <typename F>
	void run( F&& f )
	{
		F();
	}
	void wait()
	{
	}
};

#endif


#endif