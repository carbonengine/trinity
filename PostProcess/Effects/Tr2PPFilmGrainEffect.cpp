////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPFilmGrainEffect.h"


Tr2PPFilmGrainEffect::Tr2PPFilmGrainEffect( IRoot* lockobj ) :
	m_useNewTechnique( false ),
	m_compare( false ),
	m_colored( true ),
	m_colorAmount( 0.6f ),

	m_oldGrainSize( 3.0f ),
	m_oldIntensity( 0.005f ),
	m_oldLuminanceExponent( 0.8f ),

	m_newGrainSize( 1.3f ),
	m_newIntensity( 0.005f ),
	m_newGrainDensity( 0.4f ),
	m_newGrainContrast( 6.0f ),
	m_newLuminanceSensitivity( 1.0f )
{
}

Tr2PPFilmGrainEffect::~Tr2PPFilmGrainEffect()
{

}

bool Tr2PPFilmGrainEffect::IsActive()
{
	return m_display && (m_useNewTechnique ? m_newIntensity > 0.0f : m_oldIntensity > 0.0f);
}
