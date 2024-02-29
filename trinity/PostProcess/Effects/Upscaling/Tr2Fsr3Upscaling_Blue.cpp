////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2Fsr3Upscaling.h"
#include "ITr2Upscaling.h"

BLUE_DEFINE( Tr2Fsr3Upscaling );

const Be::ClassInfo* Tr2Fsr3Upscaling::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Fsr3Upscaling, "" )
		MAP_INTERFACE( Tr2Fsr3Upscaling )
		MAP_INTERFACE( ITr2Upscaling )

		MAP_PROPERTY_READONLY( "mipLevelBias", GetMipLevelBias, "The mip level bias" )
		MAP_PROPERTY_READONLY( "upscalingType", GetUpscalingType, "The upscaling type" )
		MAP_PROPERTY_READONLY( "isApplicable", IsApplicable, "Is it applicable" )
		MAP_METHOD_AND_WRAP( "GetAvailableSettings", GetAvailableSettings, "Returns all applicable settings" )
		MAP_PROPERTY_READONLY( "supportsFrameGeneration", SupportsFrameGeneration, "Does it have frame generation\n:jessica-group: Frame Generation" )


#if TRINITY_PLATFORM == TRINITY_DIRECTX12
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "jitterX", m_jitterX, "x-jitter", Be::READ )
		MAP_ATTRIBUTE( "jitterY", m_jitterY, "y-jitter", Be::READ )
		MAP_ATTRIBUTE( "jitterXScale", m_jitterXScale, "y-jitter scale", Be::READWRITE )
		MAP_ATTRIBUTE( "jitterYScale", m_jitterYScale, "y-jitter scale", Be::READWRITE )

		MAP_ATTRIBUTE( "preExposure", m_preexposure, "", Be::READWRITE )
		MAP_ATTRIBUTE( "sharpness", m_sharpness, "", Be::READWRITE )

		MAP_ATTRIBUTE( "useHDR", m_useHDR, "", Be::READWRITE | Be::NOTIFY )
#endif
	EXPOSURE_END()


}
