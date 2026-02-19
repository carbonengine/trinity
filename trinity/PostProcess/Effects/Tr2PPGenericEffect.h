#pragma once

#include "PostProcess/Effects/Tr2PPEffect.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2PostProcessRenderer );

BLUE_CLASS( Tr2PPGenericEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();
	
	Tr2PPGenericEffect( IRoot* lockobj = NULL );

	PostProcess::Quality m_quality;
	std::string m_shaderPath;

	Tr2EffectPtr m_effect;
};
TYPEDEF_BLUECLASS( Tr2PPGenericEffect );
