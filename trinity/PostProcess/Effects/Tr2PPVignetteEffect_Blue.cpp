// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPVignetteEffect.h"

BLUE_DEFINE( Tr2PPVignetteEffect );

const Be::ClassInfo* Tr2PPVignetteEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPVignetteEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "shapePath", m_shapePath, "shape texture", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "detailPath", m_detailPath, "detail texture", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "detail1Size", m_detail1Size, "detail layer 1 size (in pixels)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "detail2Size", m_detail2Size, "detail layer 2 size (in pixels)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "detail1Scroll", m_detail1Scroll, "detail layer 1 scroll speed", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "detail2Scroll", m_detail2Scroll, "detail layer 2 scroll speed", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_color, "color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "opacity", m_opacity, "opacity", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "intensity", m_intensity, "additive intensity", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sineFrequency", m_sineFrequency, "sine wave frequency", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sineMinimum", m_sineMinimum, "sine wave minimum value", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sineMaximum", m_sineMaximum, "sine wave maximum value", Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( Tr2PPEffect )
}
