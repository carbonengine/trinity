////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#include "StdAfx.h"
#include "EveChildModifierBillboard3D.h"

BLUE_DEFINE( EveChildModifierBillboard3D );

const Be::ClassInfo* EveChildModifierBillboard3D::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierBillboard3D, "" )
		MAP_INTERFACE( EveChildModifierBillboard3D )
		MAP_INTERFACE( IEveChildTransformModifier )
		MAP_ATTRIBUTE( "fixed", m_fixed, "Is this billboard fixed and will not rotate based on the view direction", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}