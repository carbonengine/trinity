// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPFogEffect.h"


Tr2PPFogEffect::Tr2PPFogEffect( IRoot* lockobj ) :
	m_color( 1.0, 0.4235294, 0.0, 1.0 ),
	m_nebulaInfluence( 0.5 ),
	m_nebulaBlur( 7.0 ),
	m_originalBrightenOnly( 0.5 ),
	m_colorInfluence( 0.125 ),
	m_totalAmount( 0.0 ),
	m_totalPower( 1.0 ),
	m_backgroundOcclusion( 1.0 ),
	m_intensity( 1.0 ),
	m_brightnessThreshold0( 0.0 ),
	m_brightnessThreshold1( 0.5 ),
	m_brightnessAdjustmentAmount( 1.0 ),
	m_blendDistance0( 2000.0 ),
	m_blendBias0( 0.0 ),
	m_blendAmount0( 0.2 ),
	m_blendPower0( 2.0 ),
	m_blendDistance1( 25000.0 ),
	m_blendBias1( 0.6 ),
	m_blendAmount1( 0.35 ),
	m_blendPower1( 1.0 ),
	m_blendDistance2( 120000.0 ),
	m_blendBias2( 1.0 ),
	m_blendAmount2( 0.5 ),
	m_blendPower2( 0.2 ),
	m_areaSize( 69142.0859375, 13828.4169922, 66337.203125 ),
	m_areaScale( 30.0, 20.0 ),
	m_areaCenter( -27042.2988281, -633.4446411, 11896.0957031 )
{
}

Tr2PPFogEffect::~Tr2PPFogEffect()
{
}

bool Tr2PPFogEffect::IsActive()
{
	return m_display && m_intensity > 0.0f;
}