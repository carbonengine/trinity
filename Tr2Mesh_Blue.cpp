#include "StdAfx.h"

#include "Tr2Mesh.h"

#include "Resources/TriGeometryRes.h"
#include "TriConstants.h"

BLUE_DEFINE( Tr2Mesh );

#if BLUE_WITH_PYTHON
static PyObject* PyBindLowLevelShaders( PyObject* self, PyObject* args )
{
	Tr2Mesh* pThis = BluePythonCast<Tr2Mesh*>( self );
	if( !pThis )
	{
		return NULL;
	}
	
	PyObject *pyValueList;
	PyObject *store = NULL;
	bool overrideDefaultSituation = false;
	if( !PyArg_ParseTuple(args, "O|bO", &pyValueList, &overrideDefaultSituation, &store) 
		|| !PyList_Check(pyValueList) )
	{
		PyErr_SetString(PyExc_TypeError, "Argument to PyBindLowLevelShaders must be a list.");
		return NULL;
	}

	size_t count = PyList_Size(pyValueList);

	std::vector<unsigned int> engineFlags;

	for(size_t i = 0; i < count; i++)
	{
		
		PyObject* pyValueDef = PyList_GetItem(pyValueList, i);
		if (PyString_Check(pyValueDef))
		{
			char * v = PyString_AsString(pyValueDef);
			unsigned int h = CcpHashFNV1( v, strlen( v ) );
			engineFlags.push_back(h);
		}
	}

	Tr2VariableStore* variableStore = NULL;
	if( store )
	{
		variableStore = BluePythonCast<Tr2VariableStore*>( store );
		if( !variableStore )
		{
			return NULL;
		}
	}

	pThis->BindLowLevelShaders( engineFlags, overrideDefaultSituation, variableStore );

	Py_RETURN_NONE;
}

static PyObject* PyGetAreas( PyObject* self, PyObject* args )
{
    Tr2Mesh* pThis = BluePythonCast<Tr2Mesh*>( self );
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

const Be::ClassInfo* Tr2Mesh::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2Mesh, "" )

		MAP_INTERFACE( Tr2Mesh )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IUnloadable )

		MAP_ATTRIBUTE( "geometry", m_geometryResource, "na", Be::READ )			
		MAP_ATTRIBUTE_WITH_CHOOSER
		( 
			"geometryResPath", 
			m_meshResPath, 
			"Resource path to granny file", 
			Be::READWRITE | Be::NOTIFY | Be::PERSIST, 
			TriGR2Chooser
		)
		MAP_ATTRIBUTE( "deferGeometryLoad", m_deferGeometryLoad, "Defers geometry load when loading mesh objects", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE
		( 
			"immutable", 
			m_immutable, 
			"Can the geometry be stored in immutable buffers? Note this needs to\n"
			"set before loading, changes will not trigger an auto-reload of gr2.", 
			Be::READWRITE | Be::PERSIST 
		)
		MAP_ATTRIBUTE
		( 
			"computeAccess", 
			m_computeAccess, 
			"If true, make the VB/IB visible to compute shaders (experimental).", 
			Be::READWRITE | Be::PERSIST 
		)
		MAP_ATTRIBUTE
		(
			"isLoading", 
			m_isLoading, 
			"If set, mesh resources might still be loading",
			Be::READ
		)

		MAP_METHOD_AND_WRAP( "SetGeometryRes", PySetGeometryRes, "Set the geometry resource used by this mesh - bypassing the regular method of setting a resource name. This is used for geometry resources that require special handling, such as pre-baked blendshapes." )
		MAP_METHOD( "BindLowLevelShaders", PyBindLowLevelShaders, "Bind a new low level shader to all areas based on the current situation" )

        MAP_METHOD( "GetAreas", PyGetAreas, "" );
		MAP_METHOD_AND_WRAP( "GetAreasCount", GetAreasCount, "" );

    EXPOSURE_CHAINTO( Tr2MeshBase )
}
