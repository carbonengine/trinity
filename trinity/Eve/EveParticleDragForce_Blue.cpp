// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveParticleDragForce.h"

BLUE_DEFINE( EveParticleDragForce );

const Be::ClassInfo* EveParticleDragForce::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveParticleDragForce, "" )
		MAP_INTERFACE( EveParticleDragForce )
	EXPOSURE_CHAINTO( Tr2ParticleDragForce )
}
