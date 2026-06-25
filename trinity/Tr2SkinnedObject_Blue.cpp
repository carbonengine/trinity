// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2SkinnedObject.h"

BLUE_DEFINE_ABSTRACT( Tr2SkinnedObject );


const Be::ClassInfo* Tr2SkinnedObject::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SkinnedObject, "" )
		MAP_INTERFACE( Tr2SkinnedObject )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IWorldPosition )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "name", m_name, "Name of this object", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Whether or not to display the object", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"skinningMatrixCount",
			m_skinningMatrixCount,
			"Size of skeleton used for skinning",
			Be::READ )
		MAP_ATTRIBUTE(
			"renderRigBoneCount",
			m_numRenderRigBones,
			"Size of skeleton in the visual model",
			Be::READ )

		MAP_ATTRIBUTE(
			"curveSets",
			m_curveSets,
			"Curvesets for animating things",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "animationUpdater", m_animationUpdater, "Object that implements ITr2AnimationUpdater interface that provides bone transforms and bone names", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransformUpdater", m_worldTransformUpdater, "Object that implements ITr2WorldTransformUpdater to update this objects transform for rendering", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "visualModel", m_visualModel, "Model used to render object representation", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "transform", m_transform, "Model 4x4 transform matrix", Be::READ | Be::PERSIST )
		MAP_PROPERTY( "translation", GetPosition, SetPosition, "Position of the model" )
		MAP_PROPERTY( "rotation", GetRotation, SetRotation, "Rotation of the model" )
		MAP_PROPERTY( "scaling", GetScaling, SetScaling, "Scale of the model" )

		// lod
		MAP_ATTRIBUTE( "estimatedPixelDiameter", m_estimatedPixelDiameter, "value for LOD selection", Be::READ )
		MAP_PROPERTY_READONLY( "currentLod", GetCurrentLod, "the current LOD" )
		MAP_ATTRIBUTE( "lowDetailModel", m_lod.m_lowDetailProxy, "Proxy for a skinned model used for rendering in low detail.", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "mediumDetailModel", m_lod.m_mediumDetailProxy, "Proxy for a skinned model used for rendering in medium detail.", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "highDetailModel", m_lod.m_highDetailProxy, "Proxy for a skinned model used for rendering in high detail.", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"useDynamicBounds",
			m_useDynamicBounds,
			"Use bounds from animated bones rather than bouds of meshes in binding pose.",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"useExplicitBounds",
			m_useExplicitBounds,
			"Use explicitely specified bounding box or the one coming from visualModel.",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"explicitMinBounds",
			m_minBounds,
			"Explicitely set min bounds of an object in local space (only used when useExplicitBounds is True).",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"explicitMaxBounds",
			m_maxBounds,
			"Explicitely set max bounds of an object in local space (only used when useExplicitBounds is True).",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"frameDelay",
			m_skinningMatrixFrameDelay,
			"Cloth simulation can introduce a frame delay between animation update and\n"
			"rendering. Changing the 'parallelPhysxMeshSkinning' and 'parallelMeshMeshSkinning'\n"
			"flags affects the frame delay.",
			Be::READ )

		MAP_METHOD_AND_WRAP( "GetBoundingBoxInLocalSpace",
							 GetBoundingBoxInLocalSpace,
							 "Gets the bounding box in local space" )

		MAP_METHOD_AND_WRAP( "ResetAnimationBindings",
							 ResetAnimationBindings,
							 "ResetAnimationBindings()\n"
							 "Reset the animation bindings to update binding to meshes on the object." )

		MAP_METHOD_AND_WRAP( "PrintAllBones", PrintAllBones, "send a list of all bones in the current animation skeleton and render rig to LOGRELEASE" )

		MAP_METHOD_AND_WRAP(
			"GetBoneIndex",
			GetBoneIndex,
			"returns the joint index in the anim rig of this bone\n"
			":param name: bone name" )
		MAP_METHOD_AND_WRAP( "GetSkeletonTag", GetSkeletonTag, "returns a counter that goes up every time the skeleton rig got invalidated" )
		MAP_METHOD_AND_WRAP(
			"GetBonePosition",
			GetBonePosition,
			"returns the position of this bone in world space\n"
			":param idx: bone index" )

		MAP_ATTRIBUTE( "updatePeriod", m_updatePeriod, "How much time should elapse before the animation is updated?", Be::READWRITE )

	EXPOSURE_END()
}
