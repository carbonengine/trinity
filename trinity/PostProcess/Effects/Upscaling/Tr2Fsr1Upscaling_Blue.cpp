////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2Fsr1Upscaling.h"

BLUE_DEFINE( Tr2Fsr1Upscaling );

const Be::ClassInfo* Tr2Fsr1Upscaling::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Fsr1Upscaling, "" )
		MAP_INTERFACE( Tr2Fsr1Upscaling )
		MAP_INTERFACE( ITr2Upscaling )
		MAP_ATTRIBUTE( "fidelityFXFsrEASUShader", m_fidelityFXFsrEASUShader, "The Fidelity EASU Shader", Be::READWRITE )
		MAP_ATTRIBUTE( "fidelityFXFsrRCASShader", m_fidelityFXFsrRCASShader, "The Fidelity RCAS Shader", Be::READWRITE )

		MAP_PROPERTY_READONLY( "mipLevelBias", GetMipLevelBias, "The mip level bias" )
		MAP_PROPERTY_READONLY( "upscalingType", GetUpscalingType, "The upscaling type" )
		MAP_PROPERTY_READONLY( "isApplicable", IsApplicable, "Is it applicable" ) 
		MAP_METHOD_AND_WRAP( "GetAvailableSettings", GetAvailableSettings, "Returns all applicable settings" )
		MAP_PROPERTY_READONLY( "supportsFrameGeneration", SupportsFrameGeneration, "Does it have frame generation\n:jessica-group: Frame Generation" )

	EXPOSURE_END()
}
