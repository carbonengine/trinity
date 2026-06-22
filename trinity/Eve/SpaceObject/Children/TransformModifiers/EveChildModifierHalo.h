// Copyright © 2018 CCP ehf.

#pragma once
#ifndef EveChildModifierHalo_H
#define EveChildModifierHalo_H

#include "IEveChildTransformModifier.h"

BLUE_CLASS( EveChildModifierHalo ) :
	public IEveChildTransformModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildModifierHalo( IRoot* lockobj = NULL );
	~EveChildModifierHalo();

	Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const Float4x3* bones ) const;
};

TYPEDEF_BLUECLASS( EveChildModifierHalo );

#endif