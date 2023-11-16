////////////////////////////////////////////////////////////////////////////////
//
// Created:   March 2022
// Copyright: CCP 2022
//

#include "StdAfx.h"
#include "Tr2SSAO.h"

BLUE_DEFINE( Tr2SSAO );

const Be::VarChooser SSAOQualityChooser[] = {
	{ "Lowest",  BeCast( SSAOQuality::LOWEST  ), "Lowest quality"             },
	{ "Low",     BeCast( SSAOQuality::LOW     ), "Low quality"                },
	{ "Medium",  BeCast( SSAOQuality::MEDIUM  ), "Medium quality"             },
	{ "High",    BeCast( SSAOQuality::HIGH    ), "High quality"               },
	{ "Highest", BeCast( SSAOQuality::HIGHEST ), "Highest (adaptive) quality" },
	{}
};

BLUE_REGISTER_ENUM_EX( "SSAOQuality", SSAOQuality, SSAOQualityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Tr2SSAO::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SSAO, "" )
		MAP_INTERFACE( Tr2SSAO )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "enabled", m_detail.enabled, "If the detail layer of SSAO is enabled\n:jessica-group: Settings", Be::READWRITE )
		MAP_ATTRIBUTE_WITH_CHOOSER( "quality", m_detail.quality, "Quality for the detail layer of SSAO\n:jessica-group: Settings", Be::READWRITE | Be::NOTIFY | Be::ENUM, SSAOQualityChooser )
		MAP_ATTRIBUTE( "downsampled", m_detail.downsampled, "Use lower resolution SSAO: low quality, but fast\n:jessica-group: Settings", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "zoomLevel", m_detail.zoomLevel, "Defines the desired detail size\n:jessica-group: Settings", Be::READWRITE )
		MAP_ATTRIBUTE( "radius", m_detail.settings.radius, "World (view) space size of the occlusion sphere. WARNING! LARGER VALUES AFFECTS PERFORMANCE!\n:jessica-group: Settings", Be::READWRITE )
		MAP_ATTRIBUTE( "shadowMultiplier", m_detail.settings.shadowMultiplier, "Effect strength linear multiplier\n:jessica-group: Settings", Be::READWRITE )
		MAP_ATTRIBUTE( "shadowPower", m_detail.settings.shadowPower, "Effect strength contrast\n:jessica-group: Settings", Be::READWRITE )
		MAP_ATTRIBUTE( "shadowClamp", m_detail.settings.shadowClamp, "Effect max limit (applied after multiplier but before blur)\n:jessica-group: Settings", Be::READWRITE )
		MAP_ATTRIBUTE( "sharpness", m_detail.settings.sharpness, "How much to bleed over edges (1: not at all, 0.5: half-half; 0.0: completely ignore edges).\n:jessica-group: Settings", Be::READWRITE )

		MAP_ATTRIBUTE( "deinterleavedDepthTarget", m_detail.resources.deinterleavedDepthTarget, "", Be::READ )
		MAP_ATTRIBUTE( "deinterleavedNormalTarget", m_detail.resources.deinterleavedNormalTarget, "", Be::READ )
		MAP_ATTRIBUTE( "importanceTargetA", m_detail.resources.importanceTargetA, "", Be::READ )
		MAP_ATTRIBUTE( "importanceTargetB", m_detail.resources.importanceTargetB, "", Be::READ )
		MAP_ATTRIBUTE( "ssaoWorkerTargetA", m_detail.resources.ssaoWorkerTargetA, "", Be::READ )
		MAP_ATTRIBUTE( "ssaoWorkerTargetB", m_detail.resources.ssaoWorkerTargetB, "", Be::READ )
		MAP_ATTRIBUTE( "outputTarget", m_outputTarget, "Final SSAO output", Be::READ )
	EXPOSURE_END()
}
