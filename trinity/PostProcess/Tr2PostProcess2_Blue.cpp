// Copyright © 2019 CCP ehf.

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
		MAP_ATTRIBUTE( "lut", m_lut, "Accesses the LUT effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "vignette", m_vignette, "Accesses the vignette effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fog", m_fog, "Accesses the fog effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "depthOfField", m_depthOfField, "Accesses the dof effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "taa", m_taa, "Accesses the temporal antialiasing effect", Be::READWRITE )
		MAP_ATTRIBUTE( "tonemapping", m_tonemapping, "Accesses the tonemapping effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "colorCorrection", m_colorCorrection, "Accesses the color correction effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "genericEffect", m_generic, "Accesses the generic effect", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
