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

Matrix EveChildModifierCameraOrientedRotationConstrained::ApplyTransform( const Matrix& transform ) const
{
	Vector3 forward = Vector3( 0, 0, 1 );
	Vector4 forward4 = Vector4( forward, 0 );
	Matrix forwardToUp = RotationMatrix( Vector3( 1, 0, 0 ), -XM_PIDIV2 );

	Vector4 res = Tr2Renderer::GetViewTransform() * forward4;
	float rot = atan2f( res.y, res.x );
	Matrix mat = RotationMatrix( forward, rot - XM_PIDIV2 );

	return transform * forwardToUp * mat;
}