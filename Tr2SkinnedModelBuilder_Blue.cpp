#include "StdAfx.h"
#include "Tr2SkinnedModelBuilder.h"
#include "Tr2SkinnedModelBuilderSource.h"
#include "Tr2SkinnedObject.h"

// ------------------------------------------------------------------------------------------------------
#if BLUE_WITH_PYTHON
static PyObject* PySetExcludeMeshes( PyObject* self, PyObject* args )
{
	Tr2SkinnedModelBuilderSource* pThis = BluePythonCast<Tr2SkinnedModelBuilderSource*>( self );
	
	PyObject* v = PyTuple_GetItem( args, 0 );
	// Setter
	pThis->m_excludeMeshes.clear();
	if( !PyList_Check( v ) )
	{
		PyErr_SetString( PyExc_TypeError, "Argument must be a list of strings" );
		return NULL;
	}
	for( Py_ssize_t i = 0; i < PyList_GET_SIZE(v); ++i )
	{
		PyObject* item = PyList_GetItem( v, i );
		if( !PyString_Check( item ) )
		{
			PyErr_SetString( PyExc_TypeError, "Argument must be a list of strings" );
			return NULL;
		}

		pThis->m_excludeMeshes.push_back( std::string( PyString_AsString( item ), PyString_Size( item ) ) );
	}
	Py_RETURN_NONE;
}

// ------------------------------------------------------------------------------------------------------
static PyObject* PyGetExcludeMeshes( PyObject* self, PyObject* args )
{
	Tr2SkinnedModelBuilderSource* pThis = BluePythonCast<Tr2SkinnedModelBuilderSource*>( self );

	// Getter
	size_t vectorLength = pThis->m_excludeMeshes.size();
	PyObject* l = PyList_New( vectorLength );
	for( size_t i = 0; i < vectorLength; ++i )
	{
		std::string& stringItem = pThis->m_excludeMeshes[i];
		PyList_SetItem( l, i, PyString_FromStringAndSize( stringItem.c_str(), stringItem.size() ) );
	}
	return l;
}

// ------------------------------------------------------------------------------------------------------
static PyObject* PySetOverrideMaterial0Meshes( PyObject* self, PyObject* args )
{
	Tr2SkinnedModelBuilderSource* pThis = BluePythonCast<Tr2SkinnedModelBuilderSource*>( self );
	
	PyObject* v = PyTuple_GetItem( args, 0 );
	// Setter
	pThis->m_overrideMaterial0Meshes.clear();
	if( !PyList_Check( v ) )
	{
		PyErr_SetString( PyExc_TypeError, "Argument must be a list of strings" );
		return NULL;
	}
	for( Py_ssize_t i = 0; i < PyList_GET_SIZE(v); ++i )
	{
		PyObject* item = PyList_GetItem( v, i );
		if( !PyString_Check( item ) )
		{
			PyErr_SetString( PyExc_TypeError, "Argument must be a list of strings" );
			return NULL;
		}

		pThis->m_overrideMaterial0Meshes.push_back( std::string( PyString_AsString( item ), PyString_Size( item ) ) );
	}
	Py_RETURN_NONE;
}

// ------------------------------------------------------------------------------------------------------
static PyObject* PyGetOverrideMaterial0Meshes( PyObject* self, PyObject* args )
{
	Tr2SkinnedModelBuilderSource* pThis = BluePythonCast<Tr2SkinnedModelBuilderSource*>( self );

	// Getter
	size_t vectorLength = pThis->m_overrideMaterial0Meshes.size();
	PyObject* l = PyList_New( vectorLength );
	for( size_t i = 0; i < vectorLength; ++i )
	{
		std::string& stringItem = pThis->m_overrideMaterial0Meshes[i];
		PyList_SetItem( l, i, PyString_FromStringAndSize( stringItem.c_str(), stringItem.size() ) );
	}
	return l;
}

