// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightAttributeModifierNoise.h"

BLUE_DEFINE( EveSmartLightAttributeModifierNoise );

const Be::ClassInfo* EveSmartLightAttributeModifierNoise::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightAttributeModifierNoise, ":jessica-icon: poo-storm\n:jessica-icon-color: (255, 115, 35)\n" )
		MAP_INTERFACE( EveSmartLightAttributeModifierNoise )
		MAP_INTERFACE( IEveSmartLightGroupAttributeModifier )

		MAP_ATTRIBUTE( "noiseAmplitude", m_noiseAmplitude, "Brightness noise amplitude", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseFrequency", m_noiseFrequency, "Brightness noise frequency", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseOctaves", m_noiseOctaves, "Brightness turbulence octaves", Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( EveSmartLightBaseAttributeModifier )
}
