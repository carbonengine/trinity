////////////////////////////////////////////////////////////
//
//    Created:   October 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveRemotePositionCurve.h"

BLUE_DEFINE( EveRemotePositionCurve );

const Be::ClassInfo* EveRemotePositionCurve::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveRemotePositionCurve, "" )
		MAP_INTERFACE( ITriVectorFunction )

		MAP_ATTRIBUTE( "value", m_value, "The actual value/result of this curve", Be::READWRITE )
		MAP_ATTRIBUTE( "startPositionCurve", m_startPositionCurve, "The start position curve", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "offsetDir1", m_offsetDir1, "The starting offset", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "offsetDir2", m_offsetDir2, "The ending offset", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "delayTime", m_delayTime, "A certain time to wait until the sweep starts", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sweepTime", m_sweepTime, "Total time of one sweep", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "cycle", m_cycle, "Does it start all over again?", Be::READWRITE | Be::PERSIST )
		
	EXPOSURE_END()
}
