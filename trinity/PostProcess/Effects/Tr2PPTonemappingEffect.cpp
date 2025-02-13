////////////////////////////////////////////////////////////////////////////////
//
// Created:		May 2024
// Copyright:	CCP 2024
//

#include "StdAfx.h"
#include "Tr2PPTonemappingEffect.h"


Tr2PPTonemappingEffect::Tr2PPTonemappingEffect( IRoot* lockobj ) :
	m_slope( 0.88f ),
	m_toe( 0.55f ),
	m_shoulder( 0.26f ),
	m_blackClip( 0.f ),
	m_whiteClip( 0.04f ),
	m_scale( 1.f ),
	m_blueCorrection( .0f ),
	m_useSweeteners( true )
{
	
}

Tr2PPTonemappingEffect::~Tr2PPTonemappingEffect()
{

}
