// Copyright © 2021 CCP ehf.

#include "EveProceduralMethodCyclingParameter.h"


BLUE_DEFINE( EveProceduralMethodCyclingParameter );

const Be::ClassInfo* EveProceduralMethodCyclingParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveProceduralMethodCyclingParameter, "" )
		MAP_INTERFACE( EveProceduralMethodCyclingParameter )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "a descriptive name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "child", m_child, ":jessica-icon: fa-suitcase-rolling", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "playDuration", m_playDuration, "how long we stay on the child until we move on to the next one", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "restartRequired", m_restartRequired, "restarts curvesets / controlers when activated", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "reloadRequired", m_reloadRequired, "only enable this if child needs to be reloaded for each loop", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
