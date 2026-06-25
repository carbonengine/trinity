// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "EveChildLink.h"

BLUE_DEFINE( EveChildLink );

const Be::ClassInfo* EveChildLink::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildLink, "" )
		MAP_INTERFACE( EveChildLink )
		MAP_INTERFACE( EveChildMesh )

		MAP_ATTRIBUTE( "currentDirection", m_currentDirection, "Current normalized direction to link source in worldspace", Be::READ )
		MAP_ATTRIBUTE( "currentDistance", m_currentDistance, "Current distance to link source", Be::READ )
		MAP_ATTRIBUTE( "target", m_target, "Link source destiny ball", Be::READWRITE )
		MAP_ATTRIBUTE( "linkStrength", m_linkStrength, "Normalized value indicating link strength", Be::READ )
		MAP_ATTRIBUTE( "linkBarrier", m_linkBarrier, "Absolute barrier radius", Be::READWRITE )
		MAP_ATTRIBUTE( "targetRadius", m_targetRadius, "The radius of the target, if we are within it then we are at 100% strength", Be::READWRITE )
		MAP_ATTRIBUTE( "linkStrengthCurves", m_linkStrengthCurves, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "linkStrengthBindings", m_linkStrengthBindings, "", Be::READ | Be::PERSIST )

	EXPOSURE_CHAINTO( EveChildMesh )
}