// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "EveHazeSetItem.h"

BLUE_DEFINE( EveHazeSetItem );

const Be::ClassInfo* EveHazeSetItem::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveHazeSetItem, "" )
		MAP_INTERFACE( EveHazeSetItem )

		MAP_ATTRIBUTE( "name", m_name, "Standard name", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "position", m_position, "Translation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "Scaling", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "Rotation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_color, "Color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hazeData", m_hazeData, "Various data for this haze", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, "the bone index this plane is tight to\n:jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
