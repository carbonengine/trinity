////////////////////////////////////////////////////////////////////////////////
//
// Created:        May 2023
// Copyright:      CCP 2023
//

#include "StdAfx.h"
#include "ITr2Upscaling.h"
#include "Tr2MetalFxUpscaling.h"

BLUE_DEFINE( Tr2MetalFxSpatialUpscaling );
BLUE_DEFINE( Tr2MetalFxTemporalUpscaling );

const Be::ClassInfo* Tr2MetalFxSpatialUpscaling::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2MetalFxSpatialUpscaling, "" )
        MAP_INTERFACE( Tr2MetalFxSpatialUpscaling )
        MAP_INTERFACE( ITr2Upscaling )
        MAP_PROPERTY_READONLY( "mipLevelBias", GetMipLevelBias, "The mip level bias" )
        MAP_PROPERTY_READONLY( "upscalingType", GetUpscalingType, "The upscaling type" )
        MAP_PROPERTY_READONLY( "isApplicable", IsApplicable, "Is it applicable" )
        MAP_METHOD_AND_WRAP( "GetAvailableSettings", GetAvailableSettings, "Returns all applicable settings" )
        MAP_PROPERTY_READONLY( "supportsFrameGeneration", SupportsFrameGeneration, "Does it have frame generation\n:jessica-group: Frame Generation" )

    EXPOSURE_END()
}

const Be::ClassInfo* Tr2MetalFxTemporalUpscaling::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2MetalFxTemporalUpscaling, "" )
        MAP_INTERFACE( Tr2MetalFxTemporalUpscaling )
        MAP_INTERFACE( ITr2Upscaling )
        MAP_PROPERTY_READONLY( "mipLevelBias", GetMipLevelBias, "The mip level bias" )
        MAP_PROPERTY_READONLY( "upscalingType", GetUpscalingType, "The upscaling type" )
        MAP_PROPERTY_READONLY( "isApplicable", IsApplicable, "Is it applicable" )
        MAP_METHOD_AND_WRAP( "GetAvailableSettings", GetAvailableSettings, "Returns all applicable settings" )
        MAP_PROPERTY_READONLY( "supportsFrameGeneration", SupportsFrameGeneration, "Does it have frame generation\n:jessica-group: Frame Generation" )

#if TRINITY_PLATFORM == TRINITY_METAL
        MAP_ATTRIBUTE("jitterX", m_jitterX, "shows the jittering in x direction", Be::READ)
        MAP_ATTRIBUTE("jitterY", m_jitterX, "shows the jittering in y direction", Be::READ)
#endif
    EXPOSURE_END()
}
