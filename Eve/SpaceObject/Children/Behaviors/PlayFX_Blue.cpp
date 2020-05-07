#include "StdAfx.h"
#include "PlayFX.h"

BLUE_DEFINE( PlayFX );

const Be::ClassInfo* PlayFX::ExposeToBlue()
{
	EXPOSURE_BEGIN( PlayFX, "" )
		MAP_INTERFACE( PlayFX )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, ":jessica-group: PlayFX", Be::READWRITE )
		MAP_ATTRIBUTE( "minSec", m_minSec, "Lower value of a randomized timing for drones to play effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxSec", m_maxSec, "Higher value of a randomized timing for drones to play effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "distanceFromCenter", m_distanceFromCenter, "How much offset should be for the effect", Be::READWRITE )
		MAP_ATTRIBUTE( "firingEffect", m_firingEffect, "A stretch effect for this firing effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingEffects", m_firingEffects, "A stretch effect for this firing effect", Be::READ )
		MAP_ATTRIBUTE( "delay", m_delay, "delay for firing effect", Be::READWRITE )

	EXPOSURE_END()
}