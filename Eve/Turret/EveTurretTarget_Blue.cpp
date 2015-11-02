////////////////////////////////////////////////////////////
//
//    Created:   November 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveTurretTarget.h"

BLUE_DEFINE( EveTurretTarget );

const Be::ClassInfo* EveTurretTarget::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveTurretTarget, "" )
        MAP_INTERFACE( EveTurretTarget )

		MAP_ATTRIBUTE( "locator", m_locator, "Target locator ID", Be::READ )
		MAP_ATTRIBUTE( "position", m_position, "Target locator ID", Be::READ )
		MAP_ATTRIBUTE( "positionOld", m_positionOld, "Target locator ID", Be::READ )
		MAP_ATTRIBUTE( "positionOldInfluence", m_positionOldInfluence, "Target locator ID", Be::READ )

	EXPOSURE_END()
}