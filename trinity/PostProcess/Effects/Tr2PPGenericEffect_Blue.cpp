// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPGenericEffect.h"

BLUE_DEFINE( Tr2PPGenericEffect );


const Be::ClassInfo* Tr2PPGenericEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPGenericEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"quality", m_quality, "post process quality level and higher where this effect gets rendered", Be::READWRITE | Be::PERSIST | Be::ENUM, PostProcess::PostProcessQualityChooser )
		MAP_ATTRIBUTE(
			"effect",
			m_effect,
			"The effect to use. The Tr2PostProcessRenderer passes the pre upscaled source into the Blit texture parameter of this effect",
			Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( Tr2PPEffect )
}
