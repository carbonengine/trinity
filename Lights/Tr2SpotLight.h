////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#include "Tr2Light.h"

class Tr2LightManager;

BLUE_CLASS( Tr2SpotLight ):
	public Tr2Light
{
public:
	EXPOSE_TO_BLUE();

	Tr2SpotLight( IRoot* lockobj = nullptr );

	void RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& worldMatrix ) override;
};

TYPEDEF_BLUECLASS( Tr2SpotLight );