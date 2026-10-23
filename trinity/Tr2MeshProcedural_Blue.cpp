// Copyright © 2026 CCP ehf.

#include "StdAfx.h"

#include "Tr2MeshProcedural.h"

#include "Resources/Tr2GeometryProvider.h"
#include "TriConstants.h"


BLUE_DEFINE( Tr2MeshProcedural );

#if BLUE_WITH_PYTHON

static PyObject* PyGetAreas( PyObject* self, PyObject* args )
{
	Tr2MeshProcedural* pThis = BluePythonCast<Tr2MeshProcedural*>( self );
	if( !pThis )
	{
		return NULL;
	}

	int areaIndex;

	if( !PyArg_ParseTuple( args, "i", &areaIndex ) )
	{
		PyErr_SetString( PyExc_TypeError, "Arguments must be (i)." );
		return 0;
	}

	TriBatchType type( static_cast<TriBatchType>( areaIndex ) );

	Tr2MeshAreaVector* areas( pThis->GetAreas( type ) );

	if( !areas )
	{
		Py_RETURN_NONE;
	}

	return PyOS->WrapBlueObject( areas->GetRawRoot() );
}
#endif

const Be::ClassInfo* Tr2MeshProcedural::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2MeshProcedural, "" )

		MAP_INTERFACE( Tr2MeshProcedural )

		MAP_ATTRIBUTE( "geometry", m_geometryResource, "", Be::HIDDEN ) // Needed to make Graphite able to detect bindings inside of the geometry
		MAP_PROPERTY( "geometry", GetGeometry, SetGeometryResource, "Procedural geometry" )

		MAP_METHOD(
			"GetAreas",
			PyGetAreas,
			"Returns mesh area list\n"
			":param areaType: area type\n"
			":type areaType: int\n"
			":rtype: None | List" );
		MAP_METHOD_AND_WRAP( "GetAreasCount", GetAreasCount, "" );

	EXPOSURE_CHAINTO( Tr2MeshBase )
}
