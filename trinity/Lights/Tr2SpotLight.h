// Copyright © 2019 CCP ehf.

#pragma once

#include "Tr2Light.h"

class Tr2LightManager;

BLUE_CLASS( Tr2SpotLight ) :
	public Tr2Light
{
public:
	EXPOSE_TO_BLUE();

	Tr2SpotLight( IRoot* lockobj = nullptr );

	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& worldMatrix, const Float4x3* bones = nullptr, size_t boneCount = 0 ) override;
};

TYPEDEF_BLUECLASS( Tr2SpotLight );