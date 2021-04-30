////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2ReflectionProbe.h"

BLUE_DEFINE( Tr2ReflectionProbe );

const Be::ClassInfo* Tr2ReflectionProbe::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ReflectionProbe, "" )
		MAP_INTERFACE( Tr2ReflectionProbe )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "unfilteredTexture", m_renderTargetCube, "Unfiltered reflection texture", Be::READ )
		MAP_ATTRIBUTE( "reflectionTexture", m_postFilterTarget, "Filtered reflection texture, with different roughness levels in mips", Be::READ )
		MAP_ATTRIBUTE( "position", m_position, "Origin for the reflection", Be::READWRITE )
		MAP_ATTRIBUTE( "reflectionSize", m_intermediateSize, "Size for the unfiltered reflection map", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "customSourceTexture", m_customSourceTexture, "A custom texture for filtering", Be::READWRITE | Be::NOTIFY)
		MAP_ATTRIBUTE( "hdrOutput", m_hdrOutput, "Generate HDR reflection texture", Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE( "hackMode", m_hackMode, "Enables evil hacks, do not touch me\n:jessica-group: Hacks", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "backlightColor", m_backlightColor, "Back light color\n:jessica-group: Hacks", Be::READWRITE )
		MAP_ATTRIBUTE( "backlightContrast", m_backlightContrast, "Back light shape/size\n:jessica-group: Hacks", Be::READWRITE )
		MAP_ATTRIBUTE( "intensity", m_intensity, "Reflection texture scale\n:jessica-group: Hacks", Be::READWRITE )


		MAP_METHOD_AND_WRAP( "RunFilter", RunFilter, "Filters the currently set texture" )
	EXPOSURE_END()
}
