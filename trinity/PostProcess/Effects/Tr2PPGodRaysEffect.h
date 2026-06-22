// Copyright © 2019 CCP ehf.

#pragma once

#ifndef Tr2PPGodRaysEffect_H
#define Tr2PPGodRaysEffect_H

#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_DECLARE( Tr2ShaderBuffer );
BLUE_DECLARE( Tr2RenderTarget );

BLUE_CLASS( Tr2PPGodRaysEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPGodRaysEffect( IRoot* lockobj = NULL );
	~Tr2PPGodRaysEffect();

	// Tr2PPEffect
	bool IsActive() override;

	const Vector4 grFactors;
	Color m_godRayColor;
	float m_intensity;
	BlueSharedString m_noiseTexturePath;
};
TYPEDEF_BLUECLASS( Tr2PPGodRaysEffect );

#endif
