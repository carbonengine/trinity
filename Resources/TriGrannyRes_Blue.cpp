#include "StdAfx.h"
#include "TriGrannyRes.h"
#include "TriGeometryRes.h"

BLUE_DEFINE( TriGrannyRes );

#if BLUE_WITH_PYTHON
static bool CheckGrannyFile( TriGrannyRes *grannyRes )
{
	if( grannyRes->GetGrannyFile() == NULL )
	{
		PyErr_SetString( PyExc_AssertionError, "Tried to get file info on invalid granny file" );
		return false;
	}

	return true;
}


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
			"  'mesh' is either the name or the index of the mesh.\n"
			"  'weights' is a list with weights as floating point numbers.\n"
			"  'geom' is a TriGeometryRes object that receives the resulting vertex buffer. Note that it should have been created with a call to x.CreateGeometryRes().\n"
			"Bake a blendshape with the given weights, using the resulting vertex buffer to replace the vertex buffer of the given TriGeometryRes."
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
			"  'ix' is the index of the model\n"
			"Gets the name of the model with index 'ix'\n"
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
			"Gets the count of areas within a specified meshID in this Granny file."
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshName", GetMeshName, 
			"Gets the name of the mesh with the given index."
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshMorphCount", GetMeshMorphCount, 
			"n = x.GetMeshMorphCount( mesh )\n"
			"  'mesh' is either the name or the index of the mesh.\n"
			"Gets the count of morph targets available for the given mesh."
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetMeshMorphName", GetMeshMorphName, 
			"s = x.GetMeshMorphName( mesh, ix )\n"
			"  'mesh' is either the name or the index of the mesh.\n"
			"  'ix' is the index of the morph target.\n"
			"Gets the name of given morph target index for the given mesh."
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetAllMeshMorphNamesNoDigits",
			GetAllMeshMorphNamesNoDigits,
			"Returns a list containing the names of all the morph targets,\n"
			"all converted to lowercase and stripped of any digits.  Specialized\n"
			"method to get answers in a format that python wants in a single call."
		)
		MAP_PROPERTY_READONLY
		(
			"animationCount", GetAnimationCount,
			"Gets the count of animation in the Granny file"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetAnimationName", GetAnimationName, 
			"s = x.GetAnimationName( ix )\n"
			"  'ix' is the index of the animation\n"
			"Gets the name of the animation with index 'ix'\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetAnimationDuration", GetAnimationDuration, 
			"d = x.GetAnimationName( ix )\n"
			"  'ix' is the index of the animation\n"
			"Gets the duration of the animation with index 'ix'\n"
		)
		MAP_METHOD_AND_WRAP( 
			"GetMeshSurfaceArea", 
			GetMeshSurfaceArea,
			"Gets the surface area for a particular meshID"
			"\n"
			"\nArguments:"
			"\nmeshID - the mesh to calculate the surface area for"
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
			"Gets material data for the given area"
		)
		MAP_METHOD_AND_WRAP
		(
			"GetMaterialDictionaryStringsForAllAreas", 
			GetMaterialDictionaryStringsForAllAreas,
			"returns all strings for the model"
		)
#endif
		MAP_METHOD_AND_WRAP
		(
			"SaveToGr2", SaveToGr2, 
			"Save the granny data to a file"
			"\n"
			"\nArguments:"
			"\nfilename - the output file to write to"
		)
		MAP_METHOD_AND_WRAP( "GetTrackGroupCount", GetTrackGroupCount, "Get the number of track groups" )
		MAP_METHOD_AND_WRAP( "GetTrackGroupName", GetTrackGroupName, "( index )\nGet the name of the track group at index" )
		MAP_METHOD_AND_WRAP( "GetTransformTrackCount", GetTransformTrackCount, "( index )\nGet the number of transform tracks under a track group" )
		MAP_METHOD_AND_WRAP( "GetTransformTrackName", GetTransformTrackName, "( groupIdx, trackIdx )\nGet the name of the transform track" )
		MAP_METHOD_AND_WRAP( "GetVectorTrackCount", GetVectorTrackCount, "( index )\nGet the number of transform tracks under a track group" )
		MAP_METHOD_AND_WRAP( "GetVectorTrackName", GetVectorTrackName, "( groupIdx, trackIdx )\nGet the name of the transform track" )
		MAP_METHOD_AND_WRAP( 
			"GetEventTrackCount", 
			GetEventTrackCount, 
			"Get the number of event (text) tracks under a track group\n"
			"Arguments:\n"
			"index - track group index" )
		MAP_METHOD_AND_WRAP( 
			"GetEventTrackName", 
			GetEventTrackName, 
			"Get the name of the event (text) track\n"
			"Arguments:\n"
			"groupIdx - track group index\n"
			"trackIdx - tack index" )
    EXPOSURE_CHAINTO( BlueAsyncRes )
}
