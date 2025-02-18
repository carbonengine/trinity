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

	float m_slope;
	float m_toe;
	float m_shoulder;
	float m_blackClip;
	float m_whiteClip;
	float m_scale;
	float m_blueCorrection;
	bool m_useSweeteners;
};

TYPEDEF_BLUECLASS( Tr2PPTonemappingEffect );