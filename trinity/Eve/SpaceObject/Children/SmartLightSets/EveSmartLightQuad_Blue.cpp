// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightQuad.h"

BLUE_DEFINE( EveSmartLightQuad );

const Be::ClassInfo* EveSmartLightQuad::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightQuad, ":jessica-icon: diamond\n" )
		MAP_INTERFACE( EveSmartLightQuad )
		MAP_INTERFACE( EveSmartLightBaseGroup )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Show/hide object", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "editMode", m_editMode, "Turn on when tweaking effect", Be::READWRITE )
		MAP_ATTRIBUTE( "effect", m_effect, "Effect to be used for rendering a quad\n:jessica-hidden: True", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "brightness", m_brightness, "Quad brightness", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "softQuad", m_softQuad, "use soft quad instead of the regular", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "staticOffsetTranslation", m_staticOffsetTranslation, "static per instance offset in local space before placement \n:jessica-group: StaticOffsetFromDistribution", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticQuadScale", m_staticQuadScale, "static multiplier in local space before placement (can deform) \n:jessica-group: StaticOffsetFromDistribution", Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( EveSmartLightBaseGroup )
}
