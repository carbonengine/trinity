// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPTonemappingEffect.h"

BLUE_DEFINE( Tr2PPTonemappingEffect );

const Be::VarChooser TonemappingEffectMethodChooser[] = {
	{ "Aces (new)", BeCast( Tr2PPTonemappingEffect::Aces ), "Aces tonemapper introduced in Frontier." },
	{ "AgX (new)", BeCast( Tr2PPTonemappingEffect::AgX ), "AgX tonemapper used in Frontier." },
	{ "Uncharted 2 (old)", BeCast( Tr2PPTonemappingEffect::Uncharted2 ), "Uncharted 2 tonemapper used in Eve" },
	{ 0 }
};

const Be::ClassInfo* Tr2PPTonemappingEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPTonemappingEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE_WITH_CHOOSER( "method", m_method, "Which tonemapper should be used?", Be::READWRITE | Be::PERSIST | Be::ENUM, TonemappingEffectMethodChooser );

		MAP_ATTRIBUTE( "slope", m_aces.m_slope, ":jessica-group: Aces (new) \n"
												":jessica-numeric-range: (0.0, 3.0)",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "toe", m_aces.m_toe, ":jessica-group: Aces (new) \n"
											":jessica-numeric-range: (0.0, 1.0)",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shoulder", m_aces.m_shoulder, ":jessica-group: Aces (new) \n"
													  ":jessica-numeric-range: (0.0, 1.0)",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blackClip", m_aces.m_blackClip, ":jessica-group: Aces (new) \n"
														":jessica-numeric-range: (0.0, 1.0)",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "whiteClip", m_aces.m_whiteClip, ":jessica-group: Aces (new) \n"
														":jessica-numeric-range: (0.0, 1.0)",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scale", m_aces.m_scale, ":jessica-group: Aces (new) \n"
												":jessica-numeric-range: (0.0, 3.0)",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blueCorrection", m_aces.m_blueCorrection, ":jessica-group: Aces (new) \n"
																  ":jessica-numeric-range: (0.0, 1.0)",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useSweeteners", m_aces.m_useSweeteners, ":jessica-group: Aces (new) \n", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "shoulderStrength", m_uncharted2.m_shoulderStrength, ":jessica-group: Uncharted 2 (old) \n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "linearStrength", m_uncharted2.m_linearStrength, ":jessica-group: Uncharted 2 (old) \n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "linearAngle", m_uncharted2.m_linearAngle, ":jessica-group: Uncharted 2 (old) \n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "toeStrength", m_uncharted2.m_toeStrength, ":jessica-group: Uncharted 2 (old) \n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "toeNumerator", m_uncharted2.m_toeNumerator, ":jessica-group: Uncharted 2 (old) \n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "toeDenominator", m_uncharted2.m_toeDenominator, ":jessica-group: Uncharted 2 (old) \n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "whiteScale", m_uncharted2.m_whiteScale, ":jessica-group: Uncharted 2 (old) \n", Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( Tr2PPEffect )
}