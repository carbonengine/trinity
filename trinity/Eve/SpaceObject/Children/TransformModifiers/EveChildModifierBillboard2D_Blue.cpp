////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#include "StdAfx.h"
#include "EveChildModifierBillboard2D.h"

BLUE_DEFINE( EveChildModifierBillboard2D );

const Be::ClassInfo* EveChildModifierBillboard2D::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierBillboard2D, "" )
		MAP_INTERFACE( EveChildModifierBillboard2D )
		MAP_INTERFACE( IEveChildTransformModifier )

	EXPOSURE_END()
}