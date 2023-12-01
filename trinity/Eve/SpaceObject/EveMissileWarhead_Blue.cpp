////////////////////////////////////////////////////////////
//
//    Created:   February 2012
//    Copyright: CCP 2012
//
#include "StdAfx.h"
#include "EveMissileWarhead.h"

BLUE_DEFINE( EveMissileWarhead );

const Be::ClassInfo* EveMissileWarhead::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveMissileWarhead, "" )
        MAP_INTERFACE( EveMissileWarhead )
		MAP_INTERFACE( EveTransform )

		MAP_ATTRIBUTE( "startDataValid", m_startDataValid, "", Be::READWRITE )

		MAP_ATTRIBUTE( "warheadLength", m_warheadLength, "Warhead model length", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "warheadRadius", m_warheadRadius, "Warhead model radius", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "pathOffset", m_pathOffset, "", Be::READWRITE )
		MAP_ATTRIBUTE( "durationEjectPhase", m_durationEjectPhase, "The total length of the warhead's ejection phase.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "startEjectVelocity", m_startEjectVelocity, "The warhead's speed at start of the ejection phase.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "acceleration", m_acceleration, "The warhead's acceleration after eject phase is complete.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxExplosionDistance", m_maxExplosionDistance, "Determine how far away from the target the warhead should explode.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "impactSize", m_impactSize, "Size of impact on targets.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "impactDuration", m_impactDuration, "Duration of impact on target.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "id", m_id, "The warhead's acceleration after eject phase is complete.", Be::READWRITE )
		MAP_ATTRIBUTE( "explosionPosition", m_explosionPosition, "Position in world space where the missile exploded", Be::READ )
		MAP_ATTRIBUTE( "doSpread", m_doSpread, "The warhead should(possibly) switch to a new damage locator if appropriate mid flight.", Be::READWRITE )
		MAP_ATTRIBUTE( "targetLocatorID", m_targetLocator, "The target locator id that the warhead is aiming for.", Be::READ )
		MAP_ATTRIBUTE( "spriteSet", m_spriteSet, "Sprites for the warhead.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "pathOffsetNoiseScale", m_pathOffsetNoiseScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "pathOffsetNoiseSpeed", m_pathOffsetNoiseSpeed, "", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"PrepareLaunch",
			PrepareLaunch,
			"Description:\n"
			"Initalize a warhead without launching it.\n" )

		MAP_METHOD_AND_WRAP(
			"Launch",
			Launch,
			"Launch warhead.\n"
			":param startTransform: transform matrix of initial position/orientation in warheads's destinyball's space.\n" )

	EXPOSURE_CHAINTO( EveTransform )
}
