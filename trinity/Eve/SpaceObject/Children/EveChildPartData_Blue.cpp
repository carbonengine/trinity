// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildPartData.h"


BLUE_DEFINE( EveChildPartData );

const Be::ClassInfo* EveChildPartData::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildPartData, "Persistent state of a modular space object (per-part transforms and bounds). Edit through EveModularObjectModifier" )
		MAP_INTERFACE( EveSpaceObjectChild );
		MAP_INTERFACE( IEveSpaceObjectChild )
	EXPOSURE_CHAINTO( EveSpaceObjectChild )
}
