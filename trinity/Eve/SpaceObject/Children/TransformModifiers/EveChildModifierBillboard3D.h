// Copyright © 2018 CCP ehf.

#pragma once
#ifndef EveChildModifierBillboard3D_H
#define EveChildModifierBillboard3D_H

#include "IEveChildTransformModifier.h"

BLUE_CLASS( EveChildModifierBillboard3D ) :
	public IEveChildTransformModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildModifierBillboard3D( IRoot* lockobj = NULL );
	~EveChildModifierBillboard3D();

	Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const Float4x3* bones ) const;

private:
	bool m_fixed;
};

TYPEDEF_BLUECLASS( EveChildModifierBillboard3D );

#endif