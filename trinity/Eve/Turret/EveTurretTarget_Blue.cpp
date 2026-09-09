// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "EveTurretTarget.h"

BLUE_DEFINE( EveTurretTarget );

const Be::ClassInfo* EveTurretTarget::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveTurretTarget, "" )
		MAP_INTERFACE( EveTurretTarget )

		MAP_ATTRIBUTE( "locator", m_locator, "Target locator ID", Be::READ )
		MAP_ATTRIBUTE( "position", m_trackingPosition, "Position the turret is tracking", Be::READ )
		MAP_ATTRIBUTE( "targetPosition", m_targetPosition, "Destination position the turret is tracking to/aiming at", Be::READ )
		MAP_ATTRIBUTE( "positionOld", m_positionOld, "Previous position the turret was tracking", Be::READ )
		MAP_ATTRIBUTE( "positionOldInfluence", m_positionOldInfluence, "Influence of previous tracking position", Be::READ )
		MAP_ATTRIBUTE( "behaviour", m_impactBehaviour, "Impact behaviour", Be::READ )

		MAP_METHOD_AND_WRAP(
			"SetShotMissed",
			SetShotMissed,
			"Queue whether the next shot misses. godma pushes one entry per damage message, the turret pops one per shot.\n"
			":param missed: is the next shot a miss" )

		MAP_METHOD_AND_WRAP(
			"GetLastShotTime",
			GetLastShotTime,
			"Get the time we last queued a shot in arbitrary units. Only use for comparison between turrets." )

		MAP_METHOD_AND_WRAP(
			"GetShotTimeVariance",
			GetShotTimeVariance,
			"Get maximum firing time variance between turrets." )

		MAP_METHOD_AND_WRAP(
			"MissQueueSize",
			MissQueueSize,
			"Get the size of the active miss/hit queue." )

	EXPOSURE_END()
}