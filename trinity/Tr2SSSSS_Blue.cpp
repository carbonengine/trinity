// Copyright © 2024 CCP ehf.

#include "StdAfx.h"
#include "Tr2SSSSS.h"

BLUE_DEFINE( Tr2SSSSS );

const Be::ClassInfo* Tr2SSSSS::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SSSSS, "" )
		MAP_INTERFACE( Tr2SSSSS )

		MAP_ATTRIBUTE( "enabled", m_enabled, "Test setting\n:jessica-group: Settings", Be::READWRITE )



		MAP_ATTRIBUTE( "subSurfaceScatteringWidth", m_subSurfaceScatteringWidth, "How wide of a sample radius for the sub surface blur\n"
																				 ":jessica-group: Settings",
					   Be::READWRITE )
		MAP_ATTRIBUTE( "subSurfaceFrontScatterColor", m_subSurfaceFrontScatterColor, "Defines what the scene level sub surface front scatter color should be\n"
																					 ":jessica-group: Settings",
					   Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "hasSSSSSInScene", m_hasSSSSSInScene, "Shows if a SSSSS object is in the scene", Be::READ )

	EXPOSURE_END()
}
