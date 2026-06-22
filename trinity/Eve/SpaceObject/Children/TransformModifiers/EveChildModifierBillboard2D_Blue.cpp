// Copyright © 2018 CCP ehf.

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