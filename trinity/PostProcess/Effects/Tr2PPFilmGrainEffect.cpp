// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2PPFilmGrainEffect.h"


Tr2PPFilmGrainEffect::Tr2PPFilmGrainEffect( IRoot* lockobj ) :
	m_colored( true ),
	m_colorAmount( 0.6f ),

	m_grainSize( 1.25f ),
	m_intensity( 0.0008f ),
	m_grainDensity( 0.35f ),
	m_grainContrast( 4.0f ),
	m_brightnessModifier( -3.0f )
{
}

Tr2PPFilmGrainEffect::~Tr2PPFilmGrainEffect()
{
}

bool Tr2PPFilmGrainEffect::IsActive()
{
	return m_display && m_intensity > 0.0f;
}
