// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPGenericEffect.h"

namespace PostProcessEnums
{
Be::VarChooser Tr2PostProcessExecutionSlotChooser[] = {

	{ "BEFORE_UPSCALING",
	  BeCast( Tr2PPGenericEffect::BEFORE_UPSCALING ),
	  "Render effect before upscaling" },
	{ "AFTER_TONEMAP",
	  BeCast( Tr2PPGenericEffect::AFTER_TONEMAP ),
	  "Render effect after tonemapping" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "Tr2PostProcessExecutionSlot", Tr2PPGenericEffect::ExecutionSlot, Tr2PostProcessExecutionSlotChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
}


BLUE_DEFINE( Tr2PPGenericEffect );


const Be::ClassInfo* Tr2PPGenericEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPGenericEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"quality", m_quality, "post process quality level and higher where this effect gets rendered", Be::READWRITE | Be::PERSIST | Be::ENUM, PostProcess::PostProcessQualityChooser )
		MAP_ATTRIBUTE(
			"effect",
			m_effect,
			"The effect to use. The Tr2PostProcessRenderer passes the pre upscaled source into the Blit texture parameter of this effect",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "executionSlot", m_executionSlot, "The execution slot for this effect", Be::READWRITE | Be::PERSIST | Be::ENUM, PostProcessEnums::Tr2PostProcessExecutionSlotChooser );
		MAP_ATTRIBUTE( "order", m_order, "Affects the order of rendering for this effect in relation to other generic effects", Be::READWRITE | Be::PERSIST );

	EXPOSURE_CHAINTO( Tr2PPEffect )
}
