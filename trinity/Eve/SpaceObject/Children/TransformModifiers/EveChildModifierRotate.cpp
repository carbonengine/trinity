// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierRotate.h"

EveChildModifierRotate::EveChildModifierRotate( IRoot* lockobj ) :
	m_rotationSpeed( 0.f, 0.f, 0.f )
{
}

EveChildModifierRotate::~EveChildModifierRotate()
{
}

Matrix EveChildModifierRotate::ApplyTransform( const Matrix& transform, size_t, const Float4x3* ) const
{
	// Absolute time — never accumulated — so rotation stays stable across frame-time variation.
	const double time = TimeAsDouble( BeOS->GetCurrentFrameTime() );
	const float angleX = float( time * double( m_rotationSpeed.x ) );
	const float angleY = float( time * double( m_rotationSpeed.y ) );
	const float angleZ = float( time * double( m_rotationSpeed.z ) );

	const Matrix localRotation =
		RotationXMatrix( angleX ) *
		RotationYMatrix( angleY ) *
		RotationZMatrix( angleZ );

	// Right-multiply so the spin is in the object's local axes and translation is preserved.
	return transform * localRotation;
}
