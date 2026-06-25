// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPEffect.h"
namespace PostProcess
{
const Be::VarChooser PostProcessQualityChooser[] = {
	{ "Low", BeCast( Quality::LOW ), "Low Quality" },
	{ "Medium", BeCast( Quality::MEDIUM ), "Medium Quality" },
	{ "High", BeCast( Quality::HIGH ), "High Quality" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "PostProcessQuality", PostProcess::Quality, PostProcessQualityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

}

BLUE_DEFINE( Tr2PPEffect );

const Be::ClassInfo* Tr2PPEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPEffect, "" )

		MAP_ATTRIBUTE( "display", m_display, "Should this be rendered", Be::READWRITE | Be::NOTIFY )
		MAP_PROPERTY_READONLY(
			"active",
			IsActive,
			"" )

	EXPOSURE_END()
}
