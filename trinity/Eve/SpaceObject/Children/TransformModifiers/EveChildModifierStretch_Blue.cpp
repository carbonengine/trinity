// Copyright © 2020 CCP ehf.

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
