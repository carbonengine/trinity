////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPBloomEffect.h"


Tr2PPBloomEffect::Tr2PPBloomEffect( IRoot* lockobj ) :
	m_luminanceThreshold( 0.0f ),
	m_luminanceScale( 0.5f ),
	m_bloomBrightness( 0.2f ),
	m_exposureDependency( false ),
	m_grimeWeight( 0.0f ),
	m_grimePath( "res:/texture/global/black.dds" )
{
	
}

Tr2PPBloomEffect::~Tr2PPBloomEffect()
{

}
