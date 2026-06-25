// Copyright © 2019 CCP ehf.

#pragma once
#ifndef Tr2PPLutEffect_H
#define Tr2PPLutEffect_H

#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_CLASS( Tr2PPLutEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPLutEffect( IRoot* lockobj = NULL );
	~Tr2PPLutEffect();

	// Tr2PPEffect
	bool IsActive() override;

	float m_influence;
	BlueSharedString m_path;
};

TYPEDEF_BLUECLASS( Tr2PPLutEffect );

#endif