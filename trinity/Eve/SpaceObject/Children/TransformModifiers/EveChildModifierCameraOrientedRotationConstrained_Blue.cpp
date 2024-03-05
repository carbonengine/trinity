////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#include "StdAfx.h"
#include "EveChildModifierCameraOrientedRotationConstrained.h"

BLUE_DEFINE( EveChildModifierCameraOrientedRotationConstrained );

const Be::ClassInfo* EveChildModifierCameraOrientedRotationConstrained::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierCameraOrientedRotationConstrained, "" )
		MAP_INTERFACE( EveChildModifierCameraOrientedRotationConstrained )
		MAP_INTERFACE( IEveChildTransformModifier )

	EXPOSURE_END()
}