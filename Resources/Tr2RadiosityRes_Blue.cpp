#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2RadiosityRes.h"

BLUE_DEFINE( Tr2RadiosityRes );


IBlueResource* CreateTr2RadiosityRes( const wchar_t* name )
{
	Tr2RadiosityResPtr p;
	p.CreateInstance();
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"rad", CreateTr2RadiosityRes );

#if BLUE_WITH_PYTHON
static PyObject* PyGetResourceHashInformation( PyObject* self, PyObject* args )
{
	Tr2RadiosityRes* pThis = BluePythonCast<Tr2RadiosityRes*>( self );

	PyObject* l = PyList_New( pThis->m_geometryHashes.size() );
	int cnt = 0;
	for( Tr2RadiosityRes::geometryHashVectorType::const_iterator i = pThis->m_geometryHashes.begin(); i != pThis->m_geometryHashes.end(); ++i, ++cnt )
	{
		PyObject* path = PyUnicode_FromWideChar( (*i).first.c_str(), (*i).first.size() );
		PyList_SET_ITEM( l, cnt, Py_BuildValue( "(O(ii))", path, (*i).second.first, (*i).second.second ));
		Py_DECREF( path );
	}
	
	return l;
}
#endif

const Be::ClassInfo* Tr2RadiosityRes::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2RadiosityRes, "" )

		MAP_INTERFACE( Tr2RadiosityRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_ICACHEABLE_METHODS()
		MAP_METHOD( "GetResourceHashInformation", PyGetResourceHashInformation, "Gets a list (resourcePath,(hash1,hash2)) for each geometry resource that the res was built with" )

	EXPOSURE_CHAINTO( BlueAsyncRes )
}

#endif
