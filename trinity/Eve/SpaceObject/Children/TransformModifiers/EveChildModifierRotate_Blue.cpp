// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierRotate.h"

BLUE_DEFINE( EveChildModifierRotate );

const Be::ClassInfo* EveChildModifierRotate::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierRotate, "Spins an object continuously around its local axes." )
		MAP_INTERFACE( EveChildModifierRotate )
		MAP_INTERFACE( IEveChildTransformModifier )

		MAP_ATTRIBUTE( "rotationSpeed", m_rotationSpeed, "Euler rotation speed in degrees per second around the local axes: x is pitch, y is yaw, z is roll.", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "timeOffset", m_timeOffset, "Seconds added to the clock. Objects with the same rotation speed spin in sync unless given different offsets.", Be::READWRITE | Be::PERSIST );

	EXPOSURE_END()
}
