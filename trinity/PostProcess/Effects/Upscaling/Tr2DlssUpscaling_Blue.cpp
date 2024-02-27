////////////////////////////////////////////////////////////////////////////////
//
// Created:		May 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2DlssUpscaling.h"

BLUE_DEFINE( Tr2DlssUpscaling );

const Be::ClassInfo* Tr2DlssUpscaling::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2DlssUpscaling, "" )
		MAP_INTERFACE( Tr2DlssUpscaling )
		MAP_INTERFACE( ITr2Upscaling )
		MAP_INTERFACE( INotify )

		MAP_PROPERTY_READONLY( "mipLevelBias", GetMipLevelBias, "The mip level bias" )
		MAP_PROPERTY_READONLY( "upscalingType", GetUpscalingType, "The upscaling type" )
		MAP_PROPERTY_READONLY( "isApplicable", IsApplicable, "Is it applicable" )

		MAP_ATTRIBUTE( "useSharpening", m_useSharpening, ":jessica-group: Sharpening", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "sharpeningAmount", m_sharpeningAmount, ":jessica-group: Sharpening", Be::READWRITE | Be::NOTIFY )

		MAP_METHOD_AND_WRAP( "GetAvailableSettings", GetAvailableSettings, "Returns all applicable settings" )

		MAP_ATTRIBUTE( "jitterX", m_jitterX, "shows the jittering in x direction", Be::READ )
		MAP_ATTRIBUTE( "jitterY", m_jitterY, "shows the jittering in y direction", Be::READ )

		MAP_PROPERTY_READONLY( "supportsFrameGeneration", SupportsFrameGeneration, "Does it have frame generation\n:jessica-group: Frame Generation" )
		MAP_PROPERTY( "useFrameGeneration", GetUseFrameGeneration, SetUseFrameGeneration, ":jessica-group: Frame Generation" )
		MAP_ATTRIBUTE( "actualFrames", m_actualFrames, "Shows the actual frames generated \n:jessica-group: Frame Generation", Be::READ )
		MAP_ATTRIBUTE( "vramUsage", m_vramUsage, "Shows the vram usage\n:jessica-group: Frame Generation", Be::READ )
		MAP_ATTRIBUTE( "minWidthHeight", m_minWidthHeight, "Shows the min width/height needed for dlssg to work\n:jessica-group: Frame Generation", Be::READ )

	EXPOSURE_END()
}