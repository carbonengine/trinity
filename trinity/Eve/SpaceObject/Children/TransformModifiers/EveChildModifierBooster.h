// Copyright © 2018 CCP ehf.

#pragma once
#ifndef EveChildModifierBooster_H
#define EveChildModifierBooster_H

#include "IEveChildTransformModifier.h"

BLUE_CLASS( EveChildModifierBooster ) :
	public IEveChildTransformModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildModifierBooster( IRoot* lockobj = NULL );
	~EveChildModifierBooster();

	Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const Float4x3* bones ) const;
};

TYPEDEF_BLUECLASS( EveChildModifierBooster );

#endif