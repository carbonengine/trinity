// Copyright © 2017 CCP ehf.

#pragma once
#ifndef IEveChildTransformModifier_H
#define IEveChildTransformModifier_H


BLUE_INTERFACE( IEveChildTransformModifier ) :
	public IRoot
{
public:
	virtual Matrix ApplyTransform( const Matrix& transform, size_t boneCount, const Float4x3* bones ) const = 0;
};

BLUE_DECLARE_IVECTOR( IEveChildTransformModifier );

#endif