////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#pragma once
#ifndef Tr2PPFilmGrainEffect_H
#define Tr2PPFilmGrainEffect_H

#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_CLASS( Tr2PPFilmGrainEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPFilmGrainEffect( IRoot* lockobj = NULL );
	~Tr2PPFilmGrainEffect();

	// Tr2PPEffect
	bool IsActive() override;

	bool m_srgbCorrect;
	bool m_colored;
	float m_colorAmount;

	float m_grainSize;
	float m_intensity;
	float m_grainDensity;
	float m_grainContrast;
	float m_brightnessModifier;
};

TYPEDEF_BLUECLASS( Tr2PPFilmGrainEffect );

#endif // Tr2PPFilmGrainEffect_H