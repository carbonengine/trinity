//    Created:   Feb 2022
//    Copyright: CCP 2022

#include "StdAfx.h"
#include "Tr2CurveCombiner.h"

BLUE_DEFINE( Tr2CurveCombiner );

const Be::ClassInfo* Tr2CurveCombiner::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveCombiner, ":jessica-icon: tree/trivectorcurve.png" )
		MAP_INTERFACE( Tr2CurveCombiner )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( ITriVectorFunction )
		MAP_INTERFACE( ITriCurveLength )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"curves",
			m_curves,
			"curves combined",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"currentValue",
			m_currentValue,
			"Curve value after the last update",
			Be::READ )

		MAP_METHOD_AND_WRAP(
			"GetValueAt",
			GetValue,
			"Returns curve value at specified time\n"
			":param time: input time"
		)

	EXPOSURE_END()
}