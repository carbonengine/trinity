////////////////////////////////////////////////////////////////////////////////
//
// Created:		May 2024
// Copyright:	CCP 2024
//

#include "StdAfx.h"
#include "Tr2PPTonemappingEffect.h"

Tr2PPTonemappingEffect::Tr2PPTonemappingEffect( IRoot* lockobj )
{
	m_uncharted2 = {};
	m_uncharted2.m_shoulderStrength = 0.125f;
	m_uncharted2.m_linearStrength = 0.25f;
	m_uncharted2.m_linearAngle = 0.1f;
	m_uncharted2.m_toeStrength = 0.15f;
	m_uncharted2.m_toeNumerator = 0.021f;
	m_uncharted2.m_toeDenominator = 0.3f;
	m_uncharted2.m_whiteScale = 2.5f;

	m_aces = {};
	m_aces.m_slope = 0.88f;
	m_aces.m_toe = 0.55f;
	m_aces.m_shoulder = 0.26f;
	m_aces.m_blackClip = 0.f;
	m_aces.m_whiteClip = 0.04f;
	m_aces.m_scale = 1.f;
	m_aces.m_blueCorrection = .0f;
	m_aces.m_useSweeteners = true;

	m_method = Aces;
}

Tr2PPTonemappingEffect::~Tr2PPTonemappingEffect()
{

}
