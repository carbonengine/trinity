// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveThrottleable.h"

BLUE_DEFINE( EveThrottleable );

const Be::ClassInfo* EveThrottleable::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveThrottleable, "" )
		MAP_INTERFACE( EveThrottleable )

		MAP_ATTRIBUTE( "updateThrottle", m_updateThrottle, "enable/disable the frequency based update throttling \n:jessica-group: updateFrequency", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minUpdateFrequency", m_minUpdateFrequency, "how often we process states when the object is very far away\n:jessica-group: updateFrequency", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxUpdateFrequency", m_maxUpdateFrequency, "how often we process states when the object is very close\n:jessica-group: updateFrequency", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "currentUpdateFrequency", m_currentUpdateFrequency, "approximately how many times per sec the controllers updates when throttling is enabled \n:jessica-group: updateFrequency", Be::READ )

	EXPOSURE_END()
}
