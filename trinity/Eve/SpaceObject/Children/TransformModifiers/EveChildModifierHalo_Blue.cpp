// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierHalo.h"

BLUE_DEFINE( EveChildModifierHalo );

const Be::ClassInfo* EveChildModifierHalo::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierHalo, "" )
		MAP_INTERFACE( EveChildModifierHalo )
		MAP_INTERFACE( IEveChildTransformModifier )

	EXPOSURE_END()
}