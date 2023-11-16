////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#include "StdAfx.h"
#include "EveChildModifierCameraOrientedRotationConstrained.h"
#include "Tr2Renderer.h"

EveChildModifierCameraOrientedRotationConstrained::EveChildModifierCameraOrientedRotationConstrained( IRoot* lockobj )
{
}

EveChildModifierCameraOrientedRotationConstrained::~EveChildModifierCameraOrientedRotationConstrained()
{
}

Matrix EveChildModifierCameraOrientedRotationConstrained::ApplyTransform( const Matrix& transform, size_t, const granny_matrix_3x4* ) const
{
	Vector3 up = Vector3( 0, 1, 0 );

	Vector3 scale, translation;
	Quaternion rotation;
	Decompose( scale, rotation, translation, transform );

	Matrix rotMatrix = RotationMatrix( rotation );
	Vector3 camPos = Tr2Renderer::GetViewPosition();

	Vector4 vec = rotMatrix * Vector4( camPos - translation, 0 );

	float rot = atan2f( vec.x, vec.z );
	Matrix result = RotationMatrix( up, rot );

	return result * transform;
}