#include "StdAfx.h"
#include "EveParticleDirectForce.h"

BLUE_DEFINE( EveParticleDirectForce );

const Be::ClassInfo* EveParticleDirectForce::ExposeToBlue()
{
	EXPOSURE_BEGIN(EveParticleDirectForce, "" )
		MAP_INTERFACE( EveParticleDirectForce )
	EXPOSURE_CHAINTO( Tr2ParticleDirectForce )
}
