////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2RotationAdapter.h"


BLUE_DEFINE( Tr2RotationAdapter );

const Be::ClassInfo* Tr2RotationAdapter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2RotationAdapter, ":jessica-icon: tree/trivectorcurve.png" )
		MAP_INTERFACE( Tr2RotationAdapter )
		MAP_INTERFACE( ITriQuaternionFunction )

		MAP_ATTRIBUTE(
			"value",
			m_value,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"curve",
			m_curve,
			"Attached curve",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"currentValue",
			m_currentValue,
			"Curve value after the last update",
			Be::READ )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"RandomizeStart",
			RandomizeStart,
			1,
			"Shits curve starting time by a random number in a specified range (-rng, +rng)\n"
			":param rng: radius of the random range in seconds (default is 60 sec)" )

		MAP_METHOD_AND_WRAP(
			"ScaleTime",
			ScaleTime,
			"Scales curve time\n"
			":param scale: new time scaling factor" )

		MAP_METHOD_AND_WRAP(
			"ResetStart",
			ResetStart,
			"Reset start time" )

	EXPOSURE_END()
}