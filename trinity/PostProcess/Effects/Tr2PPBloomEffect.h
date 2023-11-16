////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#pragma once
#ifndef Tr2PPBloomEffect_H
#define Tr2PPBloomEffect_H

#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_CLASS( Tr2PPBloomEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPBloomEffect( IRoot* lockobj = NULL );
	~Tr2PPBloomEffect();
	
	float m_luminanceThreshold;
	float m_luminanceScale;
	float m_bloomBrightness;
	bool m_exposureDependency;
	float m_grimeWeight;
	BlueSharedString m_grimePath;
};

TYPEDEF_BLUECLASS( Tr2PPBloomEffect );

#endif