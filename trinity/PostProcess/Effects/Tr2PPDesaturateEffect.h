////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#pragma once
#ifndef Tr2PPDesaturateEffect_H
#define Tr2PPDesaturateEffect_H

#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_CLASS( Tr2PPDesaturateEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPDesaturateEffect( IRoot* lockobj = NULL );
	~Tr2PPDesaturateEffect();

	float m_intensity;
};

TYPEDEF_BLUECLASS( Tr2PPDesaturateEffect );

#endif
