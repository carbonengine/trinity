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

	Tr2PPTaaEffect( IRoot* lockobj = NULL );
	~Tr2PPTaaEffect();

	int m_quality;
	bool m_applyMipBias;
	bool m_showMotionVectors;
	bool m_showEarlyOutMask;
	float m_earlyOutThreshold;

};

TYPEDEF_BLUECLASS( Tr2PPTaaEffect );

#endif