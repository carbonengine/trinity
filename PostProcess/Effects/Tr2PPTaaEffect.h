////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#pragma once
#ifndef Tr2PPTaaEffect_H
#define Tr2PPTaaEffect_H

#include "PostProcess/Effects/Tr2PPEffect.h"


BLUE_CLASS( Tr2PPTaaEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPTaaEffect( IRoot* lockobj = NULL );
	~Tr2PPTaaEffect();

	const Vector4 m_blendParams0;
	const Vector4 m_blendParams1;
	const Vector4 m_blendParams2;
	const Vector4 m_distanceParams;
	const Vector4 m_enhancementParams;
};

TYPEDEF_BLUECLASS( Tr2PPTaaEffect );

#endif