// ------------------------------------------------------------------------------------------------------
static PyObject* PySetOverrideMaterial1Meshes( PyObject* self, PyObject* args )
{
	Tr2SkinnedModelBuilderSource* pThis = BluePythonCast<Tr2SkinnedModelBuilderSource*>( self );
	
	PyObject* v = PyTuple_GetItem( args, 0 );
	// Setter
	pThis->m_overrideMaterial1Meshes.clear();
	if( !PyList_Check( v ) )
	{
		PyErr_SetString( PyExc_TypeError, "Argument must be a list of strings" );
		return NULL;
	}
	for( Py_ssize_t i = 0; i < PyList_GET_SIZE(v); ++i )
	{
		PyObject* item = PyList_GetItem( v, i );
		if( !PyString_Check( item ) )
		{
			PyErr_SetString( PyExc_TypeError, "Argument must be a list of strings" );
			return NULL;
		}

		pThis->m_overrideMaterial1Meshes.push_back( std::string( PyString_AsString( item ), PyString_Size( item ) ) );
	}
	Py_RETURN_NONE;
}

// ------------------------------------------------------------------------------------------------------
static PyObject* PyGetOverrideMaterial1Meshes( PyObject* self, PyObject* args )
{
	Tr2SkinnedModelBuilderSource* pThis = BluePythonCast<Tr2SkinnedModelBuilderSource*>( self );

	// Getter
	size_t vectorLength = pThis->m_overrideMaterial1Meshes.size();
	PyObject* l = PyList_New( vectorLength );
	for( size_t i = 0; i < vectorLength; ++i )
	{
		std::string& stringItem = pThis->m_overrideMaterial1Meshes[i];
		PyList_SetItem( l, i, PyString_FromStringAndSize( stringItem.c_str(), stringItem.size() ) );
	}
	return l;
}

#endif




BLUE_DEFINE( Tr2SkinnedModelBuilderSource );

const Be::ClassInfo* Tr2SkinnedModelBuilderSource::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2SkinnedModelBuilderSource, "" )
        MAP_INTERFACE( Tr2SkinnedModelBuilderSource )

		MAP_ATTRIBUTE( "moduleResPath", m_moduleResPath, "", Be::READWRITE )
		MAP_ATTRIBUTE( "overrideMaterial0", m_overrideMaterial0, "", Be::READWRITE )
		MAP_ATTRIBUTE( "overrideMaterial1", m_overrideMaterial1, "", Be::READWRITE )
		MAP_ATTRIBUTE( "upperLeftTexCoord", m_upperLeftTexCoord, "", Be::READWRITE )
		MAP_ATTRIBUTE( "lowerRightTexCoord", m_lowerRightTexCoord, "", Be::READWRITE )
		MAP_ATTRIBUTE( "enableCutMask", m_enableCutMask, "", Be::READWRITE )

		MAP_METHOD( "SetExcludeMeshes", PySetExcludeMeshes, "Gets a list of parameters" )
		MAP_METHOD( "GetExcludeMeshes", PyGetExcludeMeshes, "Sets a list of parameters" )
		MAP_METHOD( "SetOverrideMaterial0Meshes", PySetOverrideMaterial0Meshes, "Gets a list of parameters" )
		MAP_METHOD( "GetOverrideMaterial0Meshes", PyGetOverrideMaterial0Meshes, "Sets a list of parameters" )
		MAP_METHOD( "SetOverrideMaterial1Meshes", PySetOverrideMaterial1Meshes, "Gets a list of parameters" )
		MAP_METHOD( "GetOverrideMaterial1Meshes", PyGetOverrideMaterial1Meshes, "Sets a list of parameters" )

		MAP_ATTRIBUTE( "visualModelMeshName", m_visualModelMeshName, "", Be::READWRITE )
		MAP_ATTRIBUTE( "visualModelMeshGrannyPath", m_visualModelMeshGrannyPath, "", Be::READWRITE )

    EXPOSURE_END()
}


BLUE_DEFINE( Tr2SkinnedModelBuilder );

