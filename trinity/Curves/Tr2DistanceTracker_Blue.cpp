// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2DistanceTracker.h"

BLUE_DEFINE( Tr2DistanceTracker );

const Be::ClassInfo* Tr2DistanceTracker::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2DistanceTracker, ":jessica-deprecated: True" )

		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "value", m_value, "", Be::READ )

		MAP_ATTRIBUTE( "signedDistance", m_signedDistance, "Is distance signed based on direction", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "distanceToClosest", m_distanceToClosest, "Calculate distance to closest intersection point", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "direction", m_direction, "Directional vector used for signed distance and closest intersection", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "sourceObject", m_source, "Distance is calculated from source to the target object", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "targetObject", m_target, "Distance is calculated from source to the target object", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "sourcePosition", m_sourcePosition, "The source object position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "targetPosition", m_targetPosition, "The target object position", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}