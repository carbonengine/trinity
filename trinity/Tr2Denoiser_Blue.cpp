////////////////////////////////////////////////////////////
//
//    Created:   January 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "Tr2Denoiser.h"

BLUE_DEFINE( Tr2Denoiser );

const Be::ClassInfo* Tr2Denoiser::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Denoiser, "" )
		MAP_INTERFACE( Tr2Denoiser )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "noiseEstimate", m_noiseEstimate, "Noise estimate texture (for debugging purpuses)", Be::READ )
		MAP_ATTRIBUTE( "intermediate", m_intermediate, "Intermediate texture (for debugging purpuses)", Be::READ )
		MAP_ATTRIBUTE( "result", m_result, "Resulting denoised texture", Be::READ )

		MAP_ATTRIBUTE( "radius", m_radius, "Max blur radius in pixels", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "stepSize", m_stepSize, "Pixel step for blurring", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "depthWeight", m_depthWeight, "How much depth difference is taken into account", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "normalWeight", m_normalWeight, "How much normal difference is taken into account", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "planeWeight", m_planeWeight, "How much plane difference is taken into account", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "bypass", m_bypass, "Bypass denoising (for debugging purpuses)", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_END()
}
