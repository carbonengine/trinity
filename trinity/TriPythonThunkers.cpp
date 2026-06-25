// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if BLUE_WITH_PYTHON

#include "TriPythonThunkers.h"
#include "include/TriColor.h"
#include "include/TriVector.h"
#include "include/TriQuaternion.h"

/////////////////////////////////////////////////////////////////////////////////////////
// ITriScalarFunction
/////////////////////////////////////////////////////////////////////////////////////////


PyObject* ITriScalarFunction_Thunk::PyUpdateScalar( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	if( PyLong_Check( t ) )
	{
		return PyFloat_FromDouble( Update( Be::Time( PyLong_AsLongLong( t ) ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		return PyFloat_FromDouble( Update( PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
}


PyObject* ITriScalarFunction_Thunk::PyGetScalarAt( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	if( PyLong_Check( t ) )
	{
		return PyFloat_FromDouble( GetValueAt( Be::Time( PyLong_AsLongLong( t ) ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		return PyFloat_FromDouble( GetValueAt( PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
// ITriColorFunction
/////////////////////////////////////////////////////////////////////////////////////////

PyObject* ITriColorFunction_Thunk::PyUpdateColor( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	ITriColorPtr q;
	q.Attach( new OTriColor );

	if( PyLong_Check( t ) )
	{
		Color tmpResult;
		q->SetColor( Update( &tmpResult, Be::Time( PyLong_AsLongLong( t ) ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		Color tmpResult;
		q->SetColor( Update( &tmpResult, PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
	return PyOS->WrapBlueObject( q );
}


PyObject* ITriColorFunction_Thunk::PyGetColorAt( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	ITriColorPtr q;
	q.Attach( new OTriColor );

	if( PyLong_Check( t ) )
	{
		Color tmpResult;
		q->SetColor( GetValueAt( &tmpResult, Be::Time( PyLong_AsLongLong( t ) ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		Color tmpResult;
		q->SetColor( GetValueAt( &tmpResult, PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
	return PyOS->WrapBlueObject( q );
}


/////////////////////////////////////////////////////////////////////////////////////////
// ITriVectorFunction
/////////////////////////////////////////////////////////////////////////////////////////


PyObject* ITriVectorFunction_Thunk::PyUpdateVector( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	ITriVectorPtr q;
	q.Attach( new OTriVector );

	if( PyLong_Check( t ) )
	{
		Vector3 tmpResult;
		q->SetVector( GetValueAt( &tmpResult, Be::Time( PyLong_AsLongLong( t ) ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		Vector3 tmpResult;
		q->SetVector( GetValueAt( &tmpResult, PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
	return PyOS->WrapBlueObject( q );
}


PyObject* ITriVectorFunction_Thunk::PyGetVectorAt( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	ITriVectorPtr q;
	q.Attach( new OTriVector );

	if( PyLong_Check( t ) )
	{
		Vector3 tmpResult( 0, 0, 0 );
		q->SetVector( GetValueAt( &tmpResult, Be::Time( PyLong_AsLongLong( t ) ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		Vector3 tmpResult( 0, 0, 0 );
		q->SetVector( GetValueAt( &tmpResult, PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
	return PyOS->WrapBlueObject( q );
}

PyObject* ITriVectorFunction_Thunk::PyGetVectorDotAt( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	ITriVectorPtr q;
	q.Attach( new OTriVector );

	if( PyLong_Check( t ) )
	{
		Vector3 tmpResult;
		q->SetVector( GetValueDotAt( &tmpResult, Be::Time( PyLong_AsLongLong( t ) ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		Vector3 tmpResult;
		q->SetVector( GetValueDotAt( &tmpResult, PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
	return PyOS->WrapBlueObject( q );
}

PyObject* ITriVectorFunction_Thunk::PyGetVectorDoubleDotAt( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	ITriVectorPtr q;
	q.Attach( new OTriVector );

	if( PyLong_Check( t ) )
	{
		Vector3 tmpResult;
		q->SetVector( GetValueDoubleDotAt( &tmpResult, Be::Time( PyLong_AsLongLong( t ) ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		Vector3 tmpResult;
		q->SetVector( GetValueDoubleDotAt( &tmpResult, PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
	return PyOS->WrapBlueObject( q );
}



/////////////////////////////////////////////////////////////////////////////////////////
// ITriQuaternionFunction
/////////////////////////////////////////////////////////////////////////////////////////

PyObject* ITriQuaternionFunction_Thunk::PyUpdateQuaternion( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	ITriQuaternionPtr q;
	q.Attach( new OTriQuaternion );

	if( PyLong_Check( t ) )
	{
		Quaternion tmpResult;
		q->SetQuaternion( Update( &tmpResult, Be::Time( PyLong_AsLongLong( t ) ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		Quaternion tmpResult;
		q->SetQuaternion( Update( &tmpResult, PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
	return PyOS->WrapBlueObject( q );
}


PyObject* ITriQuaternionFunction_Thunk::PyGetQuaternionAt( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	ITriQuaternionPtr q;
	q.Attach( new OTriQuaternion );

	if( PyLong_Check( t ) )
	{
		Quaternion tmpResult;
		q->SetQuaternion( GetValueAt( &tmpResult, Be::Time( PyLong_AsLongLong( t ) ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		Quaternion tmpResult;
		q->SetQuaternion( GetValueAt( &tmpResult, PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
	return PyOS->WrapBlueObject( q );
}


PyObject* ITriQuaternionFunction_Thunk::PyGetQuaternionDotAt( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	ITriQuaternionPtr q;
	q.Attach( new OTriQuaternion );

	if( PyLong_Check( t ) )
	{
		Quaternion tmpResult;
		q->SetQuaternion( GetValueDotAt( &tmpResult, Be::Time( PyLong_AsLongLong( t ) ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		Quaternion tmpResult;
		q->SetQuaternion( GetValueDotAt( &tmpResult, PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
	return PyOS->WrapBlueObject( q );
}


PyObject* ITriQuaternionFunction_Thunk::PyGetQuaternionDoubleDotAt( PyObject* args )
{
	PyObject* t;
	if( !PyArg_ParseTuple( args, "O", &t ) )
		return NULL;

	ITriQuaternionPtr q;
	q.Attach( new OTriQuaternion );

	if( PyLong_Check( t ) )
	{
		Quaternion tmpResult;
		q->SetQuaternion( GetValueDoubleDotAt( &tmpResult, Be::Time( PyLong_AsLongLong( t ) ) ) );
	}
	else if( PyFloat_Check( t ) )
	{
		Quaternion tmpResult;
		q->SetQuaternion( GetValueDoubleDotAt( &tmpResult, PyFloat_AS_DOUBLE( t ) ) );
	}
	else
	{
		BeOS->SetError( BEDEF, Clsid(), "arg must be of type LongLong (Be::Time) or float" );
		return nullptr;
	}
	return PyOS->WrapBlueObject( q );
}




#endif
