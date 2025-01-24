////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

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

	Tr2PPTaaEffect( IRoot* lockobj = NULL );
	~Tr2PPTaaEffect();

	void IncrementJitter();
	void GetJitter( uint32_t renderWidth, uint32_t renderHeight, float& x, float& y );

	bool IsActive() override
	{
		return m_display;
	}

	int m_quality;
	bool m_showMotionVectors;
	bool m_showEarlyOutMask;
	float m_earlyOutThreshold;
private:
	Vector2 m_samplingPatterns[4];
	int m_samplingIndex;
	float m_jitterX;
	float m_jitterY;
};

TYPEDEF_BLUECLASS( Tr2PPTaaEffect );

#endif