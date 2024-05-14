////////////////////////////////////////////////////////////////////////////////
//
// Created:		May 2024
// Copyright:	CCP 2024
//

#include "StdAfx.h"
#include "Tr2PPTonemappingEffect.h"


Tr2PPTonemappingEffect::Tr2PPTonemappingEffect( IRoot* lockobj ) :
	m_shoulderStrength( 0.125f ),
	m_linearStrength( 0.25f ),
	m_linearAngle( 0.1f ),
	m_toeStrength( 0.15f ),
	m_toeNumerator( 0.021f ),
	m_toeDenominator( 0.3f ),
	m_whiteScale( 2.5f )
{
	
}

Tr2PPTonemappingEffect::~Tr2PPTonemappingEffect()
{

}
