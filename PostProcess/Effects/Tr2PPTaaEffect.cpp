////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPTaaEffect.h"


Tr2PPTaaEffect::Tr2PPTaaEffect( IRoot* lockobj ) :
	m_blendParams0( 0.99f, 0.75f, 1.0f, 1.0f ),
	m_blendParams1( 0.9f, 0.75f, 1.0f, 1.0f ),
	m_blendParams2( 0.5f, 0.25f, 1.0f, 0.0f ),
	m_distanceParams( 0.025f, 0.1f, 0.5f, 0.2f ),
	m_enhancementParams( 1.5f, 0.0f, 0.04f, 0.06f )
{

}

Tr2PPTaaEffect::~Tr2PPTaaEffect()
{

}
