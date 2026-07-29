// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierRotate.h"

BLUE_DEFINE( EveChildModifierRotate );

const Be::ClassInfo* EveChildModifierRotate::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierRotate, "Rotates an object continuously around its local axes. Angle is recomputed from absolute time each frame so the spin does not drift." )
		MAP_INTERFACE( EveChildModifierRotate )
		MAP_INTERFACE( IEveChildTransformModifier )

		MAP_ATTRIBUTE( "rotationSpeed", m_rotationSpeed, "Local-axis rotation speed in radians per second (x, y, z).", Be::READWRITE | Be::PERSIST );

	EXPOSURE_END()
}
