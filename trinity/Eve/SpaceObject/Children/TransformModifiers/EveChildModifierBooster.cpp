////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#include "StdAfx.h"
#include "EveChildModifierBooster.h"
#include "Tr2Renderer.h"
#include "EveChildModifierTransformCommon.h"

EveChildModifierBooster::EveChildModifierBooster( IRoot* lockobj )
{
}

EveChildModifierBooster::~EveChildModifierBooster()
{
}

Matrix EveChildModifierBooster::ApplyTransform( const Matrix& transform, size_t, const granny_matrix_3x4* ) const
{
	Matrix alignMat;
	float distCenter;
	Vector3 d;
	DistanceBase( transform, alignMat, distCenter, d );

	float radius = 0.5f;
	float B = sqrtf( distCenter*distCenter - radius*radius );
	float scale = B / distCenter;
	float trans = -radius*radius / (distCenter*scale);

	Matrix scalingTransform = ScalingMatrix( scale, scale, scale );
	Matrix translationTransform = TranslationMatrix( 0.0f, 0.0f, trans );

	return translationTransform * alignMat * scalingTransform * transform;
}