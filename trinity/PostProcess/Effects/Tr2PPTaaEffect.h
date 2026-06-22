// Copyright © 2019 CCP ehf.

#pragma once
#ifndef Tr2PPTaaEffect_H
#define Tr2PPTaaEffect_H

#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_CLASS( Tr2PPTaaEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	enum Quality
	{
		TAA_LOW = 1,
		TAA_MEDIUM = 2,
		TAA_HIGH = 3
	};

	enum Debug
	{
		TAA_DEBUG_OFF,
		TAA_DEBUG_MOTION_VECTORS,
		TAA_DEBUG_EARLY_OUT_MASK
	};

	Tr2PPTaaEffect( IRoot* lockobj = NULL );
	~Tr2PPTaaEffect();

	void IncrementJitter();

	bool IsActive() override
	{
		return m_display;
	}

	Quality m_quality;
	Debug m_debugMode;
	float m_earlyOutThreshold;
};

TYPEDEF_BLUECLASS( Tr2PPTaaEffect );

#endif