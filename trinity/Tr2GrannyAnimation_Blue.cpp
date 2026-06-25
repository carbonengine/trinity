// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2GrannyAnimation.h"
#include "Resources/TriGeometryRes.h"
#include "Resources/TriGrannyRes.h"


BLUE_DEFINE_INTERFACE( ITr2GrannyAnimationOwner );


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

		MAP_ATTRIBUTE(
			"debugRenderSkeleton", m_debugRenderSkeleton, "If set, and a debug renderer is set, then the skeleton is rendered with lines connecting\n"
														  "the joints of the skeleton.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"debugRenderJointNames", m_debugRenderJointNames, "If set, and a debug renderer is set, then the names of the joints in the skeleton\n"
															  "are displayed at their projected locations.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"animationEnabled", m_animationEnabled, "Enable/disable animation update", Be::READWRITE )

		MAP_METHOD_AND_WRAP(
			"PlayAnimation",
			PlayAnimationOnce,
			"PlayAnimation( animName )\n\nPlays the given animation, replacing whatever animation was playing before.\n"
			":param animName: animation name" )
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"PlayAnimationEx",
			PlayAnimationEx,
			1,
			"PlayAnimationEx( animName, loopCount, delay, speed, clearWhenDone=True )\n\n"
			"Plays the given animation, replacing whatever animation was playing before.\n"
			":param animName: animation name\n"
			":param loopCount: can be 0 to loop forever.\n"
			":param delay: time (in seconds) from now before animation should start playing.\n"
			":param speed: can be used speed up or slow down playback - use negative values to play backwards.\n"
			":param clearWhenDone: boolean True to clear animation when done, boolean False to keep it playing. Defaults to True\n" )
		MAP_METHOD_AND_WRAP(
			"ChainAnimation",
			ChainAnimation,
			"ChainAnimation( animName )\n\nPlays the given animation, starting when currently playing animation finishes.\n"
			"If it is looping then it is replaced at the end of the current loop.\n"
			":param animName: animation name\n" )
		MAP_METHOD_AND_WRAP(
			"ChainAnimationEx",
			ChainAnimationEx,
			"ChainAnimationEx( animName, loopCount, delay, speed )\n\n"
			"Plays the given animation, starting when currently playing animation finishes.\n"
			"If it is looping then it is replaced at the end of the current loop.\n"
			":param animName: animation name\n"
			":param loopCount: can be 0 to loop forever.\n"
			":param delay: time (in seconds) from now before animation should start playing.\n"
			":param speed: can be used speed up or slow down playback - use negative values to play backwards.\n" )
		MAP_METHOD_AND_WRAP(
			"TogglePauseAnimations",
			TogglePauseAnimations,
			"TogglePauseAnimations( pause )\n\n"
			"Pauses/unpauses animation playback."
			":param pause: boolean True for pause, boolean False to unpause" )
		MAP_METHOD_AND_WRAP(
			"EndAnimation",
			EndAnimation,
			"EndAnimation()\n\n"
			"Stops currently playing animation at the end of the current loop iteration." )
		MAP_METHOD_AND_WRAP(
			"ClearAnimations",
			ClearAnimations,
			"ClearAnimations()\n\n"
			"Abruptly ends all animations." )

		MAP_METHOD_AND_WRAP(
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
			":param clearWhenFinished: \n" )
		MAP_METHOD_AND_WRAP(
			"AddAnimationLayer",
			AddAnimationLayer,
			"AddAnimationLayer( layerName[, layerWeight=1.0] )\n\n"
			"Creates a new animation layer for this granny animation.\n"
			":param layerName: layer name\n"
			":param layerWeight: layer weight\n" )
		MAP_METHOD_AND_WRAP(
			"AddAnimationLayerBone",
			AddAnimationLayerBone,
			"AddAnimationLayerBone( layerName, boneName )\n\n"
			"Add the specified bone to this animation layer.\n"
			":param layerName: layer name\n"
			":param boneName: bone name\n" )
		MAP_METHOD_AND_WRAP(
			"AddAnimationLayerAllBones",
			AddAnimationLayerAllBones,
			"AddAnimationLayerAllBones( layerName )\n\n"
			"Add all bones to this animation layer.\n"
			":param layerName: layer name\n" )
		MAP_METHOD_AND_WRAP(
			"GetLayerWeight",
			GetLayerWeight,
			"GetLayerWeight( layerName )\n\n"
			"Returns a scalar float weight for the named layer.\n"
			":param layerName: layer name\n" )
		MAP_METHOD_AND_WRAP(
			"SetLayerWeight",
			SetLayerWeight,
			"SetLayerWeight( layerName, layerWeight )\n\n"
			"Sets layer blend weight for an existing layer.\n"
			":param layerName: layer name\n"
			":param layerWeight: layer weight\n" )
		MAP_METHOD_AND_WRAP(
			"SetLayerControlParam",
			SetLayerControlParam,
			"SetLayerControlParam( layerName, controlParam )\n\n"
			"Sets control parameter for driving an animation with a parameter other than time.\n"
			"Once this is called, the layer will be driven by its control parameter and not animation time.\n"
			"The animation layer's frame will be selected by multiplying the controlParam by the duration of\n"
			"the animation, so a value of 0.0 will be the first frame and 1.0 will be the last.\n"
			":param layerName: layer name\n"
			":param controlParam: layer control parameter\n" )
		MAP_METHOD_AND_WRAP(
			"SetLayerControlParamSkewRate",
			SetLayerControlParamSkewRate,
			"SetLayerControlParam( layerName, skewRate )\n\n"
			"Sets control parameter for driving an animation with a parameter other than time.\n"
			"Once this is called, the layer will be driven by its control parameter and not animation time.\n"
			"The animation layer's frame will be selected by multiplying the controlParam by the duration of\n"
			"the animation, so a value of 0.0 will be the first frame and 1.0 will be the last.\n"
			":param layerName: layer name\n"
			":param skewRate: layer control parameter maximum rate of change (for smoothing)\n" )
		MAP_METHOD_AND_WRAP(
			"AimBone",
			AimBone,
			"AimBone( bone_name, target_x, target_y, target_z, axis_x, axis_y, axis_z )\n\n"
			"Configures IK to aim one bone's specified axis at a world-space target.\n"
			":param bone_name: name of bone to aim constrain\n"
			":param target_x: world-space target x coord\n"
			":param target_y: world-space target y coord\n"
			":param target_z: world-space target z coord\n"
			":param axis_x: align axis x coord\n"
			":param axis_y: align axis y coord\n"
			":param axis_z: align axis z coord\n" )
		MAP_METHOD_AND_WRAP(
			"DisableAimBone",
			DisableAimBone,
			"DisableAimBone()\n\n"
			"Disables bone aim constraint.\n" )
		MAP_METHOD_AND_WRAP(
			"SetAdditiveBlendMode",
			SetAdditiveBlendMode,
			"SetAdditiveBlendMode( additive )\n\n"
			"When the additive blend mode is set, blending will use addition rather than averaging to combine layer poses."
			":param additive: additive on or off\n" )
		MAP_METHOD_AND_WRAP(
			"GetAdditiveBlendMode",
			GetAdditiveBlendMode,
			"GetAdditiveBlendMode( )\n\n"
			"Returns current state of additive blending. When the additive blend mode is set, blending will use addition rather than averaging to combine layer poses." )
		MAP_METHOD_AND_WRAP(
			"RemoveAnimationLayerBone",
			RemoveAnimationLayerBone,
			"RemoveAnimationLayerBone( layerName, boneName )\n\n"
			"Remove the specified bone to from the animation layer.\n"
			":param layerName: layer name\n"
			":param boneName: bone name\n" )
		MAP_METHOD_AND_WRAP(
			"ClearAnimationLayers",
			ClearAnimationLayers,
			"Clears all animation layers from this granny animation controller\n" )

		MAP_ATTRIBUTE( "boneOffset", m_boneOffset, "Per-bone post animation offsets.", Be::READ )

		MAP_ATTRIBUTE(
			"eventListener",
			m_eventListener,
			"An event listener that's triggered by granny text track events.",
			Be::READWRITE )

		MAP_METHOD_AND_WRAP(
			"AddSecondaryResPath",
			AddSecondaryResPath,
			"AddSecondaryResPath adds another file to the list of files that are searched for animations played by name.  Joint\n"
			"names must match those in the normal res file.\n"
			":param val: Path to secondary res file." )

		MAP_METHOD_AND_WRAP(
			"GetSecondaryAnimationName",
			GetSecondaryAnimationName,
			"GetSecondaryAnimationName( resPath, index )\n\n"
			"Returns the name of the animation of the given index in the given res file.\n"
			":param resPath: Path to secondary res file.\n"
			":param index: Index of selected animation in res file." )

		MAP_METHOD_AND_WRAP( "GetAnimationNames", GetAnimationNames, "Returns all animation names" )
	EXPOSURE_END()
}
