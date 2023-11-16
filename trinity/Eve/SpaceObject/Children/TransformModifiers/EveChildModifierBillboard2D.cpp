////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#include "StdAfx.h"
#include "EveChildModifierBillboard2D.h"
#include "EveChildModifierTransformCommon.h"

EveChildModifierBillboard2D::EveChildModifierBillboard2D( IRoot* lockobj )
{
}

EveChildModifierBillboard2D::~EveChildModifierBillboard2D()
{
}

Matrix EveChildModifierBillboard2D::ApplyTransform( const Matrix& transform, size_t, const granny_matrix_3x4* ) const
{
	return Billboard2D( transform );
}