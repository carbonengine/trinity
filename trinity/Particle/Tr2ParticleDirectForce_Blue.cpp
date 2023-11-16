////////////////////////////////////////////////////////////
//
//    Created:   September 2010
//    Copyright: CCP 2010
//
//    Refactored from EveParticleDirectForce_Blue.cpp

#include "StdAfx.h"
#include "Tr2ParticleDirectForce.h"

BLUE_DEFINE( Tr2ParticleDirectForce );

const Be::ClassInfo* Tr2ParticleDirectForce::ExposeToBlue()
{
	EXPOSURE_BEGIN(Tr2ParticleDirectForce, "" )
		MAP_INTERFACE( ITr2ParticleForce )
		MAP_INTERFACE( Tr2ParticleDirectForce )

		MAP_ATTRIBUTE( "force", m_force, "Applies a constant directional force", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
