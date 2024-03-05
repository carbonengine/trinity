////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef Tr2ParticleFluidDragForce_H
#define Tr2ParticleFluidDragForce_H

#include "ITr2ParticleForce.h"

BLUE_DECLARE( Tr2ParticleFluidDragForce );

// -------------------------------------------------------------
// Description:
//   A force to apply to particles in a particle system. 
//   Represents a drag force in fluids; similar to 
//   Tr2ParticleDragForce, but depends on a square of particle 
//   speed.
// SeeAlso:
//   Tr2SpriteParticleSystem
// -------------------------------------------------------------
class Tr2ParticleFluidDragForce:
	public ITr2ParticleForce
{
public:
	EXPOSE_TO_BLUE();

	Tr2ParticleFluidDragForce( IRoot* lockobj = 0 );
	~Tr2ParticleFluidDragForce();

	XMVECTOR FASTCALL GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass );
	void Update( float dt ) {}

private:
	// Drag coefficient
	float m_dragConstant;
};

TYPEDEF_BLUECLASS( Tr2ParticleFluidDragForce );
BLUE_DECLARE_VECTOR( Tr2ParticleFluidDragForce );

#endif