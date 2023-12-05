////////////////////////////////////////////////////////////////////////////////
//
// Created:		October 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2XeSSUpscaling.h"
#include "ITr2Upscaling.h"

BLUE_DEFINE( Tr2XeSSUpscaling );

const Be::ClassInfo* Tr2XeSSUpscaling::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2XeSSUpscaling, "" )
		MAP_INTERFACE( Tr2XeSSUpscaling )
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
		MAP_ATTRIBUTE( "jitterXScale", m_jitterXScale, "x-jitter scale", Be::READWRITE )
		MAP_ATTRIBUTE( "jitterYScale", m_jitterYScale, "y-jitter scale", Be::READWRITE )
		MAP_ATTRIBUTE( "jitterOffsetXScale", m_jitterOffsetXScale, "x-jitter offset scale", Be::READWRITE )
		MAP_ATTRIBUTE( "jitterOffsetYScale", m_jitterOffsetYScale, "y-jitter offset scale", Be::READWRITE )
		MAP_ATTRIBUTE( "useReactive", m_useReactive, "use reactive mask", Be::READWRITE | Be::NOTIFY )
#endif
	EXPOSURE_END()


}
