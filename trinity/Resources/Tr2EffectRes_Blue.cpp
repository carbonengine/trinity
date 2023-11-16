#include "StdAfx.h"
#include "Tr2EffectRes.h"

BLUE_DEFINE( Tr2EffectRes );

#if BLUE_WITH_PYTHON
PyObject* Tr2EffectRes::GetPermutationDescription()
{
	if( !IsGood() )
	{
		PyErr_SetString( PyExc_ValueError, "effect is not prepared" );
		return nullptr;
	}
	PyObject* result = PyTuple_New( Py_ssize_t( m_permutations.size() ) );
	for( size_t i = 0; i < m_permutations.size(); ++i )
	{
		PyObject* desc = PyDict_New();
		PyObject* element;
		element = PyString_FromString( m_permutations[i].name.c_str() );
		PyDict_SetItemString( desc, "name", element );
		Py_DECREF( element );
		element = PyTuple_New( Py_ssize_t( m_permutations[i].options.size() ) );
		for( size_t j = 0; j < m_permutations[i].options.size(); ++j )
		{
			PyTuple_SetItem( element, Py_ssize_t( j ), PyString_FromString(m_permutations[i].options[j].c_str()));
		}
		PyDict_SetItemString( desc, "options", element );
		Py_DECREF( element );
		element = PyInt_FromSize_t( m_permutations[i].defaultOption );
		PyDict_SetItemString( desc, "default", element );
		Py_DECREF( element );
		element = PyString_FromString( m_permutations[i].description.c_str() );
		PyDict_SetItemString( desc, "description", element );
		Py_DECREF( element );
		element = PyInt_FromSize_t( m_permutations[i].type );
		PyDict_SetItemString( desc, "type", element );
		Py_DECREF( element );

		PyTuple_SetItem( result, Py_ssize_t( i ), desc );
	}
	return result;
}

#endif


const Be::ClassInfo* Tr2EffectRes::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2EffectRes, "" )

		MAP_INTERFACE( Tr2EffectRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_ICACHEABLE_METHODS()

#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP(
			"GetPermutationDescription",
			GetPermutationDescription,
			"Returns a description of effect permutations as a tuple of dictionaries.\n"
			"Each dictionary contains a tuple \"options\" of permutation options, \"default\" value index and \"description\"."
			)
#endif

	EXPOSURE_CHAINTO( BlueAsyncRes )
}
