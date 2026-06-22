// Copyright © 2026 CCP ehf.

#include "EvePicking.h"

BLUE_DEFINE( EvePickingContext );


const Be::ClassInfo* EvePickingContext::ExposeToBlue()
{
	EXPOSURE_BEGIN( EvePickingContext, "" )
		MAP_INTERFACE( EvePickingContext )
	EXPOSURE_END()
}