////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2Fsr2Upscaling.h"
#include "ITr2Upscaling.h"

BLUE_DEFINE( Tr2Fsr2Upscaling );

const Be::ClassInfo* Tr2Fsr2Upscaling::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Fsr2Upscaling, "" )
		MAP_INTERFACE( Tr2Fsr2Upscaling )
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

		MAP_ATTRIBUTE( "generateReactiveMap", m_generateReactiveMap, ":jessica-group: Reactive Mask", Be::READWRITE )
		MAP_ATTRIBUTE( "autoReactiveScale", m_autoReactiveScale, ":jessica-group: Reactive Mask", Be::READWRITE )
		MAP_ATTRIBUTE( "autoReactiveUseThreshold", m_autoReactiveUseThreshold, ":jessica-group: Reactive Mask", Be::READWRITE )
		MAP_ATTRIBUTE( "autoReactiveThreshold", m_autoReactiveThreshold, ":jessica-group: Reactive Mask", Be::READWRITE )
		MAP_ATTRIBUTE( "autoReactiveBinaryValue", m_autoReactiveBinaryValue, ":jessica-group: Reactive Mask", Be::READWRITE )
		MAP_ATTRIBUTE( "autoReactiveUseInverseTonemap", m_autoReactiveUseInverseTonemap, ":jessica-group: Reactive Mask", Be::READWRITE )
		MAP_ATTRIBUTE( "autoReactiveUseTonemap", m_autoReactiveUseTonemap, ":jessica-group: Reactive Mask", Be::READWRITE )
		MAP_ATTRIBUTE( "autoReactiveUseMax", m_autoReactiveUseMax, ":jessica-group: Reactive Mask", Be::READWRITE )
		MAP_ATTRIBUTE( "preExposure", m_preexposure, "", Be::READWRITE )
		MAP_ATTRIBUTE( "sharpness", m_sharpness, "", Be::READWRITE )

		MAP_ATTRIBUTE( "useHDR", m_useHDR, "", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "cancelMotionVectorJittering", m_cancelMotionVectorJittering, "", Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE( "generateTransparencyMap", m_generateTransparencyMap, "", Be::READWRITE )
#endif
	EXPOSURE_END()


}
