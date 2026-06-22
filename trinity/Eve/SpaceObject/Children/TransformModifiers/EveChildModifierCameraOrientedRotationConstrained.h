// Copyright © 2018 CCP ehf.

#pragma once
#ifndef EveChildModifierCameraOrientedRotationConstrained_H
#define EveChildModifierCameraOrientedRotationConstrained_H

#include "IEveChildTransformModifier.h"

BLUE_CLASS( EveChildModifierCameraOrientedRotationConstrained ) :
	public IEveChildTransformModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildModifierCameraOrientedRotationConstrained( IRoot* lockobj = NULL );
	~EveChildModifierCameraOrientedRotationConstrained();

	Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const Float4x3* bones ) const;
};

TYPEDEF_BLUECLASS( EveChildModifierCameraOrientedRotationConstrained );

#endif