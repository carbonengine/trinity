// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPDynamicExposureEffect.h"

BLUE_DEFINE( Tr2PPDynamicExposureEffect );

const Be::ClassInfo* Tr2PPDynamicExposureEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPDynamicExposureEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "minBrightness", m_minBrightness, "Lower bound fraction of screen pixel brightness to calculate exposure. Allows ignoring some very dark pixels", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxBrightness", m_maxBrightness, "Upper bound fraction of screen pixel brightness to calculate exposure. Allows ignoring some very bright pixels", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "increaseSpeed", m_increaseSpeed, "Speed for increasing current exposure to match ideal exposure", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "decreaseSpeed", m_decreaseSpeed, "Speed for decreasing current exposure to match ideal exposure", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minLuminance", m_minLuminance, "Minimal absolute luminance to consider for dynamic exposure", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxLuminance", m_maxLuminance, "Maximal absolute luminance to consider for dynamic exposure", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "influence", m_influence, "Exposure influence \n:jessica-numeric-range: 0.0, 1.0\n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "middleValue", m_middleValue, "Middle gray brightness", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "adjustment", m_adjustment, "Exposure adjustment", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minExposure", m_minExposure, "Minimal exposure limit", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxExposure", m_maxExposure, "Maximal exposure limit", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "debug", m_debug, "Enable debugging for the exposure", Be::READWRITE )

	EXPOSURE_CHAINTO( Tr2PPEffect )
}