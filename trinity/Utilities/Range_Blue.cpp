#include "StdAfx.h"
#include "Range.h"

BLUE_DEFINE( Range );

const Be::ClassInfo* Range::ExposeToBlue()
{
	EXPOSURE_BEGIN( Range, "" )
		MAP_INTERFACE( Range )

		//MAP_ATTRIBUTE( "blendRangeMin", m_centerPoint, "", Be::READWRITE | Be::PERSIST )

		MAP_PROPERTY_READONLY( "centerPoint", GetCenterPoint, "is the center-point in the middle of the ranges" )
		MAP_PROPERTY_READONLY( "minRangePoint", GetMinRangePoint, "is the center-point in the middle of the ranges" )
		MAP_PROPERTY_READONLY( "maxRangePoint", GetMaxRangePoint, "is the center-point in the middle of the ranges" )
		MAP_PROPERTY_READONLY( "isUniform", GetIsUniform, "is the center-point in the middle of the ranges" )

	EXPOSURE_END()
}
