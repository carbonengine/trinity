////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#pragma once
#ifndef Tr2PPFadeEffect_H
#define Tr2PPFadeEffect_H

#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_CLASS( Tr2PPFadeEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPFadeEffect( IRoot* lockobj = NULL );
	~Tr2PPFadeEffect();

	// Tr2PPEffect
	bool IsActive() override;
	Color m_color;
	float m_intensity;
};

TYPEDEF_BLUECLASS( Tr2PPFadeEffect );

#endif