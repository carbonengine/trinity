////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPTaaEffect.h"


Tr2PPTaaEffect::Tr2PPTaaEffect( IRoot* lockobj ) :
	m_quality( 3 ),
	m_applyMipBias( true ),
	m_showMotionVectors( false ),
	m_showEarlyOutMask( false ),
	m_earlyOutThreshold( 0.04 )
{

}

Tr2PPTaaEffect::~Tr2PPTaaEffect()
{

}
