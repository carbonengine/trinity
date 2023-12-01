#include "StdAfx.h"
#include "EveLineChildContainer.h"

BLUE_DEFINE( EveLineChildContainer );

const Be::ClassInfo* EveLineChildContainer::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveLineChildContainer, "" )
		MAP_INTERFACE( EveLineChildContainer )
		MAP_INTERFACE( IEveLineSetPath )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "lines", m_lines, "Grouped up lines", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isVisible", m_isVisible, "", Be::READ )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
	
	EXPOSURE_END()
}