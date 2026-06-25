// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPBloomEffect.h"

BLUE_DEFINE( Tr2PPBloomEffect );

const Be::ClassInfo* Tr2PPBloomEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPBloomEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "luminanceThreshold", m_luminanceThreshold, "The threshold of the luminance", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "luminanceScale", m_luminanceScale, "The scale of the luminance", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "brightness", m_bloomBrightness, "The bloom brightness", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "exposureDependency", m_exposureDependency, "The exposure dependency", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "grimeWeight", m_grimeWeight, "The grime weight", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "grimePath", m_grimePath, "The grime path", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "steps", m_steps, "how many steps", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "sizeScale", m_sizeScale, "How much of the screen is taken up of bloom", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "directionalWeight", m_directionalWeight, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "step1Size", m_stepSizes[0], "step1Size", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "step2Size", m_stepSizes[1], "step2Size", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "step3Size", m_stepSizes[2], "step3Size", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "step4Size", m_stepSizes[3], "step4Size", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "step5Size", m_stepSizes[4], "step5Size", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "step6Size", m_stepSizes[5], "step6Size", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "step1Tint", m_stepTints[0], "step1Tint", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "step2Tint", m_stepTints[1], "step2Tint", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "step3Tint", m_stepTints[2], "step3Tint", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "step4Tint", m_stepTints[3], "step4Tint", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "step5Tint", m_stepTints[4], "step5Tint", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "step6Tint", m_stepTints[5], "step6Tint", Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( Tr2PPEffect )
}
