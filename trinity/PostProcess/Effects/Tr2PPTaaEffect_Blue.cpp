// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPTaaEffect.h"

BLUE_DEFINE( Tr2PPTaaEffect );


const Be::VarChooser Tr2PPTaaEffectQualityChooser[] = {
	{ "Low", BeCast( Tr2PPTaaEffect::Quality::TAA_LOW ), "Low Quality" },
	{ "Medium", BeCast( Tr2PPTaaEffect::Quality::TAA_MEDIUM ), "Medium Quality" },
	{ "High", BeCast( Tr2PPTaaEffect::Quality::TAA_HIGH ), "High Quality" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "TaaQuality", Tr2PPTaaEffect::Quality, Tr2PPTaaEffectQualityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::VarChooser Tr2PPTaaEffectDebugChooser[] = {
	{ "Off", BeCast( Tr2PPTaaEffect::Debug::TAA_DEBUG_OFF ), "Debug Off" },
	{ "Motion Vectors", BeCast( Tr2PPTaaEffect::Debug::TAA_DEBUG_MOTION_VECTORS ), "Show Motion Vectors" },
	{ "Early Out Mask", BeCast( Tr2PPTaaEffect::Debug::TAA_DEBUG_EARLY_OUT_MASK ), "Show Early Out Mask" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "TaaDebug", Tr2PPTaaEffect::Debug, Tr2PPTaaEffectDebugChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Tr2PPTaaEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPTaaEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE_WITH_CHOOSER( "quality", m_quality, "Taa Quality", Be::READWRITE | Be::ENUM, Tr2PPTaaEffectQualityChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "debug", m_debugMode, "Taa Debug", Be::READWRITE | Be::ENUM, Tr2PPTaaEffectDebugChooser )

		MAP_ATTRIBUTE( "earlyOutThreshold", m_earlyOutThreshold, "Controls the threshold used to skip calculations when a larger area is close to a flat color", Be::READWRITE )

	EXPOSURE_CHAINTO( Tr2PPEffect )
}
