#include "StdAfx.h"
#include "SeekTarget.h"

BLUE_DEFINE( SeekTarget );

const Be::ClassInfo* SeekTarget::ExposeToBlue()
{
	EXPOSURE_BEGIN( SeekTarget, "" )
		MAP_INTERFACE( SeekTarget )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE( "exit", m_exit, ":jessica-group: BackAndForth", Be::READWRITE )
		MAP_ATTRIBUTE( "tunnelBehavior", m_tunnelBehavior, ":jessica-group: BackAndForth", Be::READWRITE )
		MAP_ATTRIBUTE( "fxBehavior", m_fxBehavior, ":jessica-group: BackAndForth", Be::READWRITE )
		MAP_ATTRIBUTE( "target", m_target, ":jessica-group: BackAndForth", Be::READWRITE )
		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, ":jessica-group: BackAndForth", Be::READWRITE )
		MAP_ATTRIBUTE( "arrivedRadius", m_arrivedRadius, ":jessica-group: BackAndForth", Be::READWRITE )
		MAP_ATTRIBUTE( "slowDownRadius", m_slowDownRadius, ":jessica-group: BackAndForth", Be::READWRITE )
		MAP_ATTRIBUTE( "distanceFromShip", m_distanceFromShip, ":jessica-group: BackAndForth", Be::READWRITE )

		MAP_METHOD_AND_WRAP(
			"SetTarget",
			SetTarget,
			"Assigns target.\n"
			":param transforms: target" )

		MAP_METHOD_AND_WRAP(
			"SetExit",
			SetExit,
			"Set exit value.\n"
			":param transforms: target" )

		EXPOSURE_END()
}