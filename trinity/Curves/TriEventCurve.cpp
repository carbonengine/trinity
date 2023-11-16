#include "StdAfx.h"
#include "TriEventCurve.h"
#include "TriDevice.h"

#if BLUE_WITH_PYTHON
namespace
{
	void EventKeyCallback( void* context )
	{
		TriEventKey* key = reinterpret_cast<TriEventKey*>( context );

		PyObject* result = PyObject_CallObject( key->m_callable, key->m_callableArgs );
		if( result )
		{
			Py_DECREF( result );
		}
		else
		{
			; //TODO: Handle error
		}

		// A reference was added when the callback was set up - release it here.
		key->Unlock();
	}
}
#endif

TriEventCurve::TriEventCurve( IRoot* lockobj ) :
	PARENTLOCK( m_keys ),
	m_time( 0 ),
	m_currentKeyIt( m_keys.begin() ),
	m_extrapolation( TRIEXT_NONE ),
	m_length( 0.0 ),
	m_localTime( 0.0 )
{
}

void TriEventCurve::UpdateValue( double time )
{
	if( m_length == 0.0f )
	{
		return;
	}

	double before = m_time;
	m_time = time;
	if( m_time < before )
	{
		// Time has moved backwards - can't rely on our caching
		m_currentKeyIt = m_keys.begin();
		// Playing an event curve backwards can be defined IFF we know it's being
		// played backwards. However, here we only know that time has moved backwards for
		// this one Update call so we cannot assume that.  So we just have to give up
		// and do nothing. <halldor 2009-03-02>
		return;
	}
	
	switch( m_extrapolation )
	{
		case TRIEXT_CYCLE:
			{
				float localNow = (float)fmod( m_time, (double)m_length );
				if( localNow < m_localTime )
				{
					// We've wrapped around
					m_currentKeyIt = m_keys.begin();
				}
				m_localTime = localNow;
			}
			break;

		default:
			m_localTime = (float)m_time;
			break;
	}
	
	while( (m_currentKeyIt != m_keys.end()) && (m_localTime >= (*m_currentKeyIt)->m_time) )
	{
		// Found a key that has a time value of less than or equal to now
		TriEventKey* currentKey = *m_currentKeyIt;

		m_value = currentKey->m_value;
		
		// Fire off event
#if BLUE_WITH_PYTHON
		if( currentKey->m_callable )
		{
			if( PyCallable_Check( currentKey->m_callable ) )
			{
				currentKey->Lock();
				gTriDev->AddPostUpdateCallback( EventKeyCallback, reinterpret_cast<void*>( currentKey ) );
			}
			else
			{
				CCP_LOGWARN( "TriEventCurve: Key does not hold a callable object" );
			}
		}
		else
#endif
		if( m_eventListener )
		{
			if( !currentKey->m_value.empty() )
			{
				m_eventListener->HandleEvent( currentKey->m_value.c_str() );
			}
		}
		

		++m_currentKeyIt;
	}
}

static bool CompareKeys( IRoot* context, TriEventKey* a, TriEventKey* b )
{
	return a->m_time < b->m_time;
}


void TriEventCurve::AddKey( float time, const std::wstring& evtName )
{
	TriEventKeyPtr key;
	if( !key.CreateInstance() )
	{
		return;
	}

	key->m_time = time;
	key->m_value = evtName.c_str();
	InsertKey(key);
}

#if BLUE_WITH_PYTHON
void TriEventCurve::AddCallableKey( float time, PyObject* callable, PyObject* args )
{
	TriEventKeyPtr key;
	if( !key.CreateInstance() )
	{
		return;
	}

	key->m_time = time;

	key->m_callable = callable;
	Py_XINCREF( key->m_callable );

	PyObject* argsAsTuple;
	if( PyTuple_Check( args ) )
	{
		argsAsTuple = args;
		Py_INCREF( argsAsTuple );
	}
	else
	{
		argsAsTuple = PyTuple_New( 1 );
		PyTuple_SET_ITEM( argsAsTuple, 0, args );
		Py_INCREF( args );
	}
	key->m_callableArgs = argsAsTuple;

	InsertKey(key);
}
#endif

void TriEventCurve::RemoveKey( int ix )
{
	m_keys.Remove( ix );
	m_keys.Sort( (IList::CompareFn)CompareKeys, NULL );
	m_currentKeyIt = m_keys.begin();
	if( m_keys.GetSize() > 0 )
	{
		m_length = m_keys.back()->m_time;
	}
}

int TriEventCurve::GetKeyCount()
{
	return (int)m_keys.GetSize();
}

bool TriEventCurve::Initialize()
{
	// Sort the keys here, just in case somebody tweaked the red file externally :-)
	m_keys.Sort( (IList::CompareFn)CompareKeys, NULL );
	m_currentKeyIt = m_keys.begin();
	if( m_keys.GetSize() > 0 )
	{
		m_length = m_keys.back()->m_time;
	}

	return true;
}

float TriEventCurve::GetKeyTime( int ix )
{
	if( (ix < m_keys.GetSize()) && (ix >= 0) )
	{
		return m_keys[ix]->m_time;
	}
	return 0.0;
}

std::wstring TriEventCurve::GetKeyValue( int ix )
{
	if( (ix < m_keys.GetSize()) && (ix >= 0) )
	{
		return m_keys[ix]->m_value;
	}
	return std::wstring();
}

void TriEventCurve::SetKeyTime( int ix, float time )
{
	if( (ix < m_keys.GetSize()) && (ix >= 0) )
	{
		m_keys[ix]->m_time = time;
		m_keys.Sort( (IList::CompareFn)CompareKeys, NULL );
		m_length = m_keys.back()->m_time;
	}
}

void TriEventCurve::SetKeyValue( int ix, std::wstring value )
{
	if( (ix < m_keys.GetSize()) && (ix >= 0) )
	{
		m_keys[ix]->m_value = value;
	}
}

#if BLUE_WITH_PYTHON
PyObject* TriEventCurve::GetCallableKeyValue( int ix )
{
    if( (ix < m_keys.GetSize()) && (ix >= 0) )
    {
        Py_XINCREF( m_keys[ix]->m_callable );
        return m_keys[ix]->m_callable;
    }
    Py_RETURN_NONE;
}

PyObject* TriEventCurve::GetCallableKeyArgs( int ix )
{
    if( (ix < m_keys.GetSize()) && (ix >= 0) )
    {
        Py_XINCREF( m_keys[ix]->m_callableArgs );
        return m_keys[ix]->m_callableArgs;
    }
    Py_RETURN_NONE;
}
#endif

void TriEventCurve::InsertKey( TriEventKey* key )
{
	m_keys.Insert( -1, key );
	m_keys.Sort( (IList::CompareFn)CompareKeys, NULL );

	m_currentKeyIt = m_keys.begin();
	m_length = m_keys.back()->m_time;
}
