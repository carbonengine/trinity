// Copyright © 2021 CCP ehf.

#include "StdAfx.h"
#include "Tr2GpuProfiler.h"


namespace
{

CTr2GpuProfiler s_profiler;

}


Tr2GpuProfiler::Tr2GpuProfiler( IRoot* ) :
	m_pendingCapture( false ),
	m_capturing( false )
{
}

Tr2GpuProfiler& Tr2GpuProfiler::GetProfiler()
{
	return s_profiler;
}

void Tr2GpuProfiler::Capture()
{
	m_pendingCapture = true;
}

bool Tr2GpuProfiler::IsCapturing()
{
	return m_capturing;
}

void Tr2GpuProfiler::BeginFrame( uint64_t frameNumber )
{
	if( m_pendingCapture )
	{
		m_capturing = true;
		m_pendingCapture = false;
		m_zones.clear();
		m_stack.clear();
		m_frameFence = Tr2FenceAL();

		char buffer[128];
		sprintf_s( buffer, "Frame %llu", frameNumber );

		USE_MAIN_THREAD_RENDER_CONTEXT();
		Begin( nullptr, buffer, renderContext );
	}
}

void Tr2GpuProfiler::EndFrame()
{
	if( m_capturing )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR( m_frameFence.Create( renderContext ) );
		CR( m_frameFence.PutFence( renderContext ) );

		while( !m_stack.empty() )
		{
			End( renderContext );
		}
		m_capturing = false;
	}
}

void Tr2GpuProfiler::Begin( IRoot* owner, const char* label, Tr2RenderContextAL& context )
{
	if( m_capturing )
	{
		Zone capture;
		capture.type = Zone::REGION_BEGIN;
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			CR( capture.query.Create( renderContext ) );
			CR( capture.timer.Create( renderContext ) );
		}
		capture.owner = owner;
		capture.message = label;
		CR( capture.query.Begin( context ) );
		capture.timer.Begin( context );
		m_stack.push_back( m_zones.size() );
		m_zones.push_back( capture );
	}
}

void Tr2GpuProfiler::End( Tr2RenderContextAL& renderContext )
{
	if( m_capturing )
	{
		auto& last = m_zones[m_stack.back()];
		m_stack.pop_back();
		CR( last.query.End( renderContext ) );
		last.timer.End( renderContext );

		Zone capture;
		capture.type = Zone::REGION_END;
		m_zones.push_back( capture );
	}
}

bool Tr2GpuProfiler::IsDataReady()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	bool result = false;
	m_frameFence.IsReached( result, renderContext ).GetResult();
	return result;
}

BlueScriptValue Tr2GpuProfiler::GetFrameReport()
{
#if BLUE_WITH_PYTHON
	if( !IsDataReady() )
	{
		return PyErr_SetString( PyExc_RuntimeError, "Data is not ready" ), nullptr;
	}

	size_t index = 0;
	auto roots = GetRegionReport( index );
	BlueScriptValue root;
	if( PyObject_Length( roots ) > 0 )
	{
		root = PyList_GetItem( roots, 0 );
	}
	else
	{
		root = Py_None;
	}
	Py_IncRef( root );
	Py_DecRef( roots );
	return root;
#else
	return BlueScriptValue();
#endif
}

BlueScriptValue Tr2GpuProfiler::GetRegionReport( size_t& index )
{
#if BLUE_WITH_PYTHON
	USE_MAIN_THREAD_RENDER_CONTEXT();

	auto result = PyList_New( 0 );

	while( index < m_zones.size() )
	{
		auto& zone = m_zones[index++];
		if( zone.type == Zone::REGION_END )
		{
			return result;
		}

		PyObject* children;
		if( zone.type == Zone::REGION_BEGIN )
		{
			children = GetRegionReport( index );
		}
		else
		{
			children = PyList_New( 0 );
		}

		auto stats = PyDict_New();
		{
			auto value = PyFloat_FromDouble( zone.timer.GetTime( renderContext ) );
			PyDict_SetItemString( stats, "GPU Time", value );
			Py_DecRef( value );
		}
		Tr2PipelineStatsDataAL data;
		if( SUCCEEDED( zone.query.GetStats( data, renderContext ) ) )
		{
			auto count = Tr2PipelineStatsQueryAL::GetValueCount( data );
			for( size_t i = 0; i < count; ++i )
			{
				auto value = PyLong_FromLongLong( static_cast<long long>( Tr2PipelineStatsQueryAL::GetValue( data, i ) ) );
				PyDict_SetItemString( stats, Tr2PipelineStatsQueryAL::GetLabel( data, i ), value );
				Py_DecRef( value );
			}
		}
		PyObject* owner;
		if( zone.owner )
		{
			owner = PyOS->WrapBlueObject( zone.owner );
		}
		else
		{
			owner = Py_None;
			Py_IncRef( owner );
		}
		PyObject* item = Py_BuildValue( "OsOO", owner, zone.message.c_str(), stats, children );
		Py_DecRef( stats );
		Py_DecRef( owner );

		PyList_Append( result, item );
		Py_DecRef( item );
		Py_DecRef( children );
	}
	return result;
#else
	return BlueScriptValue();
#endif
}

void Tr2GpuProfiler::ClearData()
{
	m_zones.clear();
	m_stack.clear();
}


Tr2GpuProfilerZone::Tr2GpuProfilerZone( IRoot* owner, const char* label, Tr2RenderContextAL& renderContext ) :
	context( renderContext )
{
	Tr2GpuProfiler::GetProfiler().Begin( owner, label, renderContext );
}

Tr2GpuProfilerZone::~Tr2GpuProfilerZone()
{
	Tr2GpuProfiler::GetProfiler().End( context );
}
