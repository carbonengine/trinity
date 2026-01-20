////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPTaaEffect.h"


Tr2PPTaaEffect::Tr2PPTaaEffect( IRoot* lockobj ) :
	m_quality( Tr2PPTaaEffect::Quality::TAA_HIGH ),
	m_showMotionVectors( false ),
	m_showEarlyOutMask( false ),
	m_earlyOutThreshold( 0.001 )
{
}

Tr2PPTaaEffect::~Tr2PPTaaEffect()
{

}
