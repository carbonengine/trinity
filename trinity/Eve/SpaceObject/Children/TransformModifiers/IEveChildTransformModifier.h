////////////////////////////////////////////////////////////
//
//    Created:   2017
//    Copyright: CCP 2017
//
#pragma once
#ifndef IEveChildTransformModifier_H
#define IEveChildTransformModifier_H


BLUE_INTERFACE( IEveChildTransformModifier ) : public IRoot
{
public:
	virtual Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const granny_matrix_3x4* bones ) const = 0;
};

BLUE_DECLARE_IVECTOR( IEveChildTransformModifier );

#endif