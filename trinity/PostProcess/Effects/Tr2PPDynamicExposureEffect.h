////////////////////////////////////////////////////////////////////////////////
//
// Created:		2/5/2019 
// Copyright:	CCP 2019
//

#pragma once

#ifndef Tr2PPDynamicExposureEffect_H
#define Tr2PPDynamicExposureEffect_H

#include "PostProcess/Effects/Tr2PPEffect.h"

BLUE_CLASS( Tr2PPDynamicExposureEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPDynamicExposureEffect( IRoot* lockobj = NULL );
	~Tr2PPDynamicExposureEffect();

	float m_minBrightness;
	float m_maxBrightness;
	float m_increaseSpeed;
	float m_decreaseSpeed;
	float m_minLuminance;
	float m_maxLuminance;
	float m_influence;
	float m_middleValue;
	float m_adjustment;
	float m_minExposure;
	float m_maxExposure;

};
TYPEDEF_BLUECLASS( Tr2PPDynamicExposureEffect );

#endif // Tr2PPDynamicExposureEffect_H
