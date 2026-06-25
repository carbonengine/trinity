// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPFilmGrainEffect.h"

BLUE_DEFINE( Tr2PPFilmGrainEffect );

const Be::ClassInfo* Tr2PPFilmGrainEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPFilmGrainEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "colored", m_colored, "If film grain is grayscale or colored \n:jessica-group: Common settings", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "colorAmount", m_colorAmount, "Color amount in film grain if 'colored' is enabled \n:jessica-group: Common settings", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "grainSize", m_grainSize, "Grain size in pixels. \n:jessica-group: New technique settings", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "intensity", m_intensity, "Film grain intensity. \n:jessica-group: New technique settings", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "grainDensity", m_grainDensity, "Density of grains. Lower values produces well-spaced distinct grains, while higher values produce a more even noise. \n:jessica-group: New technique settings", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "grainContrast", m_grainContrast, "Contrast of grains, a higher value produces sharper grain edges. \n:jessica-group: New technique settings", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "brightnessModifier", m_brightnessModifier, "How the brightness of the pixel affects the film grain. Negative values reduce the noise intensity for bright pixels, while positive values increase the noise for bright pixels. \n:jessica-group: New technique settings", Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( Tr2PPEffect )
}
