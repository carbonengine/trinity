// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveTrailsSet.h"

BLUE_DEFINE( EveTrailsSet );

const Be::ClassInfo* EveTrailsSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveTrailsSet, "" )
		MAP_INTERFACE( EveTrailsSet )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "display", m_display, "Toggle rendering", Be::READWRITE )

		MAP_ATTRIBUTE( "geometryResPath", m_geometryResPath, "The base geometry that will get instanced", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "fadeSpeed", m_fadeSpeed, "How fast they fade in/out depending on speed of parent", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "effect", m_effect, "Shader used for rendering", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "geometryResource", m_geometryResource, "Base geometry used for rendering", Be::READ )

	EXPOSURE_END()
}
