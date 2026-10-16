// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierShake.h"

BLUE_DEFINE( EveChildModifierShake );

const Be::ClassInfo* EveChildModifierShake::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierShake, "Shakes an object in place with smooth 3D noise around its original position." )
		MAP_INTERFACE( EveChildModifierShake )
		MAP_INTERFACE( IEveChildTransformModifier )

		MAP_ATTRIBUTE( "frequency", m_frequency, "Noise frequency. Higher values produce faster, more turbulent motion.", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "amplitude", m_amplitude, "Upper bound on the displacement along each axis, in either direction.", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "timeOffset", m_timeOffset, "Seconds added to the clock. Objects with the same settings shake in sync unless given different offsets.", Be::READWRITE | Be::PERSIST );

	EXPOSURE_END()
}
