////////////////////////////////////////////////////////////////////////////////
//
// Created:		November 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPFidelityFXEffect.h"

Tr2PPFidelityFXEffect::Tr2PPFidelityFXEffect( IRoot* lockobj ) :
	m_intensity( 0 )
{
}

Tr2PPFidelityFXEffect::~Tr2PPFidelityFXEffect()
{

}

bool Tr2PPFidelityFXEffect::IsActive()
{
	return m_display && m_intensity > 0;
}