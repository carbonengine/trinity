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

	EXPOSURE_END()
}