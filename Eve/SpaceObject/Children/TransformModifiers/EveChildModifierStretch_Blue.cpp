////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//
#include "StdAfx.h"
#include "EveChildModifierStretch.h"

BLUE_DEFINE( EveChildModifierStretch );

const Be::ClassInfo* EveChildModifierStretch::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierStretch, "" )
		MAP_INTERFACE( EveChildModifierStretch )
		MAP_INTERFACE( IEveChildTransformModifier )

		MAP_ATTRIBUTE( "dest", m_dest, "", Be::READWRITE | Be::PERSIST );

	EXPOSURE_END()
}
