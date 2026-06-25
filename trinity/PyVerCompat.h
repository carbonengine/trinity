// Copyright © 2025 CCP ehf.

#pragma once

namespace PyVerCompat
{

inline bool IsPyString( PyObject* pyObj )
{
#if PY_MAJOR_VERSION == 2
	return PyString_Check( pyObj );
#else
	return PyUnicode_Check( pyObj );
#endif
}

inline bool IsPyInt( PyObject* pyObj )
{
#if PY_MAJOR_VERSION == 2
	return PyInt_Check( pyObj );
#else
	return PyLong_Check( pyObj );
#endif
}

inline PyObject* ToPyBytes( const char* str, size_t size )
{
#if PY_MAJOR_VERSION == 2
	return PyString_FromStringAndSize( str, size );
#else
	return PyBytes_FromStringAndSize( str, size );
#endif
}

inline PyObject* MemoryViewRW( void* data, Py_ssize_t size )
{
#if PY_MAJOR_VERSION == 2
	return PyBuffer_FromReadWriteMemory( data, size );
#else
	return PyMemoryView_FromMemory( static_cast<char*>( data ), size, PyBUF_WRITE );
#endif
}

}

template <typename T>
inline T FromPython( PyObject* pyObj )
{
	T result{};
	if( !BlueExtractArgument( pyObj, result, 0 ) )
	{
		PyErr_Clear();
	}
	return result;
}

template <typename T>
inline PyObject* ToPython( const T& value )
{
	return BlueWrapReturnValue( {}, value );
}
