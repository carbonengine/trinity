// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveParticleSpringAttractor.h"

BLUE_DEFINE( EveParticleSpring );

const Be::ClassInfo* EveParticleSpring::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveParticleSpring, "" )
		MAP_INTERFACE( EveParticleSpring )
	EXPOSURE_CHAINTO( Tr2ParticleSpring )
}
