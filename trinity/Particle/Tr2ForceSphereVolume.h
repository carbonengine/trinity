// Copyright © 2012 CCP ehf.

#pragma once
#ifndef Tr2ForceSphereVolume_H
#define Tr2ForceSphereVolume_H

#include "ITr2ParticleForce.h"

BLUE_DECLARE( Tr2ForceSphereVolume );

class Tr2ForceSphereVolume : public ITr2ParticleForce
{
public:
	EXPOSE_TO_BLUE();

	Tr2ForceSphereVolume( IRoot* lockobj = 0 );
	~Tr2ForceSphereVolume();

	XMVECTOR FASTCALL GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass );
	void Update( float dt )
	{
	}

	void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldTransform, const CcpMath::AxisAlignedBox& aabb ) const override;

private:
	// Forces in volume
	PITr2ParticleForceVector m_forces;
	// Sphere origin
	Vector3 m_position;
	// Sphere radius
	float m_radius;
	// Falloff exponent
	float m_exponent;
};

TYPEDEF_BLUECLASS( Tr2ForceSphereVolume );
BLUE_DECLARE_VECTOR( Tr2ForceSphereVolume );

#endif // Tr2ForceSphereVolume_H