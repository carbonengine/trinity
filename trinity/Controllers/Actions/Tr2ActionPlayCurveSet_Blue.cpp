////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ActionPlayCurveSet.h"


BLUE_DEFINE( Tr2ActionPlayCurveSet );

const Be::ClassInfo* Tr2ActionPlayCurveSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionPlayCurveSet, "" )
		MAP_INTERFACE( Tr2ActionPlayCurveSet )
		MAP_INTERFACE( ITr2ControllerAction )

		MAP_ATTRIBUTE( "curveSetName", m_curveSetName, "Curve set name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rangeName", m_rangeName, "Name of the curve set time range (or empty to play the full curve set)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "syncToRange", m_syncToRange, "Only transition out of owner state if curve set range has played full loop", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
