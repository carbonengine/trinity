// Copyright © 2010 CCP ehf.

#pragma once
#ifndef Tr2ParticleDragForce_H
#define Tr2ParticleDragForce_H

#include "ITr2ParticleForce.h"

BLUE_DECLARE( Tr2ParticleDragForce );

// -------------------------------------------------------------
// Description:
//   A force to apply to particles in a particle system.
//   Represents a drag force: proportional and opposite to
//   particle velocity.
// SeeAlso:
//   Tr2SpriteParticleSystem
// -------------------------------------------------------------
class Tr2ParticleDragForce : public ITr2ParticleForce
{
public:
	EXPOSE_TO_BLUE();

	Tr2ParticleDragForce( IRoot* lockobj = 0 );
	~Tr2ParticleDragForce();

	XMVECTOR FASTCALL GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass );
	void Update( float dt )
	{
	}

private:
	// Drag coefficient
	float m_dragConstant;
};

TYPEDEF_BLUECLASS( Tr2ParticleDragForce );
BLUE_DECLARE_VECTOR( Tr2ParticleDragForce );

#endif