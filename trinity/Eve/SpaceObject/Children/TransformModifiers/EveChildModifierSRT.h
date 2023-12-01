////////////////////////////////////////////////////////////
//
//    Created:   2017
//    Copyright: CCP 2017
//
#pragma once
#ifndef EveChildModifierSRT_H
#define EveChildModifierSRT_H

#include "IEveChildTransformModifier.h"

BLUE_CLASS( EveChildModifierSRT ) :
	public IEveChildTransformModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildModifierSRT( IRoot* lockobj = NULL );
	~EveChildModifierSRT();
	
	Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const granny_matrix_3x4* bones ) const;

private:
	Vector3 m_scaling;
	Vector3 m_translation;
	Quaternion m_rotation;
};

TYPEDEF_BLUECLASS( EveChildModifierSRT );

#endif