// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierAttachToBone.h"
#include "Utilities/MatrixUtils.h"


EveChildModifierAttachToBone::EveChildModifierAttachToBone( IRoot* ) :
	m_boneIndex( -1 )
{
}

Matrix EveChildModifierAttachToBone::ApplyTransform( const Matrix& transform, size_t boneCount, const Float4x3* bones ) const
{
	if( m_boneIndex < 0 || size_t( m_boneIndex ) >= boneCount )
	{
		return transform;
	}

	Matrix m = IdentityMatrix();
	TriMatrixCopyFrom3x4( &m, &bones[m_boneIndex] );
	return m * transform;
}

void EveChildModifierAttachToBone::SetBoneIndex( int32_t index )
{
	m_boneIndex = index;
}
