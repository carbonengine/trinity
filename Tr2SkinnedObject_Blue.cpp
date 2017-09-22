#include "StdAfx.h"

#include "Tr2SkinnedObject.h"

BLUE_DEFINE_ABSTRACT( Tr2SkinnedObject );

#if BLUE_WITH_PYTHON
static PyObject* PyGetClippedWorldBoundingObb( PyObject* self, PyObject* args )
{
	Tr2SkinnedObject* pThis = BluePythonCast<Tr2SkinnedObject*>( self );
	Matrix m;

	PyObject* arg0 = NULL;
	
	if( !PyArg_ParseTuple( args, "O", &arg0 ) )
	{
		PyErr_SetString(PyExc_RuntimeError, "Function expects one argument, a local to world transform.");
		return NULL;
	}

	if( !BlueExtractArgument( arg0, m, 0 ) )
	{
		PyErr_SetString(PyExc_RuntimeError, "Function expects one argument, a local to world transform.");
		return NULL;
	}

	Vector3 x( 0.0f, 0.0f, 0.0f );
	Vector3 y = x, z = x, center = x, sizes = x;

	pThis->GetClippedWorldBoundingObb( m, x, y, z, center, sizes );

	PyObject* ret = Py_BuildValue( "(fff)(fff)(fff)(fff)(fff)", 
		x.x, x.y, x.z,
		y.x, y.y, y.z,
		z.x, z.y, z.z,
		center.x, center.y, center.z,
		sizes.x, sizes.y, sizes.z );

	return ret;
}
#endif

