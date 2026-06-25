// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if BLUE_WITH_PYTHON

#include "Tr2PyValueBinding.h"

Tr2PyValueBinding::Tr2PyValueBinding( IRoot* lockobj ) :
	m_isValid( false )
{
	m_sourceObject = Py_None;
	Py_INCREF( m_sourceObject );
	m_destinationObject = Py_None;
	Py_INCREF( m_destinationObject );
}

Tr2PyValueBinding::~Tr2PyValueBinding()
{
	Py_DECREF( m_sourceObject );
	Py_DECREF( m_destinationObject );
}

void Tr2PyValueBinding::CopyValue()
{
	if( m_isValid )
	{
		PyObject* value = PyObject_GetAttrString( m_sourceObject, m_sourceAttribute.c_str() );
		if( value )
		{
			PyObject_SetAttrString( m_destinationObject, m_destinationAttribute.c_str(), value );
			Py_DECREF( value );
		}
	}
}

bool Tr2PyValueBinding::OnModified( Be::Var* val )
{
	Initialize();
	return true;
}

void Tr2PyValueBinding::Initialize()
{
	m_isValid = false;

	if( m_sourceObject != Py_None && m_destinationObject != Py_None && !m_sourceAttribute.empty() && !m_destinationAttribute.empty() )
	{
		// We have valid Python objects and attributes have been specified.
		// We don't check here if attributes are valid - this may change and
		// this setup is only done once.
		m_isValid = true;
	}
}
#endif
