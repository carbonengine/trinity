// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "Tr2ParticleVortexForce.h"

BLUE_DEFINE( Tr2ParticleVortexForce );

const Be::ClassInfo* Tr2ParticleVortexForce::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ParticleVortexForce, "" )
		MAP_INTERFACE( ITr2ParticleForce )
		MAP_INTERFACE( Tr2ParticleVortexForce )

		MAP_ATTRIBUTE( "magnitude", m_magnitude, "Force magnitude", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "Vortex position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "axis", m_axis, "Vortex axis", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
