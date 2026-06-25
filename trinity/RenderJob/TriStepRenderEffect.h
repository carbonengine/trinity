// Copyright © 2023 CCP ehf.

#pragma once
#ifndef _TRISTEPRENDEREFFECT_H_
#define _TRISTEPRENDEREFFECT_H_


#include "TriRenderStep.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2ShaderBuffer );

BLUE_CLASS( TriStepRenderEffect ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepRenderEffect( IRoot* lockobj = 0 );
	~TriStepRenderEffect( void );

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	// Python __init__ constructor
	void py__init__( Tr2Effect * effect, Tr2ShaderBuffer * shaderBuffer );

private:
	Tr2EffectPtr m_effect;
	Tr2ShaderBufferPtr m_shaderBuffer;
	Vector2 m_tlTexCoord;
	Vector2 m_brTexCoord;
};

TYPEDEF_BLUECLASS( TriStepRenderEffect );

#endif