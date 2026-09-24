// Copyright © 2024 CCP ehf.

#include "StdAfx.h"
#include "EveChildPostProcessVolume.h"
#include "PostProcess/ITr2PostProcessOwner.h"
#include "PostProcess/Tr2PostProcessEnums.h"
#include "PostProcess/Tr2PostProcessAttributes.h"

BLUE_DEFINE_INTERFACE( ITr2PostProcessOwner );
BLUE_DEFINE( EveChildPostProcessVolume );

const Be::ClassInfo* EveChildPostProcessVolume::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildPostProcessVolume, "" )
		MAP_INTERFACE( ITr2PostProcessOwner )
		MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( EveSpaceObjectChild )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "enabled", m_enabled, "Quickly disable all modifications. It will fade in/out using the minimumTransitionTime", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "intensityMultiplier", m_intensityMultiplier, "Param to scale/control/animate the modifications", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minimumTransitionTime", m_minimumTransitionTime, "Seconds it takes to move between the min and max value", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boundingSphereCenter", m_boundingSphere.center, "", Be::READ )
		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphere.radius, "", Be::READ )
		MAP_ATTRIBUTE( "volumes", m_volumes, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "exclusionVolumes", m_exclusionVolumes, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "postProcessAttributes", m_postProcessAttributes, "", Be::READWRITE | Be::PERSIST )

		MAP_PROPERTY_READONLY( "partTag", GetPartTag, "Part tag for multi-part space objects" )
		MAP_METHOD_AND_WRAP( "GetParent", GetParent, "Returns the parent space object child in the hierarchy" )
		MAP_METHOD_AND_WRAP( "GetOwner", GetOwner, "Returns the owner space object" )
	EXPOSURE_END()
}