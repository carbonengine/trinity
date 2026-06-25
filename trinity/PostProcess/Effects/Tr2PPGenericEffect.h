// Copyright © 2026 CCP ehf.

#pragma once

#include "PostProcess/Effects/Tr2PPEffect.h"

BLUE_DECLARE( Tr2Effect );

BLUE_CLASS( Tr2PPGenericEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPGenericEffect( IRoot* lockobj = NULL );

	Tr2EffectPtr GetEffect() const;

	PostProcess::Quality m_quality;

private:
	Tr2EffectPtr m_effect;
};
TYPEDEF_BLUECLASS( Tr2PPGenericEffect );
