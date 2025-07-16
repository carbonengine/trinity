////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2ReflectionProbe.h"

BLUE_DEFINE( Tr2ReflectionProbe );

const Be::VarChooser RenderFrequencyChooser[] = {
	{ "OneSidePerFrame", BeCast( Tr2ReflectionProbe::ONE_SIDE_PER_FRAME ), "One side per frame" },
	{ "AllSidesPerFrame", BeCast( Tr2ReflectionProbe::ALL_SIDES_PER_FRAME ), "All sides per frame" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( "ReflectionProbeRenderFrequency", Tr2ReflectionProbe::ReflectionProbeRenderFrequency, RenderFrequencyChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Tr2ReflectionProbe::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ReflectionProbe, "" )
		MAP_INTERFACE( Tr2ReflectionProbe )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "unfilteredTexture", m_renderTargetCube, "Unfiltered reflection texture", Be::READ )
		MAP_ATTRIBUTE( "reflectionTexture", m_postFilterTarget, "Filtered reflection texture, with different roughness levels in mips", Be::READ )
		MAP_ATTRIBUTE( "position", m_position, "Origin for the reflection", Be::READWRITE )
		MAP_ATTRIBUTE( "lockPosition", m_lockPosition, "Lock the position of the reflection", Be::READWRITE )
		MAP_ATTRIBUTE( "reflectionSize", m_intermediateSize, "Size for the unfiltered reflection map", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "customSourceTexture", m_customSourceTexture, "A custom texture for filtering", Be::READWRITE | Be::NOTIFY)
		MAP_ATTRIBUTE( "hdrOutput", m_hdrOutput, "Generate HDR reflection texture", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE_WITH_CHOOSER( "renderFrequency", m_renderFrequency, "", Be::READWRITE | Be::NOTIFY | Be::ENUM, RenderFrequencyChooser )
		MAP_ATTRIBUTE( "currentFrame", m_currentFrame, "", Be::READ )

		MAP_ATTRIBUTE( "hollywoodMode", m_hollywoodMode, "Enables/Disables Hollywood backlight mode \n:jessica-group: Hollywood", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "backlightContrast", m_backlightContrast, "Back light shape/size. Set by the scene\n:jessica-group: Hollywood", Be::READ )
		MAP_ATTRIBUTE( "backlightColor", m_backlightColor, "Back light color. Set by the scene\n:jessica-group: Hollywood", Be::READ )

		MAP_METHOD_AND_WRAP( "RunFilter", RunFilter, "Filters the currently set texture" )
	EXPOSURE_END()
}
