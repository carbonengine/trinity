// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2AnimationRetargeter.h"
#include "Tr2GrannyAnimation.h"

BLUE_DEFINE( Tr2AnimationRetargeter );

const Be::ClassInfo* Tr2AnimationRetargeter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2AnimationRetargeter, "Plays clips authored for another skeleton on top of what a Tr2GrannyAnimation plays, pairing bones by name" )
		MAP_INTERFACE( Tr2AnimationRetargeter )

		MAP_PROPERTY(
			"animation",
			GetAnimation,
			SetAnimation,
			"The Tr2GrannyAnimation whose pose is modified.  Setting it takes the animation's single pose modifier slot.\n"
			"The animation does not keep the retargeter alive, so hold a reference to it for as long as it should play." )

		MAP_PROPERTY( "sourceBindPosePath", GetSourceBindPosePath, SetSourceBindPosePath, "CMF file whose skeleton holds the source rig's bind pose; empty uses the first clip's skeleton, for clips whose rest pose is the bind pose" )
		MAP_PROPERTY( "targetBindPosePath", GetTargetBindPosePath, SetTargetBindPosePath, "CMF file whose skeleton holds the target rig's bind pose" )

		MAP_PROPERTY_READONLY( "isReady", IsReady, "True once every clip and the bind pose files have loaded" )

		MAP_METHOD_AND_WRAP(
			"AddClip",
			AddClip,
			"AddClip( name, path, looping )\n\n"
			"Adds a clip of the source rig, or replaces the one with that name.  It plays once given a weight.\n"
			":param name: name to drive the clip by\n"
			":param path: CMF file holding the clip; its first animation is played\n"
			":param looping: the phase wraps round; otherwise it stops at the end of the clip" )

		MAP_METHOD_AND_WRAP( "ClearClips", ClearClips, "Removes all clips" )

		MAP_METHOD_AND_WRAP(
			"SetClipWeight",
			SetClipWeight,
			"SetClipWeight( name, weight )\n\n"
			"Sets how much a clip counts, 0 to 1.  The clips with weight are blended by their weights, and their total\n"
			"weight, up to 1, is how much of them replaces the sampled pose.  Returns False if there is no such clip.\n"
			":param name: clip name\n"
			":param weight: clip weight" )

		MAP_METHOD_AND_WRAP(
			"SetClipPhase",
			SetClipPhase,
			"SetClipPhase( name, phase )\n\n"
			"Sets where a clip is, 0 at its start and 1 at its end; set it every frame to drive the clip, from movement\n"
			"for example.  Returns False if there is no such clip.\n"
			":param name: clip name\n"
			":param phase: position in the clip" )

		MAP_METHOD_AND_WRAP(
			"MapBone",
			MapBone,
			"Pairs a bone of the source skeleton with a bone of the target skeleton; the target bone takes the source's world direction.\n"
			"Each mapped bone's first mapped child, in the order bones are mapped, sets the direction used to line up the bind poses.\n"
			":param sourceBone: bone name in the clips' skeleton\n"
			":param targetBone: bone name in the animation's skeleton" )

		MAP_METHOD_AND_WRAP(
			"MapBoneFromBind",
			MapBoneFromBind,
			"Pairs two bones that both rigs hold the same way in their bind poses (feet standing flat): the target bone turns\n"
			"from its own bind pose as the source turns from its, so each rig keeps its own shape.\n"
			":param sourceBone: bone name in the clips' skeleton\n"
			":param targetBone: bone name in the animation's skeleton" )

		MAP_METHOD_AND_WRAP( "ClearBoneMap", ClearBoneMap, "Removes all bone pairs" )

	EXPOSURE_END()
}
