////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#pragma once

#include "IEveChildTransformModifier.h"

BLUE_CLASS( EveChildModifierAttachToBone ) :
	public IEveChildTransformModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildModifierAttachToBone( IRoot* lockobj = nullptr );

	Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const granny_matrix_3x4* bones ) const;

	void SetBoneIndex( int32_t index );
private:
	int32_t m_boneIndex;
};

TYPEDEF_BLUECLASS( EveChildModifierAttachToBone );
