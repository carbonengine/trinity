////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPTaaEffect.h"


Tr2PPTaaEffect::Tr2PPTaaEffect( IRoot* lockobj ) :
	m_quality( Tr2PPTaaEffect::Quality::TAA_HIGH ),
	m_showMotionVectors( false ),
	m_showEarlyOutMask( false ),
	m_earlyOutThreshold( 0.001 ),
	m_jitterX( 0.0f ),
	m_jitterY( 0.0f )
{
	m_samplingPatterns[0] = Vector2( .125f, -.375f );
	m_samplingPatterns[1] = Vector2( -.125f, .375f );
	m_samplingPatterns[2] = Vector2( .375f, .125f );
	m_samplingPatterns[3] = Vector2( -.375f, -.125f );
}

Tr2PPTaaEffect::~Tr2PPTaaEffect()
{

}

void Tr2PPTaaEffect::GetJitter( uint32_t renderWidth, uint32_t renderHeight, float& x, float& y )
{
	if (m_display)
	{
		m_samplingIndex = ++m_samplingIndex % 4;
		m_jitterX = m_samplingPatterns[m_samplingIndex].x;
		m_jitterY = m_samplingPatterns[m_samplingIndex].y;

		x = 2.0f * m_jitterX / (float)renderWidth;
		y = 2.0f * m_jitterY / (float)renderHeight;
	}
	else
	{
		x = y = 0.0f;
	}
}
