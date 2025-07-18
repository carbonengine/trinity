////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#pragma once

#include "PostProcess/Effects/Tr2PPEffect.h"

namespace Tr2ColorCorrection
{

extern const Be::VarChooser ColorCorrectionDebugChooser[];
}

BLUE_CLASS( Tr2PPColorCorrectionEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPColorCorrectionEffect( IRoot* lockobj = NULL );
	~Tr2PPColorCorrectionEffect();
	
	float m_whiteTemperature;
	float m_whiteTint;
	float m_colorSaturation;
	float m_colorContrast;
	float m_colorGamma;
	Vector3 m_colorGain;
	Vector3 m_colorOffset;
};

TYPEDEF_BLUECLASS( Tr2PPColorCorrectionEffect );
