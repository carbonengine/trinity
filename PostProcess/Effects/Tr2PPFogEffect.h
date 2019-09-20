////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#pragma once
#ifndef Tr2PPFogEffect_H
#define Tr2PPFogEffect_H

#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_CLASS( Tr2PPFogEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPFogEffect( IRoot* lockobj = NULL );
	~Tr2PPFogEffect();

	// Tr2PPEffect
	bool IsActive() override;

	Color m_color;
	float m_nebulaInfluence;
	float m_nebulaBlur;
	float m_originalBrightenOnly;
	float m_colorInfluence;
	float m_totalAmount;
	float m_totalPower;
	float m_backgroundOcclusion;
	float m_intensity;
	float m_brightnessThreshold0;
	float m_brightnessThreshold1;
	float m_brightnessAdjustmentAmount;
	float m_blendDistance0;
	float m_blendBias0;
	float m_blendAmount0;
	float m_blendPower0;
	float m_blendDistance1;
	float m_blendBias1;
	float m_blendAmount1;
	float m_blendPower1;
	float m_blendDistance2;
	float m_blendBias2;
	float m_blendAmount2;
	float m_blendPower2;
	Vector3 m_areaSize;
	Vector2 m_areaScale;
	Vector3 m_areaCenter;



};

TYPEDEF_BLUECLASS( Tr2PPFogEffect );

#endif