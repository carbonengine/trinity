////////////////////////////////////////////////////////////
//
//    Created:   2017
//    Copyright: CCP 2017
//
#include "StdAfx.h"
#include "EveChildModifierSRT.h"

BLUE_DEFINE( EveChildModifierSRT );
BLUE_DEFINE_INTERFACE( IEveChildTransformModifier );

const Be::ClassInfo* EveChildModifierSRT::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveChildModifierSRT, "" )
        MAP_INTERFACE( EveChildModifierSRT )
        MAP_INTERFACE( IEveChildTransformModifier )

		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST );

    EXPOSURE_END()
}
