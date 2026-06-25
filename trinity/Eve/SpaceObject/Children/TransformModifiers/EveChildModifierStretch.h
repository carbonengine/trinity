// Copyright © 2020 CCP ehf.

#pragma once

#include "IEveChildTransformModifier.h"

#include <ITriFunction.h>

BLUE_CLASS( EveChildModifierStretch ) :
	public IEveChildTransformModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildModifierStretch( IRoot* lockobj = NULL );
	~EveChildModifierStretch();

	Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const Float4x3* bones ) const;
	void SetDest( ITriVectorFunction * dest );
	void SetDestPosition( Vector3 destPosition );

private:
	ITriVectorFunctionPtr m_dest;
	Vector3 m_destPosition;
};

TYPEDEF_BLUECLASS( EveChildModifierStretch );
