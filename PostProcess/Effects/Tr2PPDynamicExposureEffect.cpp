////////////////////////////////////////////////////////////////////////////////
//
// Created:		2/5/2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPDynamicExposureEffect.h"
#include "Tr2Renderer.h"


Tr2PPDynamicExposureEffect::Tr2PPDynamicExposureEffect( IRoot* lockobj ) :
	m_minBrightness( 0.9f ),
	m_maxBrightness( 0.98f ),
	m_increaseSpeed( 2.0f ),
	m_decreaseSpeed( 1.5f ),
	m_minLuminance( 0.09f ),
	m_maxLuminance( 10.0f ),
	m_influence( 1.0f ),
	m_middleValue( 0.55f ),
	m_adjustment( 0.15f ),
	m_minExposure( -1.25f ),
	m_maxExposure( 0.0f )
{
}


Tr2PPDynamicExposureEffect::~Tr2PPDynamicExposureEffect()
{
}
