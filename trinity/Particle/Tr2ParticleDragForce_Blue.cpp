////////////////////////////////////////////////////////////
//
//    Created:   September 2010
//    Copyright: CCP 2010
//
//    Refactored from EveParticleDragForce_Blue.cpp

#include "StdAfx.h"
#include "Tr2ParticleDragForce.h"

BLUE_DEFINE( Tr2ParticleDragForce );

const Be::ClassInfo* Tr2ParticleDragForce::ExposeToBlue()
{
	EXPOSURE_BEGIN(Tr2ParticleDragForce, "" )
		MAP_INTERFACE( ITr2ParticleForce )
		MAP_INTERFACE( Tr2ParticleDragForce )

		MAP_ATTRIBUTE( "drag",  m_dragConstant,  "Applies a drag force: -bV where V is the velocity of a particle", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
