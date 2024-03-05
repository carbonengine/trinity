////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
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