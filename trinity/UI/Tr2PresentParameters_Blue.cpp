// Copyright © 2013 CCP ehf.

#include "StdAfx.h"
#include "Tr2PresentParameters.h"
#include "TriDevice.h"

BLUE_DEFINE( Tr2PresentParameters );

const Be::ClassInfo* Tr2PresentParameters::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PresentParameters, "Structure used to pass values to functions creating the device" )
		MAP_ATTRIBUTE(
			"backBufferWidth",
			mode.width,
			"",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"backBufferHeight",
			mode.height,
			"",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"windowed",
			windowed,
			"",
			Be::READWRITE )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"software",
			software,
			"",
			Be::READWRITE | Be::ENUM,
			TriDeviceTypeChooser )
	EXPOSURE_END()
}