const Be::ClassInfo* Tr2SkinnedModelBuilder::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2SkinnedModelBuilder, "" )
        MAP_INTERFACE( Tr2SkinnedModelBuilder )

		MAP_ATTRIBUTE( "sourceMeshesInfo", m_sourceMeshesInfo, "list of all source meshes for assembly", Be::READWRITE )
		MAP_ATTRIBUTE( "blendshapeInfo", m_blendshapeInfo, "list of all blendshapes for assembly", Be::READWRITE )
		MAP_ATTRIBUTE( "outputName", m_outputName, "filename of resulting granny", Be::READWRITE )
		MAP_ATTRIBUTE( "weldThreshold", m_weldThreshold, "", Be::READWRITE )
		MAP_ATTRIBUTE( "createGPUMesh", m_createGPUMesh, "should the mesh be GPU-skinned (fast but bone-limit) or CPU-skinned (slow but always works)", Be::READWRITE )
		MAP_ATTRIBUTE( "removeReversed", m_removeReversed, "should areas that have 'reversed' set be removed? Default = False", Be::READWRITE )
		MAP_ATTRIBUTE( "collapseToOpaque", m_collapseToOpaque, "should all destination areas be of type opaque? Default = False", Be::READWRITE )
		MAP_ATTRIBUTE( "collapseTransparentAreas", m_collapseTransparentAreas, "should transparentAreas also be collapsed? Default = False", Be::READWRITE )
		MAP_ATTRIBUTE( "collapseFromDepthNormal", m_collapseFromDepthNormal, "if true, use depthNormal batches as the source areas instead of the usual setup opaque/decal/transparent", Be::READWRITE )
		MAP_ATTRIBUTE( "effectPath", m_effectPath, "path to the effect to apply on the resulting skinned model", Be::READWRITE )

		//MAP_ATTRIBUTE( "forcedVertexSize"    , m_forcedVertexSize,     "if not zero, force vertices to be of this size", Be::READWRITE )
		MAP_ATTRIBUTE( "enableVertexChopping", m_enableVertexChopping, "if true, chop off any data on vertices that are too big", Be::READWRITE )
		MAP_ATTRIBUTE( "enableVertexPadding" , m_enableVertexPadding,  "if true, zero-pad vertices that are too small", Be::READWRITE )
		MAP_ATTRIBUTE( "enableSubsetBuilding", m_enableSubsetBuilding, "if true, process sourcedata in chunks to stay below the bone count. Call Build multiple times", Be::READWRITE )

		MAP_ATTRIBUTE( "sourceSkinnedModel", m_sourceSkinnedModel, "if set, get meshes and their areas from this already set up model. Note, needs granny paths to be set up", Be::READWRITE )

//		MAP_ATTRIBUTE( "vertexSize"    , m_vertexSize,     "vertex size of the output mesh; equal to size in first valid mesh + one float for a material index", Be::READ )

		MAP_METHOD_AND_WRAP( "Build", Build, "Builds the final avatar. Assumes resources have finished loading." )
#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP( "BuildAsync", BuildAsync, "Builds the final avatar on a background thread. Assumes resources have finished loading." )
#endif
		MAP_METHOD_AND_WRAP( "PrepareForBuild", PrepareForBuild, "Loads the modules and initiates loading of resources used by them." )
		MAP_METHOD_AND_WRAP( "GetSkinnedModel", GetSkinnedModel, "Returns the resulting skinned model from Build." )

		MAP_METHOD_AND_WRAP( "SetAdjustPathMethod", SetAdjustPathMethod, 
			"Sets the method to invoke to convert a raw .gr2 path to one adjusted for lod."
			"\n"
			"\nArguments:"
			"\nAdjustPathMethod - a method which has the following signature:"
			"\n           Arguments:"
			"\n           path - string, the original path to the gr2"
			"\n           Returns:"
			"\n			  a string, which is path adjusted for lod"
			)

		MAP_METHOD( "GetCollapsedInfo", PyGetCollapsedInfo, 
			"Return a list with information about all the source areas that went into"
			"\ncollapsing the mesh in the last Build() call. Each item in the list is"
			"\na tuple, containing..."
			"\n\t sourceIndex - index of the file"
			"\n\t batchType - batch type of the area"
			"\n\t areaIndex - index of the area within the specific batch type"
			"\n\t permuteIndex - permuteIndex of low level shader of the source item, if any"
			)

		MAP_METHOD( "SetExtraArrayOf", PySetExtraArrayOf, 
			"Sets a list of strings, each string will trigger the creation of"
			"\na TriFloatArrayParameter with that name."
			)

    EXPOSURE_END()
}