const Be::ClassInfo* Tr2SkinnedObject::ExposeToBlue()
{
	EXPOSURE_BEGIN(Tr2SkinnedObject, "" )
		MAP_INTERFACE( Tr2SkinnedObject )
		MAP_INTERFACE( ITr2Renderable )
        MAP_INTERFACE( IWorldPosition )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "name", m_name, "Name of this object", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Whether or not to display the object", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "displayMarker", m_displayMarker, "Whether or not to display the object marker", Be::READWRITE )
		MAP_ATTRIBUTE( "displayName", m_displayName, "If set, the name is displayed in the 3D view (debug feature)", Be::READWRITE )
		MAP_PROPERTY(
			"debugSkeletonTrailLength",
			GetDebugSkeletonTrailLength,
			SetDebugSkeletonTrailLength,
			"Length of skeleton trail to capture. The skeleton trail can be rendered by"
			"\nsetting 'debugRenderSkeletonTrail' to True."
		)
		MAP_PROPERTY(
			"debugRenderSkeletonTrail",
			GetDebugRenderSkeletonTrail,
			SetDebugRenderSkeletonTrail,
			"If set, and a debug renderer is set for Trinity, the full trail of skeletons is rendered."
			"\nThis is useful for debugging frame delay issues with skinning."
		)
		MAP_ATTRIBUTE(
			"debugRenderSkeletonTrailIx",
			m_debugRenderSkeletonTrailIx,
			"Adds the frame index to the rendering of skeleton trail (see debugRenderSkeletonTrail)."
			"\nThis is useful for debugging frame delay issues with skinning.",
			Be::READWRITE
		)
		MAP_ATTRIBUTE(
			"debugRenderSkeletonJointIndices",
			m_debugRenderSkeletonJointIndices,
			"Renders skeleton joint indices",
			Be::READWRITE
		)
		MAP_ATTRIBUTE(
			"debugFreezeSkeletonTrail",
			m_debugFreezeSkeletonTrail,
			"If set, the skeleton trail is frozen."
			"\nThis is useful for debugging frame delay issues with skinning.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"skinningMatrixCount",
			m_skinningMatrixCount,
			"Size of skeleton used for skinning",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"renderRigBoneCount",
			m_numRenderRigBones,
			"Size of skeleton in the visual model",
			Be::READ
		)

		MAP_ATTRIBUTE
		( 
			"curveSets", 
			m_curveSets, 
			"Curvesets for animating things", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE( "animationUpdater", m_animationUpdater, "Object that implements ITr2AnimationUpdater interface that provides bone transforms and bone names", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransformUpdater", m_worldTransformUpdater, "Object that implements ITr2WorldTransformUpdater to update this objects transform for rendering", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "visualModel", m_visualModel, "Model used to render object representation", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "transform", m_transform, "Model 4x4 transform matrix", Be::READWRITE | Be::PERSIST )
		MAP_PROPERTY( "translation", GetPosition, SetPosition, "Position of the model" )
		MAP_PROPERTY( "rotation", GetRotation, SetRotation, "Rotation of the model" )
		MAP_PROPERTY( "scaling", GetScaling, SetScaling, "Scale of the model" )

		// lod
		MAP_ATTRIBUTE( "estimatedPixelDiameter", m_estimatedPixelDiameter, "value for LOD selection", Be::READ )
		MAP_PROPERTY_READONLY( "currentLod", GetCurrentLod, "the current LOD" )
		MAP_ATTRIBUTE( "lowDetailModel",	m_lod.m_lowDetailProxy,		"Proxy for a skinned model used for rendering in low detail." ,		Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "mediumDetailModel",	m_lod.m_mediumDetailProxy,	"Proxy for a skinned model used for rendering in medium detail.",	Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "highDetailModel",	m_lod.m_highDetailProxy,	"Proxy for a skinned model used for rendering in high detail.",		Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( 
			"useExplicitBounds",	
			m_useExplicitBounds,	
			"Use explicitely specified bounding box or the one coming from visualModel.",		
			Be::READWRITE | Be::PERSIST | Be::NOTIFY 
		)
		MAP_ATTRIBUTE( 
			"explicitMinBounds",	
			m_minBounds,	
			"Explicitely set min bounds of an object in local space (only used when useExplicitBounds is True).",		
			Be::READWRITE | Be::PERSIST | Be::NOTIFY 
		)
		MAP_ATTRIBUTE( 
			"explicitMaxBounds",	
			m_maxBounds,	
			"Explicitely set max bounds of an object in local space (only used when useExplicitBounds is True).",		
			Be::READWRITE | Be::PERSIST | Be::NOTIFY 
		)

		MAP_ATTRIBUTE
		( 
			"frameDelay", 
			m_skinningMatrixFrameDelay,
			"Cloth simulation can introduce a frame delay between animation update and\n"
			"rendering. Changing the 'parallelPhysxMeshSkinning' and 'parallelMeshMeshSkinning'\n"
			"flags affects the frame delay.",
			Be::READ
		)

		MAP_METHOD_AND_WRAP( "GetBoundingBoxInLocalSpace",
					GetBoundingBoxInLocalSpace, 
					"Gets the bounding box in local space" )

		MAP_METHOD( "GetClippedWorldBoundingObb",
					PyGetClippedWorldBoundingObb,
					"Computes an Oriented Bounding Box in world space, which has been shrunk\n"
					"to fit the current viewing frustum as tightly as possible with visibly clipping\n"
					"The frustum is derived from Tr2Renderer GetViewTransform and so on.\n"
					"The function supports explicitMaxBounds\n"
					":param localToWorld: geo2.Matrix transform that takes this skinned object from local to world coordinates\n"
					":type localToWorld: tuple[tuple[float]]\n"
					":returns:\n"
					"   x, y, z      - normalized vectors defining the OBB's orientation\n"
					"   center       - the center, in world coordinates, of the OBB\n"
					"   sizes        - half the size of the OBB along every axis x, y or z.\n"
					"                  Ie you get to a corner point with center + sizes[0] * x.\n"
					"                  Full width/height/depth is sizes*2."
					":rtype: tuple[tuple[float]]"
					)

		MAP_METHOD_AND_WRAP( "ResetAnimationBindings",           
					ResetAnimationBindings,          
					"ResetAnimationBindings()\n"
					"Reset the animation bindings to update binding to meshes on the object." )

		MAP_METHOD_AND_WRAP( "PrintAllBones"   , PrintAllBones   , "send a list of all bones in the current animation skeleton and render rig to LOGRELEASE" )

		MAP_METHOD_AND_WRAP( 
			"GetBoneIndex"    , 
			GetBoneIndex    , 
			"returns the joint index in the anim rig of this bone\n" 
			":param name: bone name"
		)
		MAP_METHOD_AND_WRAP( "GetSkeletonTag"  , GetSkeletonTag  , "returns a counter that goes up every time the skeleton rig got invalidated" )
		MAP_METHOD_AND_WRAP( 
			"GetBonePosition" , 
			GetBonePosition , 
			"returns the position of this bone in world space\n"
			":param idx: bone index"
		)

		MAP_ATTRIBUTE( "updatePeriod", m_updatePeriod, "How much time should elapse before the animation is updated?", Be::READWRITE )

	EXPOSURE_END()
}
