////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#pragma once
#ifndef EveChildModifierBillboard2D_H
#define EveChildModifierBillboard2D_H

#include "IEveChildTransformModifier.h"

BLUE_CLASS( EveChildModifierBillboard2D ) :
	public IEveChildTransformModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildModifierBillboard2D( IRoot* lockobj = NULL );
	~EveChildModifierBillboard2D();

	Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const granny_matrix_3x4* bones ) const;
};

TYPEDEF_BLUECLASS( EveChildModifierBillboard2D );

#endif