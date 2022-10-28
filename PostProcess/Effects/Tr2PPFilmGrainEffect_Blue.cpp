////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPFilmGrainEffect.h"

BLUE_DEFINE( Tr2PPFilmGrainEffect );

const Be::ClassInfo* Tr2PPFilmGrainEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPFilmGrainEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "useNewTechnique", m_useNewTechnique, "Enables the new, optimized film grain technique \n:jessica-group: Common settings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "compare", m_compare, "Compare the old and new implementations \n:jessica-group: Common settings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "colored", m_colored, "If film grain is grayscale or colored \n:jessica-group: Common settings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "colorAmount", m_colorAmount, "Color amount in film grain if 'colored' is enabled \n:jessica-group: Common settings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "grainSize", m_oldGrainSize, "Grain size in pixels. \n:jessica-group: Old technique settings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "intensity", m_oldIntensity, "Film grain intensity. \n:jessica-group: Old technique settings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "luminanceExponent", m_oldLuminanceExponent, "Grain intensity falloff with brightness \n:jessica-group: Old technique settings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "newGrainSize", m_newGrainSize, "Grain size in pixels. \n:jessica-group: New technique settings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "newIntensity", m_newIntensity, "Film grain intensity. \n:jessica-group: New technique settings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "newGrainDensity", m_newGrainDensity, "Density of grains. Lower values produces well-spaced distinct grains, while higher values produce a more even noise. \n:jessica-group: New technique settings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "newGrainContrast", m_newGrainContrast, "Contrast of grains, a higher value produces sharper grain edges. \n:jessica-group: New technique settings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "newLuminanceSensitivity", m_newLuminanceSensitivity, "Luminance sensitivity. The higher the value, the more the noise is reduced for bright pixels. 0.0 disables the reduction completely. \n:jessica-group: New technique settings", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		EXPOSURE_CHAINTO( Tr2PPEffect )
}
