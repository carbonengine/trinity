////////////////////////////////////////////////////////////////////////////////
//
// Created:	January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PostProcess2.h"

BLUE_DEFINE( Tr2PostProcess2 );

const Be::ClassInfo* Tr2PostProcess2::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2PostProcess2, "" )

		MAP_INTERFACE( Tr2PostProcess2 )

		MAP_ATTRIBUTE( "signalLoss", m_signalLoss, "Accesses the Signal loss", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "godRays", m_godRays, "Accesses the God Rays effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "bloom", m_bloom, "Accesses the bloom effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "dynamicExposure", m_dynamicExposure, "Accesses the dynamic exposure effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "filmGrain", m_filmGrain, "Accesses the film grain effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "desaturate", m_desaturate, "Accesses the desaturate effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fade", m_fade, "Accesses the fade effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "luts", m_luts, "Accesses the LUT effect", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "lut", m_lut, "Accesses the LUT effect", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "vignette", m_vignette, "Accesses the vignette effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fog", m_fog, "Accesses the fog effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "depthOfField", m_depthOfField, "Accesses the dof effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "taa", m_taa, "Accesses the temporal antialiasing effect", Be::READWRITE )
		MAP_ATTRIBUTE( "tonemapping", m_tonemapping, "Accesses the tonemapping effect", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( 
			"whiteTemperature", 
			m_whiteTemperature, 
			"Color grading white temperature\n"
			":jessica-group: Color Correction\n"
			":jessica-numeric-range: (3000, 15000)", 
			Be::READWRITE )
		MAP_ATTRIBUTE( 
			"whiteTint", 
			m_whiteTint, 
			"Color grading white tint\n"
			":jessica-group: Color Correction\n"
			":jessica-numeric-range: (-1.0, 1.0)", 
			Be::READWRITE )
		MAP_ATTRIBUTE( 
			"colorSaturation", 
			m_colorSaturation, 
			"Color saturation\n"
			":jessica-group: Color Correction\n"
			":jessica-numeric-range: (0.0, 2.0)",
			Be::READWRITE )
		MAP_ATTRIBUTE( 
			"colorContrast", 
			m_colorContrast, 
			"Image contrast\n"
			":jessica-group: Color Correction\n"
			":jessica-numeric-range: (0.0, 2.0)",
			Be::READWRITE )
		MAP_ATTRIBUTE( 
			"colorGamma", 
			m_colorGamma, 
			"Image gamma\n"
			":jessica-group: Color Correction\n"
			":jessica-numeric-range: (0.0, 2.0)",
			Be::READWRITE )
		MAP_ATTRIBUTE( 
			"colorGain", 
			m_colorGain, 
			"Image color gain (scale)\n"
			":jessica-group: Color Correction\n"
			":jessica-numeric-range: (0.0, 2.0)",
			Be::READWRITE )
		MAP_ATTRIBUTE( 
			"colorOffset", 
			m_colorOffset, 
			"Image color offset\n"
			":jessica-group: Color Correction\n"
			":jessica-numeric-range: (0.0, 2.0)",
			Be::READWRITE )


    EXPOSURE_END()
}
