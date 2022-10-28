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

	bool m_useNewTechnique;
	bool m_compare;
	bool m_srgbCorrect;
	bool m_colored;
	float m_colorAmount;

	float m_oldGrainSize;
	float m_oldIntensity;
	float m_oldLuminanceExponent;

	float m_newGrainSize;
	float m_newIntensity;
	float m_newGrainDensity;
	float m_newGrainContrast;
	float m_newLuminanceSensitivity;
};

TYPEDEF_BLUECLASS( Tr2PPFilmGrainEffect );

#endif // Tr2PPFilmGrainEffect_H