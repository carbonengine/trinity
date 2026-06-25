// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierHaloInverted.h"
#include "Tr2Renderer.h"
#include "EveChildModifierTransformCommon.h"
#include "include/TriMath.h"

EveChildModifierHaloInverted::EveChildModifierHaloInverted( IRoot* lockobj )
{
}

EveChildModifierHaloInverted::~EveChildModifierHaloInverted()
{
}

Matrix EveChildModifierHaloInverted::ApplyTransform( const Matrix& transform, size_t, const Float4x3* ) const
{
	Matrix alignMat;
	float distCenter;
	Vector3 d;
	DistanceBase( transform, alignMat, distCenter, d );

	Vector3 forward;

	forward = Normalize( *TriVectorRotatedBasisMatrix( &forward, TRITA_Z, &transform ) );
	Vector3 backward = -forward;

	float scale = Dot( Normalize( d ), backward );
	if( scale < 0.0f )
	{
		scale = 0.0f;
	}

	Matrix scalingTransform = ScalingMatrix( scale, scale, scale );

	return scalingTransform * alignMat * transform;
}