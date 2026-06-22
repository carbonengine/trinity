// Copyright © 2024 CCP ehf.

#pragma once
#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_CLASS( Tr2PPTonemappingEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPTonemappingEffect( IRoot* lockobj = NULL );
	~Tr2PPTonemappingEffect();

	struct
	{
		float m_shoulderStrength;
		float m_linearStrength;
		float m_linearAngle;
		float m_toeStrength;
		float m_toeNumerator;
		float m_toeDenominator;
		float m_whiteScale;
	} m_uncharted2;

	struct
	{
		float m_slope;
		float m_toe;
		float m_shoulder;
		float m_blackClip;
		float m_whiteClip;
		float m_scale;
		float m_blueCorrection;
		bool m_useSweeteners;
	} m_aces;

	enum
	{
		Uncharted2,
		Aces,
		AgX,
	} m_method;
};

TYPEDEF_BLUECLASS( Tr2PPTonemappingEffect );