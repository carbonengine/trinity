// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "EveMissile.h"

BLUE_DEFINE( EveMissile );

const Be::ClassInfo* EveMissile::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveMissile, "" )
		MAP_INTERFACE( EveMissile )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2Renderable )

		MAP_ATTRIBUTE( "warheads", m_warheads, "", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "updateWarheads", m_updateWarheads, "", Be::READWRITE )

		MAP_ATTRIBUTE( "target", m_target, "", Be::READWRITE )
		MAP_ATTRIBUTE( "targetRadius", m_targetRadius, "", Be::READWRITE )
#if BLUE_WITH_PYTHON
		MAP_ATTRIBUTE( "explosionCallback", m_callback, "", Be::READWRITE )
#endif


		MAP_METHOD_AND_WRAP(
			"Start",
			Start,
			"Start a missile from a ship into space.\n"
			":param shipVelocity: velocity vector of initial ship speed in missile's destinyball's space.\n"
			":param estimatedFlyingTime: rough estimate fo the flight time." )

		MAP_METHOD_AND_WRAP(
			"RebuildMissileBoundingSphere",
			RebuildMissileBoundingSphere,
			"Rebuild the missile object's bounding sphere based on the internal position of all attached warheads.\n" )

	EXPOSURE_CHAINTO( EveSpaceObject2 )
}
