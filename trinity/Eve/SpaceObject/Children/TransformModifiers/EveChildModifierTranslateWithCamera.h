// Copyright © 2018 CCP ehf.

#pragma once
#ifndef EveChildModifierTranslateWithCamera_H
#define EveChildModifierTranslateWithCamera_H

#include "IEveChildTransformModifier.h"

BLUE_CLASS( EveChildModifierTranslateWithCamera ) :
	public IEveChildTransformModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildModifierTranslateWithCamera( IRoot* lockobj = NULL );
	~EveChildModifierTranslateWithCamera();

	Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const Float4x3* bones ) const;

private:
	bool m_attachedToCamera;
};

TYPEDEF_BLUECLASS( EveChildModifierTranslateWithCamera );

#endif