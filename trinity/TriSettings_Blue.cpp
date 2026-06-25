// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriSettings.h"

BLUE_DEFINE_ABSTRACT( TriSettings );

std::string TriSettings::GetSettingReprString( const TriSettings::Setting* s )
{
	char buff[512];
	switch( s->m_type )
	{
	case Be::LONG:
		sprintf_s( buff, "%d", *(int*)s->m_var );
		break;
	case Be::BOOL:
		sprintf_s( buff, *(bool*)s->m_var ? "True" : "False" );
		break;
	case Be::FLOAT: {
		double d = *(float*)s->m_var;
		sprintf_s( buff, "%f", d );
	}
	break;
	case Be::DOUBLE:
		sprintf_s( buff, "%f", *(double*)s->m_var );
		break;
	case Be::SHORT: {
		int d = *(short*)s->m_var;
		sprintf_s( buff, "%d", d );
	}
	break;
	case Be::CSTRING: {
		const char* string = *(const char**)s->m_var;
		sprintf_s( buff, "'%s'", string ? string : "" );
	}
	break;
	default:
		sprintf_s( buff, "NOT IMPLEMENTED YET! DO IT!" );
		break;
	}

	return buff;
}

#if BLUE_WITH_PYTHON
PyObject* PyRepr( PyObject* self, PyObject* args )
{
	TriSettings* pThis = BluePythonCast<TriSettings*>( self );
	// the cast above will always succeed

	std::string repr = pThis->GetReprString();
	return ToPython( repr.c_str() );
}

static PyObject* PyGetValue( PyObject* self, PyObject* args )
{
	TriSettings* pThis = BluePythonCast<TriSettings*>( self );
	// the cast above will always succeed

	const char* key;
	if( !PyArg_ParseTuple( args, "s", &key ) )
	{
		PyErr_SetString( PyExc_TypeError, "Function accepts one string argument!" );
		return NULL;
	}

	TriSettings::Setting* s = pThis->FindSetting( key );
	if( !s )
	{
		PyErr_SetString( PyExc_LookupError, "Setting name not registered!" );
		return NULL;
	}

	// Construct mostly bogus Be::VarEntry to use the BluePython converters.
	Be::VarEntry entry;
	entry.mType = s->m_type;
	entry.mSize = s->m_size;

	return BlueConvertValueToPython( &entry, s->m_var );
}

static PyObject* PySetValue( PyObject* self, PyObject* args )
{
	TriSettings* pThis = BluePythonCast<TriSettings*>( self );
	// the cast above will always succeed

	const char* key;
	PyObject* value;
	if( !PyArg_ParseTuple( args, "sO", &key, &value ) )
	{
		PyErr_SetString( PyExc_TypeError, "Function accepts key, value where key is a string!" );
		return NULL;
	}

	TriSettings::Setting* s = pThis->FindSetting( key );
	if( !s )
	{
		PyErr_SetString( PyExc_LookupError, "Setting name not registered!" );
		return NULL;
	}

	// Construct mostly bogus Be::VarEntry to use the BluePython converters.
	Be::VarEntry entry;
	entry.mType = s->m_type;
	entry.mSize = s->m_size;
	entry.mName = key;

	bool ok = BlueConvertValueFromPython( &entry, s->m_var, value );
	if( !ok )
	{
		return NULL;
	}

	Py_RETURN_NONE;
}
#endif


const Be::ClassInfo* TriSettings::ExposeToBlue()
{
	int bingo;
	CTriSettings s;
	s.RegisterSetting( "bingo", &bingo );
	EXPOSURE_BEGIN( TriSettings, "Encapsulates settings for Trinity" )
		MAP_INTERFACE( TriSettings )

		MAP_METHOD(
			"GetValue",
			PyGetValue,
			"Returns a copy of the value assigned to the string key passed in\n"
			":param name: setting name\n"
			":type name: str\n" )
		MAP_METHOD(
			"SetValue",
			PySetValue,
			"Sets the string key to the value passed in\n"
			":param name: setting name\n"
			":type name: str\n"
			":param value: setting value\n" )
		MAP_METHOD(
			"__repr__",
			PyRepr,
			"Returns a string representation for the object" )

	EXPOSURE_END()
}
