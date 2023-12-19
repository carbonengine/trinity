////////////////////////////////////////////////////////////
//
//    Created:   January 2015
//    Copyright: CCP 2015
//

#pragma once
#include "Tr2Light.h"

class Tr2LightManager;

BLUE_CLASS( Tr2PointLight ) :
	public Tr2Light
{
public:
	EXPOSE_TO_BLUE();
	Tr2PointLight( IRoot* lockobj = nullptr );
	void SetBlinkingBehavior( float blinkRate, float blinkPhase, float minScale, float maxScale );

	void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldMatrix, const granny_matrix_3x4* bones = nullptr, size_t boneCount = 0 ) override;
	void Update() override;

private:

	float Blink() const;

	bool m_blinks;
	float m_blinkRate;
	float m_blinkPhase;
	float m_minScale;
	float m_maxScale;
	float m_blinkShift;

	float m_originalRadius;
	float m_originalInnerRadius;

};

TYPEDEF_BLUECLASS( Tr2PointLight );