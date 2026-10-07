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

		MAP_PROPERTY( "sourcePath", GetSourcePath, SetSourcePath, "CMF file holding the clip to play and the skeleton it was authored for; its first animation is played" )
		MAP_PROPERTY( "blendSourcePath", GetBlendSourcePath, SetBlendSourcePath, "Optional second clip of the same rig, blended over the first by blend (for example a run over a walk)" )
		MAP_PROPERTY( "sourceBindPosePath", GetSourceBindPosePath, SetSourceBindPosePath, "CMF file whose skeleton holds the source rig's bind pose; empty uses the first clip's skeleton, for clips whose rest pose is the bind pose" )
		MAP_PROPERTY( "targetBindPosePath", GetTargetBindPosePath, SetTargetBindPosePath, "CMF file whose skeleton holds the target rig's bind pose" )

		MAP_ATTRIBUTE( "weight", m_weight, "How much of the clips replaces the sampled pose, 0 to 1", Be::READWRITE )
		MAP_ATTRIBUTE( "blend", m_blend, "How much of the second clip replaces the first, 0 to 1", Be::READWRITE )
		MAP_ATTRIBUTE( "phase", m_phase, "Position in the cycle, 0 to 1, shared by both clips; set it every frame to drive the clips from movement", Be::READWRITE )
		MAP_ATTRIBUTE( "speed", m_speed, "Cycles the phase advances by itself per clip length; 0 when phase is driven from outside", Be::READWRITE )
		MAP_PROPERTY( "sourcePhaseOffset", GetSourcePhaseOffset, SetSourcePhaseOffset, "Where in the first clip phase 0 falls, 0 to 1 (for lining up the clips' steps)" )
		MAP_PROPERTY( "blendSourcePhaseOffset", GetBlendSourcePhaseOffset, SetBlendSourcePhaseOffset, "Where in the second clip phase 0 falls, 0 to 1" )

		MAP_PROPERTY_READONLY( "isReady", IsReady, "True once the clips and both bind pose files have loaded" )

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
