// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierHaloInverted.h"

BLUE_DEFINE( EveChildModifierHaloInverted );

const Be::ClassInfo* EveChildModifierHaloInverted::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierHaloInverted, "" )
		MAP_INTERFACE( EveChildModifierHaloInverted )
		MAP_INTERFACE( IEveChildTransformModifier )

	EXPOSURE_END()
}