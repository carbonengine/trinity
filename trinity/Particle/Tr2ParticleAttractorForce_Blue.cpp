// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "Tr2ParticleAttractorForce.h"

BLUE_DEFINE( Tr2ParticleAttractorForce );

const Be::ClassInfo* Tr2ParticleAttractorForce::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ParticleAttractorForce, "" )
		MAP_INTERFACE( ITr2ParticleForce )
		MAP_INTERFACE( Tr2ParticleAttractorForce )

		MAP_ATTRIBUTE( "magnitude", m_magnitude, "Force magnitude", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "Attractor position", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
