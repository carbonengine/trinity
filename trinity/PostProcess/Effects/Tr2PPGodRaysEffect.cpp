///////////////////////////////////////////////////////////////////////////////
//
// Created:		1/17/2019 
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPGodRaysEffect.h"


Tr2PPGodRaysEffect::Tr2PPGodRaysEffect( IRoot* lockobj ) :
	m_godRayColor( 1.0, 1.0, 1.0, 1.0 ),
	m_intensity( 0.0f ),
	m_noiseTexturePath( "res:/Texture/Global/noise.dds" ),
	grFactors( 1000.0, 0.2, 128.0, 2.0 )
{
}


Tr2PPGodRaysEffect::~Tr2PPGodRaysEffect()
{
}


bool Tr2PPGodRaysEffect::IsActive()
{
	return m_display && m_intensity > 0.0f;
}

