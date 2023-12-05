////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2NoopUpscaling.h"
#include "ITr2Upscaling.h"

BLUE_DEFINE( Tr2NoopUpscaling );

// Small hack to register the chooser so it can be used in other upscaling effects
namespace Tr2Upscaling
{
	Be::VarChooser UpscalingTypeChooser[] = {
		{ "Spatial", BeCast( UT_SPATIAL ), "Spatial" },
		{ "Temporal", BeCast( UT_TEMPORAL ), "Temporal" },
		{ "Unknown", BeCast( UT_UNKNOWN ), "Unknown" },
		{ "N/A", BeCast( UT_NOT_APPLICABLE ), "N/A" },
		{ 0 }
	};

	BLUE_REGISTER_ENUM_EX( "UpscalingType", Tr2Upscaling::UpscalingType, UpscalingTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
}

const Be::ClassInfo* Tr2NoopUpscaling::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2NoopUpscaling, "" )
		MAP_INTERFACE( Tr2NoopUpscaling )
		MAP_INTERFACE( ITr2Upscaling )
		
		MAP_PROPERTY_READONLY( "mipLevelBias", GetMipLevelBias, "The mip level bias" )
		MAP_PROPERTY_READONLY( "upscalingType", GetUpscalingType, "The upscaling type" )
		MAP_PROPERTY_READONLY( "isApplicable", IsApplicable, "Is it applicable" )
		MAP_METHOD_AND_WRAP( "GetAvailableSettings", GetAvailableSettings, "Returns all applicable settings" )
		MAP_PROPERTY_READONLY( "supportsFrameGeneration", SupportsFrameGeneration, "Does it have frame generation\n:jessica-group: Frame Generation" )


	EXPOSURE_END()
}
