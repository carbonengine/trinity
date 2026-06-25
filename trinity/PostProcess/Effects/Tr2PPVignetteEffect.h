// Copyright © 2019 CCP ehf.

#pragma once
#ifndef Tr2PPVignetteEffect_H
#define Tr2PPVignetteEffect_H

#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_CLASS( Tr2PPVignetteEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPVignetteEffect( IRoot* lockobj = NULL );
	~Tr2PPVignetteEffect();

	// Tr2PPEffect
	bool IsActive() override;
	BlueSharedString m_shapePath;
	BlueSharedString m_detailPath;
	Vector2 m_detail1Size;
	Vector2 m_detail2Size;
	Vector2 m_detail1Scroll;
	Vector2 m_detail2Scroll;
	Color m_color;
	float m_opacity;
	float m_intensity;
	float m_sineFrequency;
	float m_sineMinimum;
	float m_sineMaximum;
};

TYPEDEF_BLUECLASS( Tr2PPVignetteEffect );

#endif