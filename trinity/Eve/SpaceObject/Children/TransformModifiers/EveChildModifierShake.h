// Copyright © 2026 CCP ehf.

#pragma once
#ifndef EveChildModifierShake_H
#define EveChildModifierShake_H

#include "IEveChildTransformModifier.h"

BLUE_CLASS( EveChildModifierShake ) :
	public IEveChildTransformModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildModifierShake( IRoot* lockobj = NULL );
	~EveChildModifierShake();

	Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const Float4x3* bones ) const;

private:
	float m_frequency;
	float m_amplitude;
};

TYPEDEF_BLUECLASS( EveChildModifierShake );

#endif
