#include "StdAfx.h"
#include "EveCurveLineSet.h"

BLUE_DEFINE( EveCurveLineSet );

const Be::ClassInfo* EveCurveLineSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveCurveLineSet, "" )
		MAP_INTERFACE( EveCurveLineSet )
		MAP_INTERFACE( IEveTransform )
		MAP_INTERFACE( IEveSpaceObject2 )
	EXPOSURE_CHAINTO( Tr2CurveLineSet )
}
