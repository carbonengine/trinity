// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2AnimationRetargeter.h"
#include "Tr2GrannyAnimation.h"

BLUE_DEFINE( Tr2AnimationRetargeter );

const Be::ClassInfo* Tr2AnimationRetargeter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2AnimationRetargeter, "Plays a clip authored for another skeleton on top of what a Tr2GrannyAnimation plays, pairing bones by name" )
		MAP_INTERFACE( Tr2AnimationRetargeter )

		MAP_PROPERTY(
			"animation",
			GetAnimation,
			SetAnimation,
			"The Tr2GrannyAnimation whose pose is modified.  Setting it takes the animation's single pose modifier slot.\n"
			"The animation does not keep the retargeter alive, so hold a reference to it for as long as it should play." )

		MAP_PROPERTY( "sourcePath", GetSourcePath, SetSourcePath, "CMF file holding the clip to play and the skeleton it was authored for; its first animation is played" )
		MAP_PROPERTY( "sourceBindPosePath", GetSourceBindPosePath, SetSourceBindPosePath, "CMF file whose skeleton holds the source rig's bind pose" )
		MAP_PROPERTY( "targetBindPosePath", GetTargetBindPosePath, SetTargetBindPosePath, "CMF file whose skeleton holds the target rig's bind pose" )

		MAP_ATTRIBUTE( "weight", m_weight, "How much of the clip replaces the sampled pose, 0 to 1", Be::READWRITE )
		MAP_ATTRIBUTE( "speed", m_speed, "Playback rate of the clip; changes take effect without jumping", Be::READWRITE )

		MAP_PROPERTY_READONLY( "isReady", IsReady, "True once the clip and both bind pose files have loaded" )

		MAP_METHOD_AND_WRAP(
			"MapBone",
			MapBone,
			"Pairs a bone of the source skeleton with a bone of the target skeleton.\n"
			"Each mapped bone's first mapped child, in the order bones are mapped, sets the direction used to line up the bind poses.\n"
			":param sourceBone: bone name in the clip's skeleton\n"
			":param targetBone: bone name in the animation's skeleton" )

		MAP_METHOD_AND_WRAP( "ClearBoneMap", ClearBoneMap, "Removes all bone pairs" )

	EXPOSURE_END()
}
