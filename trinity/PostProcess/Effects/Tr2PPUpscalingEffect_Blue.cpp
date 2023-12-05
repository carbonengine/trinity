////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//

#include "StdAfx.h"
#include "Tr2PPUpscalingEffect.h"

Be::VarChooser UpscalingTechniqueChooser[] = {
	{ "NONE",
	  BeCast( Tr2Upscaling::UPSCALING_TECHNIQUE_NONE ),
	  "No upscaling effect" },
	{ "FSR1",
	  BeCast( Tr2Upscaling::UPSCALING_TECHNIQUE_FSR1 ),
	  "FidelityFX Super Resolution 1" },
	{ "FSR2",
	  BeCast( Tr2Upscaling::UPSCALING_TECHNIQUE_FSR2 ),
	  "FidelityFX Super Resolution 2" },
    { "METALFX_SPATIAL",
      BeCast( Tr2Upscaling::UPSCALING_TECHNIQUE_METALFX_SPATIAL ),
      "MetalFX Spatial Upscaling" },
    { "METALFX_TEMPORAL",
      BeCast( Tr2Upscaling::UPSCALING_TECHNIQUE_METALFX_TEMPORAL ),
      "MetalFX Temporal Upscaling" },
	{ "DLSS",
	  BeCast( Tr2Upscaling::UPSCALING_TECHNIQUE_DLSS ),
	  "Nvidia Deep Learning Super Sampling" },
	{ "XESS",
	  BeCast( Tr2Upscaling::UPSCALING_TECHNIQUE_XESS ),
	  "Intel Xe Super Sampling" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( "Tr2UpscalingTechnique", Tr2Upscaling::Technique, UpscalingTechniqueChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

Be::VarChooser SharpeningTechniqueChooser[] = {
	{ "NONE",
	  BeCast( Tr2Sharpening::SHARPENING_TECHNIQUE_NONE ),
	  "No sharpening effect" },
	{ "CAS",
	  BeCast( Tr2Sharpening::SHARPENING_TECHNIQUE_CAS ),
	  "FidelityFX Contrast Adaptive Sharpening" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( "Tr2SharpeningTechnique", Tr2Sharpening::Technique, SharpeningTechniqueChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );


Be::VarChooser UpscalingSettingChooser[] = {
	{ "ULTRA_PERFORMANCE",
	  BeCast( Tr2Upscaling::ULTRA_PERFORMANCE ),
	  "Ultra Performance" },
	{ "PERFORMANCE",
	  BeCast( Tr2Upscaling::PERFORMANCE ),
	  "Performance" },
	{ "BALANCED",
	  BeCast( Tr2Upscaling::BALANCED ),
	  "Balanced" },
	{ "QUALITY",
	  BeCast( Tr2Upscaling::QUALITY ),
	  "Quality" },
	{ "ULTRA_QUALITY",
	  BeCast( Tr2Upscaling::ULTRA_QUALITY ),
	  "Ultra Quality" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( "Tr2UpscalingSetting", Tr2Upscaling::Setting, UpscalingSettingChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE_INTERFACE( ITr2Upscaling );
BLUE_DEFINE_INTERFACE( ITr2Sharpening );

BLUE_DEFINE( Tr2PPUpscalingEffect );

const Be::ClassInfo* Tr2PPUpscalingEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPUpscalingEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"upscalingTechnique",
			m_currentUpscalingTechnique,
			"",
			Be::READ | Be::ENUM,
			UpscalingTechniqueChooser )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"sharpeningTechnique",
			m_currentSharpeningTechnique,
			"",
			Be::READ | Be::ENUM,
			SharpeningTechniqueChooser )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"setting",
			m_currentSetting,
			"",
			Be::READ | Be::ENUM,
			UpscalingSettingChooser )

		MAP_PROPERTY_READONLY(
			"upscalingAmount",
			GetUpscalingAmount,
			"" )

			MAP_PROPERTY_READONLY(
			"mipLodBias",
			GetMipLodBias,
			"" )

		MAP_ATTRIBUTE(
			"sharpeningEffect",
			m_sharpeningEffect,
			"The Sharpening Effect",
			Be::READ )

		MAP_ATTRIBUTE(
			"upscalingEffect",
			m_upscalingEffect,
			"The Upscaling Effect",
			Be::READ )

		MAP_METHOD_AND_WRAP(
			"SetUpscalingTechnique",
			SetUpscalingTechnique,
			"" 
		)

		MAP_METHOD_AND_WRAP(
			"ApplySetting",
			ApplySetting,
			"" )
	
		MAP_ATTRIBUTE( "renderWidth", m_renderWidth, "The actual width of rendering", Be::READ)
		MAP_ATTRIBUTE( "renderHeight", m_renderHeight, "The actual height of rendering", Be::READ)
		MAP_ATTRIBUTE( "displayWidth", m_displayWidth, "The actual width of the display", Be::READ)
		MAP_ATTRIBUTE( "displayHeight", m_displayHeight, "The actual height of the display", Be::READ)

		MAP_ATTRIBUTE( "debugRenderSize", m_debugRenderSize, "Shows how big the rendering is", Be::READWRITE )

		EXPOSURE_CHAINTO( Tr2PPEffect )
}
