// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPVignetteEffect.h"


Tr2PPVignetteEffect::Tr2PPVignetteEffect( IRoot* lockobj ) :
	m_shapePath( "res:/texture/global/black.dds" ),
	m_detailPath( "res:/texture/global/white.dds" ),
	m_detail1Size( 16.0f, 16.0f ),
	m_detail2Size( 16.0f, 16.0f ),
	m_detail1Scroll( 0.0f, 0.0f ),
	m_detail2Scroll( 0.0f, 0.0f ),
	m_color( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_opacity( 0.0f ),
	m_intensity( 0.0f ),
	m_sineFrequency( 1.0f ),
	m_sineMinimum( 0.0f ),
	m_sineMaximum( 1.0f )
{
}

Tr2PPVignetteEffect::~Tr2PPVignetteEffect()
{
}

bool Tr2PPVignetteEffect::IsActive()
{
	return m_display && m_intensity > 0.0f && m_opacity > 0.0f;
}
