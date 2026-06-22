// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriEventKey.h"

TriEventKey::TriEventKey( IRoot* lockobj ) :
	m_time( 0.0f )
#if BLUE_WITH_PYTHON
	,
	m_callable( NULL ),
	m_callableArgs( NULL )
#endif
{
}

TriEventKey::~TriEventKey()
{
#if BLUE_WITH_PYTHON
	Py_XDECREF( m_callable );
	Py_XDECREF( m_callableArgs );
#endif
}