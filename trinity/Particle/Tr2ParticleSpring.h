// Copyright © 2010 CCP ehf.

#pragma once
#ifndef Tr2ParticleSpringAttractor_H
#define Tr2ParticleSpringAttractor_H

#include "ITr2ParticleForce.h"

BLUE_DECLARE( Tr2ParticleSpring );

// -------------------------------------------------------------
// Description:
//   A force to apply to particles in a particle system.
//   Represents a spring force: proportional to the distance from
//   particle to a fixed "spring origin".
// SeeAlso:
//   Tr2SpriteParticleSystem
// -------------------------------------------------------------
class Tr2ParticleSpring : public ITr2ParticleForce
{
public:
	EXPOSE_TO_BLUE();

	Tr2ParticleSpring( IRoot* lockobj = 0 );
	;
	~Tr2ParticleSpring();

	XMVECTOR FASTCALL GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass );
	virtual void Update( float dt )
	{
	}

	void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldTransform, const CcpMath::AxisAlignedBox& aabb ) const override;

private:
	// Spring coefficient
	float m_springConstant;
	// Spring origin
	Vector3 m_position;
};

TYPEDEF_BLUECLASS( Tr2ParticleSpring );
BLUE_DECLARE_VECTOR( Tr2ParticleSpring );

#endif