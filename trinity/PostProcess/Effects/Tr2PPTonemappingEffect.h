////////////////////////////////////////////////////////////////////////////////
//
// Created:		May 2024
// Copyright:	CCP 2024
//

#pragma once
#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_CLASS( Tr2PPTonemappingEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPTonemappingEffect( IRoot* lockobj = NULL );
	~Tr2PPTonemappingEffect();

	float m_shoulderStrength;
	float m_linearStrength;
	float m_linearAngle;
	float m_toeStrength;
	float m_toeNumerator;
	float m_toeDenominator;
	float m_whiteScale;
};

TYPEDEF_BLUECLASS( Tr2PPTonemappingEffect );