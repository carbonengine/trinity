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

		MAP_ATTRIBUTE( "geometry", m_geometryResource, "Procedural geometry", Be::READWRITE )
		
		MAP_METHOD_AND_WRAP(
			"SetGeometryRes",
			PySetGeometryRes,
			"Set the geometry resource used by this mesh - bypassing the regular method of setting a resource name.\n"
			"This is used for geometry resources that require special handling, such as pre-baked blendshapes.\n"
			":param geometry: new geometry resource" )

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
