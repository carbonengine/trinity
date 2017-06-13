#include "StdAfx.h"
#include "Tr2GrannyAnimation.h"
#include "Resources/TriGeometryRes.h"
#include "Resources/TriGrannyRes.h"

BLUE_DEFINE( Tr2GrannyAnimation );

const Be::ClassInfo* Tr2GrannyAnimation::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2GrannyAnimation, "" )
        MAP_INTERFACE( Tr2GrannyAnimation )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2AnimationUpdater )

		MAP_PROPERTY( "resPath", GetResPath, SetResPath, "The resource path to the Granny file containing the animations to be played." )
		MAP_ATTRIBUTE( "resPath_", m_resPath, "", Be::PERSISTONLY )
		MAP_ATTRIBUTE( "grannyRes", m_grannyRes, "", Be::READ )
		MAP_PROPERTY( "model", GetModel, SetModel, "" )
		MAP_ATTRIBUTE( "model_", m_model, "", Be::PERSISTONLY )

		MAP_ATTRIBUTE
		( 
			"debugRenderSkeleton", m_debugRenderSkeleton, 
			"If set, and a debug renderer is set, then the skeleton is rendered with lines connecting\n"
			"the joints of the skeleton.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		( 
			"debugRenderJointNames", m_debugRenderJointNames, 
			"If set, and a debug renderer is set, then the names of the joints in the skeleton\n"
			"are displayed at their projected locations.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		( 
			"animationEnabled", m_animationEnabled, 
			"Enable/disable animation update",
			Be::READWRITE
		)

		MAP_METHOD_AND_WRAP
		(
			"PlayAnimation",
			PlayAnimationOnce,
			"PlayAnimation( animName )\n\nPlays the given animation, replacing whatever animation was playing before.\n"
			":param animName: animation name"
		)
		MAP_METHOD_AND_WRAP
		(
			"PlayAnimationEx",
			PlayAnimationEx,
			"PlayAnimationEx( animName, loopCount, delay, speed )\n\n"
			"Plays the given animation, replacing whatever animation was playing before.\n"
			":param animName: animation name\n"
			":param loopCount: can be 0 to loop forever.\n"
			":param delay: time (in seconds) from now before animation should start playing.\n"
			":param speed: can be used speed up or slow down playback - use negative values to play backwards.\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"ChainAnimation",
			ChainAnimation,
			"ChainAnimation( animName )\n\nPlays the given animation, starting when currently playing animation finishes.\n"
			"If it is looping then it is replaced at the end of the current loop.\n"
			":param animName: animation name\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"ChainAnimationEx",
			ChainAnimationEx,
			"ChainAnimationEx( animName, loopCount, delay, speed )\n\n"
			"Plays the given animation, starting when currently playing animation finishes.\n"
			"If it is looping then it is replaced at the end of the current loop.\n"
			":param animName: animation name\n"
			":param loopCount: can be 0 to loop forever.\n"
			":param delay: time (in seconds) from now before animation should start playing.\n"
			":param speed: can be used speed up or slow down playback - use negative values to play backwards.\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"EndAnimation",
			EndAnimation,
			"EndAnimation()\n\n"
			"Stops currently playing animation at the end of the current loop iteration."
		)
		MAP_METHOD_AND_WRAP
		(
			"ClearAnimations",
			ClearAnimations,
			"ClearAnimations()\n\n"
			"Abruptly ends all animations."
		)
		
		MAP_METHOD_AND_WRAP
		(
			"PlayLayerAnimation",
			PlayLayerAnimationByName,
			"PlayLayerAnimation( layerName, animationName, replace, loops, delay, speed, clearWhenFinished )\n\n"
			"Plays the given animation on the layer specified.\n"
			":param layerName: layer name\n"
			":param animName: animation name\n"
			":param replace: \n"
			":param loops: can be 0 to loop forever.\n"
			":param delay: time (in seconds) from now before animation should start playing.\n"
			":param speed: can be used speed up or slow down playback - use negative values to play backwards.\n"
			":param clearWhenFinished: \n"
		)
		MAP_METHOD_AND_WRAP
		(
			"AddAnimationLayer",
			AddAnimationLayer,
			"AddAnimationLayer( layerName[, layerWeight=1.0] )\n\n"
			"Creates a new animation layer for this granny animation.\n"
			":param layerName: layer name\n"
			":param layerWeight: layer weight\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"AddAnimationLayerBone",
			AddAnimationLayerBone,
			"AddAnimationLayerBone( layerName, boneName )\n\n"
			"Add the specified bone to this animation layer.\n"
			":param layerName: layer name\n"
			":param boneName: bone name\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"GetLayerWeight",
			GetLayerWeight,
			"GetLayerWeight( layerName )\n\n"
			"Returns a scalar float weight for the named layer.\n"
			":param layerName: layer name\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"SetLayerWeight",
			SetLayerWeight,
			"SetLayerWeight( layerName, layerWeight )\n\n"
			"Sets layer blend weight for an existing layer.\n"
			":param layerName: layer name\n"
			":param layerWeight: layer weight\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"RemoveAnimationLayerBone",
			RemoveAnimationLayerBone,
			"RemoveAnimationLayerBone( layerName, boneName )\n\n"
			"Remove the specified bone to from the animation layer.\n"
			":param layerName: layer name\n"
			":param boneName: bone name\n"
		)

		MAP_ATTRIBUTE( "boneOffset", m_boneOffset, "Per-bone post animation offsets.", Be::READWRITE )

		MAP_ATTRIBUTE
		(
			"eventListener",
			m_eventListener,
			"An event listener that's triggered by granny text track events.",
			Be::READWRITE
		)

		MAP_METHOD_AND_WRAP
		(
			"CreateStaticGeometry",
			CreateStaticGeometry,
			"Bakes several skinned geometries into a single static one applying current animation in the process.\n"
			"Returns a tuple (geometry, remapping) where geometry is the newly created TriGeometryRes and remapping\n"
			"is a dict (TriGeometryRes, meshIndex) -> areaStartIndex that maps input geometry res objects and their\n"
			"mesh indices to the area index in the resulting static mesh.\n"
			":param geometries: list of TriGeometryRes objects to merge"
		)
	EXPOSURE_END()
}
