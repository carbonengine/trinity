// Copyright © 2019 CCP ehf.

#pragma once

#ifndef Tr2PPSignalLossEffect_H
#define Tr2PPSignalLossEffect_H

#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_CLASS( Tr2PPSignalLossEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPSignalLossEffect( IRoot* lockobj = NULL );
	~Tr2PPSignalLossEffect();

	// Tr2PPEffect
	bool IsActive() override;

	float m_strength;
};
TYPEDEF_BLUECLASS( Tr2PPSignalLossEffect );

#endif
