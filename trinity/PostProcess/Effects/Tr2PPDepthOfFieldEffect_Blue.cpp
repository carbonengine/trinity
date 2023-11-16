////////////////////////////////////////////////////////////////////////////////
//
// Created:		December 2021
// Copyright:	CCP 2021
//

#include "StdAfx.h"
#include "Tr2PPDepthOfFieldEffect.h"

BLUE_DEFINE( Tr2PPDepthOfFieldEffect );

const Be::VarChooser BokehShapeChooser[] = {
	{ "Disk", BeCast( Tr2Bokeh::Disk ), "A disk shape" },
	{ "Rectangle", BeCast( Tr2Bokeh::Rectangle ), "A Rectangle shape" },
	{ "Triangle", BeCast( Tr2Bokeh::Triangle ), "A Triangle shape" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "BokehShapeType", Tr2Bokeh::Shape, BokehShapeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::VarChooser DoFDebugMode[] = {
	{ "Off", BeCast( Tr2PPDepthOfFieldEffect::DofDebug_Off ), "No Debugging" },
	{ "CoC", BeCast( Tr2PPDepthOfFieldEffect::DofDebug_CoC ), "Debug the Circle of Confusion" },
	{ "CoC Blurred", BeCast( Tr2PPDepthOfFieldEffect::DofDebug_CoCBlurred ), "Debug the Blurred Circle of confusion" },
	{ "Bokeh Blend", BeCast( Tr2PPDepthOfFieldEffect::DofDebug_BokehBlend ), "Debug the Bokeh blendpass" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "DoFDebugMode", Tr2PPDepthOfFieldEffect::DofDebug, DoFDebugMode, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Tr2PPDepthOfFieldEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPDepthOfFieldEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "focalDistance", m_focalDistance, "The distance from the camera to the focal plane. Any object that lies on the focal plane is fully in-focus", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "focalLength", m_focalLength, "How far an object can be from the focal plane before it goes completely out-of-focus", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "scale", m_scale, "A value that scales the blur kernel", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "cocScale", m_cocScale, "A value that scales the coc texture compared to the source", Be::READWRITE )
		MAP_ATTRIBUTE( "foregroundBlurNeeded", m_foregroundBlurNeeded, "If foreground is always in focus, we can safely uncheck this", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "debug", m_debug, "A value that sets debug mode", Be::READWRITE | Be::ENUM, DoFDebugMode );
		MAP_ATTRIBUTE_WITH_CHOOSER( "bokehShape", m_bokehShape, "What is the shape of the bokeh", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BokehShapeChooser );
		MAP_ATTRIBUTE( "useTAAFriendlyBokeh", m_useTAAFriendlyBokeh, "Enables a separate Bokeh shader when TAA is enabled that is optimized to reduce flickering", Be::READWRITE | Be::NOTIFY )
		
	EXPOSURE_CHAINTO( Tr2PPEffect )
}
