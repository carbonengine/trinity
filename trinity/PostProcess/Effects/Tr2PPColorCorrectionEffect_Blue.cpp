// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPColorCorrectionEffect.h"

BLUE_DEFINE( Tr2PPColorCorrectionEffect );

const Be::ClassInfo* Tr2PPColorCorrectionEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPColorCorrectionEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "whiteTemperature", m_whiteTemperature, "\n:jessica-numeric-range: (3000, 15000)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "whiteTint", m_whiteTint, "\n:jessica-numeric-range: (-1.0, 1.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "colorSaturation", m_colorSaturation, "\n:jessica-numeric-range: (0.0, 2.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "colorContrast", m_colorContrast, "\n:jessica-numeric-range: (0.0, 2.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "colorGamma", m_colorGamma, "\n:jessica-numeric-range: (0.0, 2.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "colorGain", m_colorGain, "\n:jessica-numeric-range: (0.0, 2.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "colorOffset", m_colorOffset, "\n:jessica-numeric-range: (0.0, 2.0)", Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( Tr2PPEffect )
}
