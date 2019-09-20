////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPFilmGrainEffect.h"


Tr2PPFilmGrainEffect::Tr2PPFilmGrainEffect( IRoot* lockobj ) :
	m_intensity( 0.003f ),
	m_colored( true ),
	m_colorAmount( 0.6f ),
	m_grainSize( 2.0f ),
	m_luminanceExponent( 0.2f )
{
}

Tr2PPFilmGrainEffect::~Tr2PPFilmGrainEffect()
{

}

bool Tr2PPFilmGrainEffect::IsActive()
{
	return m_display && m_intensity > 0.0;
}
