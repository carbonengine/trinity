// Copyright © 2023 CCP ehf.

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
		element = ToPython( m_permutations[i].name );
		PyDict_SetItemString( desc, "name", element );
		Py_DECREF( element );
		element = PyTuple_New( Py_ssize_t( m_permutations[i].options.size() ) );
		for( size_t j = 0; j < m_permutations[i].options.size(); ++j )
		{
			PyTuple_SetItem( element, Py_ssize_t( j ), ToPython( m_permutations[i].options[j] ) );
		}
		PyDict_SetItemString( desc, "options", element );
		Py_DECREF( element );
		element = ToPython( m_permutations[i].defaultOption );
		PyDict_SetItemString( desc, "default", element );
		Py_DECREF( element );
		element = ToPython( m_permutations[i].description );
		PyDict_SetItemString( desc, "description", element );
		Py_DECREF( element );
		element = ToPython( m_permutations[i].type );
		PyDict_SetItemString( desc, "type", element );
		Py_DECREF( element );

		PyTuple_SetItem( result, Py_ssize_t( i ), desc );
	}
	return result;
}

void ModifyGlobalOptions( const std::vector<std::pair<BlueSharedString, BlueSharedString>>& change, std::vector<BlueSharedString>& remove )
{
	std::vector<Tr2ShaderOption> changes;
	for( auto& ch : change )
	{
		changes.push_back( { ch.first, ch.second } );
	}
	for( auto& name : remove )
	{
		changes.push_back( { name } );
	}
	ModifyGlobalEffectOptions( changes );
}

MAP_FUNCTION_AND_WRAP(
	"ModifyGlobalEffectOptions",
	ModifyGlobalOptions,
	"Sets global effect options. Calling this function will result in rebuilding of all effects (slow).\n"
	":param change: list of pairs (name, value) for options\n"
	":param remove: list of names to remove from global option set\n" );

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
			"Each dictionary contains a tuple \"options\" of permutation options, \"default\" value index and \"description\"." )
#endif

	EXPOSURE_CHAINTO( BlueAsyncRes )
}
