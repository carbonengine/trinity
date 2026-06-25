// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2QuaternionLerpCurve.h"

BLUE_DEFINE( Tr2QuaternionLerpCurve );

const Be::ClassInfo* Tr2QuaternionLerpCurve::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2QuaternionLerpCurve, ":jessica-deprecated: True" )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( ITriQuaternionFunction )
		MAP_INTERFACE( ITriCurveLength )

		MAP_ATTRIBUTE(
			"start",
			m_start,
			"The time at which the sequence should begin",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"length",
			m_length,
			"Length of the ",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"value",
			m_value,
			"na",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"startCurve",
			m_startCurve,
			"Starting rotation curve",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"endCurve",
			m_endCurve,
			"End rotation curve",
			Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
