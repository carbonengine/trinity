////////////////////////////////////////////////////////////
//
//    Created:   2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveConnector.h"

BLUE_DEFINE( EveConnector );

const Be::ClassInfo* EveConnector::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveConnector, "" )
        MAP_INTERFACE( EveConnector )

		MAP_ATTRIBUTE( "color", m_color, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "lineWidth", m_width, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "animationColor", m_animationColor, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "animationScale", m_animationScale, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "animationSpeed", m_animationSpeed, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "isAnimated", m_isAnimated, "", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "destPosition", m_destPosition, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "sourcePosition", m_sourcePosition, "", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "destObject", m_destObject, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "sourceObject", m_sourceObject, "", Be::READWRITE | Be::PERSIST );

    EXPOSURE_END()
}