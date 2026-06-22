// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierBooster.h"

BLUE_DEFINE( EveChildModifierBooster );

const Be::ClassInfo* EveChildModifierBooster::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierBooster, "" )
		MAP_INTERFACE( EveChildModifierBooster )
		MAP_INTERFACE( IEveChildTransformModifier )

	EXPOSURE_END()
}