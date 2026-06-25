// Copyright © 2011 CCP ehf.

#pragma once
#ifndef Tr2ParticleVortexForce_H
#define Tr2ParticleVortexForce_H

#include "ITr2ParticleForce.h"

BLUE_DECLARE( Tr2ParticleVortexForce );

// -------------------------------------------------------------
// Description:
//   A force to apply to particles in a particle system.
//   Models a "vortex" tangent force.
// SeeAlso:
//   Tr2SpriteParticleSystem
// -------------------------------------------------------------
class Tr2ParticleVortexForce : public ITr2ParticleForce
{
public:
	EXPOSE_TO_BLUE();

	Tr2ParticleVortexForce( IRoot* lockobj = 0 );
	~Tr2ParticleVortexForce();

	XMVECTOR FASTCALL GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass );
	void Update( float dt )
	{
	}

	void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldTransform, const CcpMath::AxisAlignedBox& aabb ) const override;

private:
	// Force magnitude
	float m_magnitude;
	// Vortex origin
	Vector3 m_position;
	// Vortex axis
	Vector3 m_axis;
};

TYPEDEF_BLUECLASS( Tr2ParticleVortexForce );
BLUE_DECLARE_VECTOR( Tr2ParticleVortexForce );

#endif