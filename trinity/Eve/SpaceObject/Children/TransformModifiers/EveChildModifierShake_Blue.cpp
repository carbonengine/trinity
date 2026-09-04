// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierShake.h"

BLUE_DEFINE( EveChildModifierShake );

const Be::ClassInfo* EveChildModifierShake::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierShake, "Shakes an object in place with randomized 3D noise around its original transform. Direction covers all axes (positive and negative); offset is recomputed each frame so the object cannot drift." )
		MAP_INTERFACE( EveChildModifierShake )
		MAP_INTERFACE( IEveChildTransformModifier )

		MAP_ATTRIBUTE( "frequency", m_frequency, "Noise frequency. Higher values produce faster, more turbulent motion.", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "amplitude", m_amplitude, "Maximum displacement from the origin. Motion reaches every direction within this radius, including negative axes.", Be::READWRITE | Be::PERSIST );

	EXPOSURE_END()
}
