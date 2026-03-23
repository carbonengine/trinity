#pragma once

#include "PostProcess/Effects/Tr2PPEffect.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2PostProcessRenderer );

BLUE_CLASS( Tr2PPGenericEffect ) :
	public Tr2PPEffect,
	public INotify
{
public:
	EXPOSE_TO_BLUE();
	
	Tr2PPGenericEffect( IRoot* lockobj = NULL );

	Tr2EffectPtr GetEffect();
	bool OnModified( Be::Var* value );

	PostProcess::Quality m_quality;
	std::string m_shaderPath;

private:
	Tr2EffectPtr m_effect;
	bool m_recreateEffect;
};
TYPEDEF_BLUECLASS( Tr2PPGenericEffect );
