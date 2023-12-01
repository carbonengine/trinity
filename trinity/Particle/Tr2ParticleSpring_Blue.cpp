////////////////////////////////////////////////////////////
//
//    Created:   September 2010
//    Copyright: CCP 2010
//
//    Refactored from EveParticleSpringAttractor_Blue.cpp

#include "StdAfx.h"
#include "Tr2ParticleSpring.h"

BLUE_DEFINE( Tr2ParticleSpring );

const Be::ClassInfo* Tr2ParticleSpring::ExposeToBlue()
{
	EXPOSURE_BEGIN(Tr2ParticleSpring, "" )
		MAP_INTERFACE( ITr2ParticleForce )
		MAP_INTERFACE( Tr2ParticleSpring )

		MAP_ATTRIBUTE( "springConstant",  m_springConstant,  "Applies a force from or towards a point proportional to the distance to that point", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
