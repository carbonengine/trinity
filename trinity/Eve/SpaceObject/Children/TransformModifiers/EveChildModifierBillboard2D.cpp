// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierBillboard2D.h"
#include "EveChildModifierTransformCommon.h"

EveChildModifierBillboard2D::EveChildModifierBillboard2D( IRoot* lockobj )
{
}

EveChildModifierBillboard2D::~EveChildModifierBillboard2D()
{
}

Matrix EveChildModifierBillboard2D::ApplyTransform( const Matrix& transform, size_t, const Float4x3* ) const
{
	return Billboard2D( transform );
}