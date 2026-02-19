////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2025
// Copyright:	CCP 2025
//
////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Tr2PPGenericEffect.h"

BLUE_DEFINE( Tr2PPGenericEffect );


const Be::ClassInfo* Tr2PPGenericEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPGenericEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"quality", m_quality,
			"post process quality level and higher where this effect gets rendered",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM,
			PostProcess::PostProcessQualityChooser
		)
		MAP_ATTRIBUTE(
			"shaderPath",
			m_shaderPath,
			"Path to the shader file, remember to add a texture parameter Blit so the effect can feed it into the shader",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)
		MAP_ATTRIBUTE(
			"effect",
			m_effect,
			"The effect to use, generated from the shaderPath",
			Be::READ | Be::PERSIST
		)

		EXPOSURE_CHAINTO( Tr2PPEffect )
}
