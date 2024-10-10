////////////////////////////////////////////////////////////////////////////////
//
// Created:   October 2024
// Copyright: CCP 2024
//

#include "StdAfx.h"
#include "Tr2SSSSS.h"

BLUE_DEFINE( Tr2SSSSS );

const Be::ClassInfo* Tr2SSSSS::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SSSSS, "" )
		MAP_INTERFACE( Tr2SSSSS )

		MAP_ATTRIBUTE( "enabled", m_enabled, "Test setting\n:jessica-group: Settings", Be::READWRITE )

		

		MAP_ATTRIBUTE( "subSurfaceScatteringWidth", m_subSurfaceScatteringWidth, "How wide of a sample radius for the sub surface blur\n" ":jessica-group: Settings",Be::READWRITE )
		MAP_ATTRIBUTE( "subSurfaceScatteringFOV", m_subSurfaceScatteringFOV, "What FOV should the virtual sub surface scatter use\n"":jessica-group: Settings", Be::READWRITE )
		MAP_ATTRIBUTE( "subSurfaceFrontScatterColor", m_subSurfaceFrontScatterColor, "Defines what the scene level sub surface front scatter color should be\n" ":jessica-group: Settings", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "seprableSpecularTexture", m_seprableSpecularColorMap, ".", Be::READWRITE )

	EXPOSURE_END()
}
