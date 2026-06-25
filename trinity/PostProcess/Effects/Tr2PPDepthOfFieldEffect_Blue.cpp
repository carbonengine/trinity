// Copyright © 2021 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPDepthOfFieldEffect.h"

BLUE_DEFINE( Tr2PPDepthOfFieldEffect );

namespace Tr2Bokeh
{
const Be::VarChooser BokehShapeChooser[] = {
	{ "Disk", BeCast( Tr2Bokeh::Disk ), "A perfectly circular aperture" },
	{ "Triangle", BeCast( Tr2Bokeh::Triangle ), "An aperture with 3 sides" },
	{ "Rectangle", BeCast( Tr2Bokeh::Rectangle ), "An aperture with 4 sides" },
	{ "Pentagon", BeCast( Tr2Bokeh::Pentagon ), "An aperture with 5 sides" },
	{ "Hexagon", BeCast( Tr2Bokeh::Hexagon ), "An aperture with 6 sides" },
	{ "Heart", BeCast( Tr2Bokeh::Heart ), "A heart-shaped aperture <3" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "BokehShapeType", Shape, BokehShapeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
}
const Be::ClassInfo* Tr2PPDepthOfFieldEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPDepthOfFieldEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "focalDistance", m_focalDistance, "The distance from the camera to the focal plane. Any object that lies on the focal plane is fully in-focus", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "focalLength", m_focalLength, "How far an object can be from the focal plane before it goes completely out-of-focus", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scale", m_scale, "A value that scales the blur kernel", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "cocScale", m_cocScale, "A value that scales the coc texture compared to the source", Be::READWRITE )
		MAP_ATTRIBUTE( "foregroundBlurNeeded", m_foregroundBlurNeeded, "If foreground is always in focus, we can safely uncheck this", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "bokehShape", m_bokehShape, "What is the shape of the bokeh", Be::READWRITE | Be::PERSIST | Be::ENUM, Tr2Bokeh::BokehShapeChooser );
		MAP_ATTRIBUTE( "useTAAFriendlyBokeh", m_useTAAFriendlyBokeh, "Enables a separate Bokeh shader when TAA is enabled that is optimized to reduce flickering", Be::READWRITE )

	EXPOSURE_CHAINTO( Tr2PPEffect )
}
