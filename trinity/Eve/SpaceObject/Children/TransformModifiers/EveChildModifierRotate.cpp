// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierRotate.h"
#include "Include/TriMath.h"

EveChildModifierRotate::EveChildModifierRotate( IRoot* lockobj ) :
	m_rotationSpeed( 0.f, 0.f, 0.f ),
	m_timeOffset( 0.f )
{
}

EveChildModifierRotate::~EveChildModifierRotate()
{
}

Matrix EveChildModifierRotate::ApplyTransform( const Matrix& transform, size_t, const Float4x3* ) const
{
	// Absolute time — never accumulated — so rotation stays stable across frame-time variation.
	const double time = TimeAsDouble( BeOS->GetCurrentFrameTime() ) + double( m_timeOffset );

	// Wrap in double degrees before narrowing so the angle keeps float precision on long-running clients.
	auto angle = [time]( float degreesPerSecond ) -> float {
		return float( fmod( time * double( degreesPerSecond ), 360.0 ) ) * ( TRI_PI / 180.f );
	};

	// Same Euler composition as Tr2CurveEulerRotation: yaw about local Y, pitch about X, roll about Z.
	const Quaternion rotation = RotationQuaternion( angle( m_rotationSpeed.y ), angle( m_rotationSpeed.x ), angle( m_rotationSpeed.z ) );

	// Left-multiply so the spin happens in the object's local space, before its world transform.
	return RotationMatrix( rotation ) * transform;
}
