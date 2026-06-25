// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPColorCorrectionEffect.h"

Tr2PPColorCorrectionEffect::Tr2PPColorCorrectionEffect( IRoot* lockobj ) :
	m_whiteTemperature( 6500.0f ),
	m_whiteTint( 0.0f ),
	m_colorSaturation( 1.0f ),
	m_colorContrast( 1.0f ),
	m_colorGamma( 1.0f ),
	m_colorGain( Vector3( 1.0f, 1.0f, 1.0f ) ),
	m_colorOffset( Vector3( 0.0f, 0.0f, 0.0f ) )
{
}

Tr2PPColorCorrectionEffect::~Tr2PPColorCorrectionEffect()
{
}
