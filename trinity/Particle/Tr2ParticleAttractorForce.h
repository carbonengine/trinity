////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef Tr2ParticleAttractorForce_H
#define Tr2ParticleAttractorForce_H

#include "ITr2ParticleForce.h"

BLUE_DECLARE( Tr2ParticleAttractorForce );

// -------------------------------------------------------------
// Description:
//   A force to apply to particles in a particle system. 
//   Represents a constant magnitude force pointing to a defined
//   position.
// SeeAlso:
//   Tr2SpriteParticleSystem
// -------------------------------------------------------------
class Tr2ParticleAttractorForce:
	public ITr2ParticleForce
{
public:
	EXPOSE_TO_BLUE();

	Tr2ParticleAttractorForce( IRoot* lockobj = 0 );
	~Tr2ParticleAttractorForce();

	XMVECTOR FASTCALL GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass );
	void Update( float dt ) {}

private:
	// Force magnitude
	float m_magnitude;
	// Attractor origin
	Vector3 m_position;
};

TYPEDEF_BLUECLASS( Tr2ParticleAttractorForce );
BLUE_DECLARE_VECTOR( Tr2ParticleAttractorForce );

#endif