#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorStatic.h"

BLUE_DEFINE( Tr2InteriorStatic );

// ---------------------------------------------------------------------------------------
// Description:
//   Python helper function for extracting the local-space bounding box of the static.
// ---------------------------------------------------------------------------------------
#if BLUE_WITH_PYTHON
static PyObject* PyGetBoundingBoxInLocalSpace( PyObject* self, PyObject* args )
{
	Tr2InteriorStatic* pThis = BluePythonCast<Tr2InteriorStatic*>( self );
	if (!PyArg_ParseTuple(args, ""))
	{
		return NULL;
	}

	Vector3 min( 0.0f, 0.0f, 0.0f );
	Vector3 max( 0.0f, 0.0f, 0.0f );

	if( pThis->GetBoundingBox( min, max ) )
	{
		PyObject* ret = Py_BuildValue( "(fff)(fff)", min.x, min.y, min.z, max.x, max.y, max.z );
		return ret;
	}

	return NULL;
}

// ---------------------------------------------------------------------------------------
// Description:
//   Python helper function for extracting the world-space bounding box of the static.
// ---------------------------------------------------------------------------------------
static PyObject* PyGetBoundingBoxInWorldSpace( PyObject* self, PyObject* args )
{
	Tr2InteriorStatic* pThis = BluePythonCast<Tr2InteriorStatic*>( self );
	if (!PyArg_ParseTuple(args, ""))
	{
		return NULL;
	}

	Vector3 min( 0.0f, 0.0f, 0.0f );
	Vector3 max( 0.0f, 0.0f, 0.0f );

	if( pThis->GetWorldBoundingBox( min, max ) )
	{
		PyObject* ret = Py_BuildValue( "(fff)(fff)", min.x, min.y, min.z, max.x, max.y, max.z );
		return ret;
	}

	return NULL;
}
#endif

const Be::ClassInfo* Tr2InteriorStatic::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2InteriorStatic, "" )
		MAP_INTERFACE( Tr2InteriorStatic )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2Pickable )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "The name of this interior static object", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Boolean flag indicating whether or not to render this static object", Be::READWRITE )
		MAP_ATTRIBUTE( "displayTargetMesh", m_displayTargetMesh, "Boolean flag indicating whether or not to display the target mesh", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "displayDetailMeshes", m_displayDetailMeshes, "Boolean flag indicating whether or not to render the detail meshes", Be::READWRITE | Be::PERSIST)
		MAP_ATTRIBUTE( "enlightenAreas", m_enlightenAreas, "A vector containing the Enlighten areas (mesh chunks that can have their own albedo and/or emissive color)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "detailMeshes", m_detailMeshes, "A vector containing the Enlighten detail meshes", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "geometryResPath", m_geometryResPath, "Path to the geometry resource (usually a .gr2 file)", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "occlusionResPath", m_occlusionResPath, "Path to the occlusion geometry resource (usually a .gr2 file)", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "geometry", m_geometryResource, "The geometry resource which provides this static with its mesh data", Be::READ )
		
		MAP_ATTRIBUTE( "worldPosition", m_position, "Vector specifying the position of the static in the cell space", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rotation", m_rotation, "Quaternion specifying the rotation of the static in the world", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_PROPERTY_READONLY( "worldTransform", GetWorldTransform, "Matrix specifying the position and rotation of the static in the world" )
		MAP_PROPERTY_READONLY( "parentTransform", GetParentTransform, "Matrix specifying the position and rotation of the parent cell in the world" )
		MAP_ATTRIBUTE( "instanceInSystemIdx", m_instanceInSystemIdx, "Index for assigning Enlighten material ids", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "uvTranslation", m_uvTranslation, "Translation of the light-map UVs into the global system light map (Enlighten)", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "uvLinearTransform", m_uvLinearTransform, "Rotation and scaling of the light-map UVs into the global system light map (Enlighten)", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "visibleLightCount", m_visibleLightCount, "The number of visible lights affecting the static, taking into account the current view", Be::READ )
		MAP_ATTRIBUTE( "totalLightCount", m_totalLightCount, "The total number of lights affecting the static, independent of the current view", Be::READ )
		MAP_ATTRIBUTE( "visualizeVisibleLights", m_drawLightSpider, "Draw lines to light sources affecting this static", Be::READWRITE )
		MAP_ATTRIBUTE( "useOcclusionGeo", m_useOcclusionGeometry, "Toggle occlusion geometry on or off for this static", Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curve sets to animate light attributes", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "depthOffset", m_depthOffset, "Depth offset for transparency sorting", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "SetInstanceData", SetInstanceData, 
				"\nSet the per instance data. Should only be used for data that has been loaded from database and not disk.\n" 
				"\nArguments:"
				"\nlinearTransform : (4tuple) linear transformation"
				"\ntranslation : (2tuple)the uv translation"
				"\ninstanceInSystemIdx: the system index for the static instance"
				)

		MAP_METHOD_AND_WRAP( "BindLowLevelShaders", BindLowLevelShaders, "Binds low level shaders for all meshes of the static." );

		MAP_METHOD( "GetBoundingBoxInLocalSpace", PyGetBoundingBoxInLocalSpace, "Gets the bounding box in local space" )
		MAP_METHOD( "GetBoundingBoxInWorldSpace", PyGetBoundingBoxInWorldSpace, "Gets the bounding box in world space" )

	EXPOSURE_END()
}

#endif
