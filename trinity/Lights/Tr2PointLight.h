////////////////////////////////////////////////////////////
//
//    Created:   January 2015
//    Copyright: CCP 2015
//

#pragma once
#include "Tr2Light.h"

class Tr2LightManager;

BLUE_CLASS( Tr2PointLight ): 
	public Tr2Light
{
public:
	EXPOSE_TO_BLUE();
	Tr2PointLight( IRoot* lockobj = nullptr );
	
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldMatrix, const granny_matrix_3x4* bones = nullptr, size_t boneCount = 0) override;
};

TYPEDEF_BLUECLASS( Tr2PointLight );