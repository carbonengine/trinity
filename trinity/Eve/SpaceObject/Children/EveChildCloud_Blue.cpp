// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "EveChildCloud.h"
#include "EveCloudEditableVolume.h"

BLUE_DEFINE( EveChildCloud );

const Be::ClassInfo* EveChildCloud::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildCloud, "Cloud space object child" )
		MAP_INTERFACE( EveChildCloud )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IEveSpaceObjectChild )

		MAP_ATTRIBUTE( "name", m_name, "The name of the cloud", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Toggle display", Be::READWRITE )
		MAP_ATTRIBUTE( "sortingModifier", m_sortingModifier, "Affects the transparency sorting", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "effect", m_effect, "Shader used for the rendering", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "scaling", m_scaling, "Object scaling", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "Object local translation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "Object local rotation", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "boundingSphere", m_boundingSphere, "Used for culling", Be::READ )
		MAP_ATTRIBUTE( "preTesselationLevel", m_preTesselationLevel, "Number of triangles per width/heigth", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "volume", m_volume, "Shape volume texture editor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"minScreenSize",
			m_minScreenSize,
			"Minimal size of object on screen, objects smaller than this size are not rendered.\n:jessica-group: LOD",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "cellScreenSize", m_cellScreenSize, "Target size of a single cell in a grid on the screen (in pixels)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "currentLod", m_currentIB, "Current cloud LOD", Be::READ )
	EXPOSURE_END()
}
