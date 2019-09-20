////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPFadeEffect.h"


Tr2PPFadeEffect::Tr2PPFadeEffect( IRoot* lockobj ) :
	m_intensity( 0.0f ),
	m_color( 0.0, 0.0, 0.0, 0.0 )
{

}

Tr2PPFadeEffect::~Tr2PPFadeEffect()
{

}

bool Tr2PPFadeEffect::IsActive() {
	return m_display && m_intensity > 0;
}