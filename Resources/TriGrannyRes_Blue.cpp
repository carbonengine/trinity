#include "StdAfx.h"
#include "TriGrannyRes.h"
#include "TriGeometryRes.h"

BLUE_DEFINE( TriGrannyRes );

#if BLUE_WITH_PYTHON

PyObject * TriGrannyRes:: GetMaterialDictionaryForArea( int mesh, int area )
{
	unsigned int key;

	key = (mesh << 16) | area;

	GrannyMaterialWrapper * mat = m_meshAreaMaterials[key];

	if (mat==0)
	{
		PyObject*emptyDict = PyDict_New();
		return(emptyDict); // return empty list.
	}

	return mat->m_dictionary ;
}

PyObject * TriGrannyRes:: GetMaterialDictionaryStringsForAllAreas()
{
	if (m_allMaterialStringsDictionary == 0)
	{
		PyObject*emptyDict = PyDict_New();
		return(emptyDict); // return empty list.
	}

	return  m_allMaterialStringsDictionary ;
}
#endif



const Be::ClassInfo* TriGrannyRes::ExposeToBlue()
{
    EXPOSURE_BEGIN( TriGrannyRes, "" )
        MAP_INTERFACE( TriGrannyRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_ICACHEABLE_METHODS()

		MAP_METHOD_AND_WRAP
		(
			"CreateGeometryRes", 
			CreateGeometryRes,
			"y = x.CreateGeometryRes()\n"
			"Create a TriGeometryRes from this resource. Useful for baking blendshapes, for example."
		)

		MAP_METHOD_AND_WRAP
		( 
			"BakeBlendshape", 
			BakeBlendshapeFromScript,
			"x.BakeBlendshape( mesh, weights, geom )\n"
			"Bake a blendshape with the given weights, using the resulting vertex buffer to replace the vertex buffer of the given TriGeometryRes.\n"
			":param mesh: index of the mesh.\n"
			":param weights: a list with weights as floating point numbers.\n"
			":param geom: a TriGeometryRes object that receives the resulting vertex buffer. Note that it should have been created with a call to x.CreateGeometryRes().\n"
		)
		MAP_PROPERTY_READONLY
		(
			"modelCount", GetModelCount,
			"Gets the count of models in the Granny file"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetModelName", GetModelName, 
			"s = x.GetModelName( ix )\n"
			"Gets the name of the model with index 'ix'\n"
			":param ix: index of the model\n"
		)
		MAP_PROPERTY_READONLY
		(
			"meshCount", GetMeshCount,
			"Gets the count of meshes in this Granny file."
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshCount", GetMeshCount, 
			"Gets the count of meshes in this Granny file."
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshAreaCount", GetMeshAreaCount, 
			"Gets the count of areas within a specified meshID in this Granny file.\n"
			":param meshIdx: mesh index"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshName", GetMeshName, 
			"Gets the name of the mesh with the given index.\n"
			":param meshIdx: mesh index"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshMorphCount", GetMeshMorphCount, 
			"Gets the count of morph targets available for the given mesh.\n"
			":param meshIdx: mesh index"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshMorphName", GetMeshMorphName, 
			"Gets the name of given morph target index for the given mesh.\n"
			":param meshIdx: mesh index\n"
			":param morphIdx: morph index"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetAllMeshMorphNamesNoDigits",
			GetAllMeshMorphNamesNoDigits,
			"Returns a list containing the names of all the morph targets,\n"
			"all converted to lowercase and stripped of any digits.  Specialized\n"
			"method to get answers in a format that python wants in a single call.\n"
			":param meshIdx: mesh index\n"
		)
		MAP_PROPERTY_READONLY
		(
			"animationCount", GetAnimationCount,
			"Gets the count of animation in the Granny file"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetAnimationName", GetAnimationName, 
			"Gets the name of the animation with index 'ix'\n"
			":param ix: index of the animation\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetAnimationDuration", GetAnimationDuration, 
			"Gets the duration of the animation with index 'ix'\n"
			":param ix: index of the animation\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"CollectGrannyMaterials", 
			CollectGrannyMaterials,
			"Creates helper material data for the export process"
		)
#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP
		(
			"GetMaterialDictionaryForArea", 
			GetMaterialDictionaryForArea,
			"Gets material data for the given area\n"
			":param meshIdx: mesh index\n"
			":param areaIdx: area index\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"GetMaterialDictionaryStringsForAllAreas", 
			GetMaterialDictionaryStringsForAllAreas,
			"returns all strings for the model"
		)
#endif
		MAP_METHOD_AND_WRAP( "GetTrackGroupCount", GetTrackGroupCount, "Get the number of track groups" )
		MAP_METHOD_AND_WRAP( 
			"GetTrackGroupName", 
			GetTrackGroupName, 
			"Get the name of the track group at index\n" 
			":param groupIdx: track group index"
		)
		MAP_METHOD_AND_WRAP( 
			"GetTransformTrackCount", 
			GetTransformTrackCount, 
			"Get the number of transform tracks under a track group\n"
			":param groupIdx: track group index"
		)
		MAP_METHOD_AND_WRAP( 
			"GetTransformTrackName", 
			GetTransformTrackName, 
			"Get the name of the transform track\n"
			":param groupIdx: track group index\n"
			":param trackIdx: track index\n"
		)
		MAP_METHOD_AND_WRAP( 
			"GetVectorTrackCount", 
			GetVectorTrackCount, 
			"Get the number of transform tracks under a track group\n" 
			":param groupIdx: track group index\n"
		)
		MAP_METHOD_AND_WRAP( 
			"GetVectorTrackName", 
			GetVectorTrackName, 
			"Get the name of the transform track\n" 
			":param groupIdx: track group index\n"
			":param trackIdx: track index\n"
		)
		MAP_METHOD_AND_WRAP( 
			"GetEventTrackCount", 
			GetEventTrackCount, 
			"Get the number of event (text) tracks under a track group\n"
			":param groupIdx: track group index\n"
		)
		MAP_METHOD_AND_WRAP( 
			"GetEventTrackName", 
			GetEventTrackName, 
			"Get the name of the event (text) track\n"
			":param groupIdx: track group index\n"
			":param trackIdx: track index\n"
		)
    EXPOSURE_CHAINTO( BlueAsyncRes )
}
