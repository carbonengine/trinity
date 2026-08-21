// Copyright © 2026 CCP ehf.

#pragma once
#ifndef EveChildModifierRotate_H
#define EveChildModifierRotate_H

#include "IEveChildTransformModifier.h"

BLUE_CLASS( EveChildModifierRotate ) :
	public IEveChildTransformModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildModifierRotate( IRoot* lockobj = NULL );
	~EveChildModifierRotate();

	Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const Float4x3* bones ) const;

private:
	Vector3 m_rotationSpeed;
};

TYPEDEF_BLUECLASS( EveChildModifierRotate );

#endif
