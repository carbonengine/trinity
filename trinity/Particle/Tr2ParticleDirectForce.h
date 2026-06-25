// Copyright © 2010 CCP ehf.

#pragma once
#ifndef Tr2ParticleDirectForce_H
#define Tr2ParticleDirectForce_H

#include "ITr2ParticleForce.h"

BLUE_DECLARE( Tr2ParticleDirectForce );

// -------------------------------------------------------------
// Description:
//   A force to apply to particles in a particle system.
//   Represents a constant force.
// SeeAlso:
//   Tr2SpriteParticleSystem
// -------------------------------------------------------------
class Tr2ParticleDirectForce : public ITr2ParticleForce
{
public:
	EXPOSE_TO_BLUE();

	Tr2ParticleDirectForce( IRoot* lockobj = 0 );
	~Tr2ParticleDirectForce();

	XMVECTOR FASTCALL GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass );
	void Update( float dt )
	{
	}

	void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldTransform, const CcpMath::AxisAlignedBox& aabb ) const override;

private:
	// Force vector
	Vector3 m_force;
};

TYPEDEF_BLUECLASS( Tr2ParticleDirectForce );
BLUE_DECLARE_VECTOR( Tr2ParticleDirectForce );

#endif