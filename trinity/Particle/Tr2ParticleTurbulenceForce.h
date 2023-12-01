////////////////////////////////////////////////////////////
//
//    Created:   June 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef Tr2ParticleTurbulenceForce_H
#define Tr2ParticleTurbulenceForce_H

#include "ITr2ParticleForce.h"

BLUE_DECLARE( Tr2ParticleTurbulenceForce );

// -------------------------------------------------------------
// Description:
//   A force to apply to particles in a particle system. 
//   Represents a random time/position dependant directional
//   force.
// SeeAlso:
//   Tr2SpriteParticleSystem
// -------------------------------------------------------------
class Tr2ParticleTurbulenceForce:
	public ITr2ParticleForce
{
public:
	EXPOSE_TO_BLUE();

	Tr2ParticleTurbulenceForce( IRoot* lockobj = 0 );
	~Tr2ParticleTurbulenceForce();

	XMVECTOR FASTCALL GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass );
	void Update( float dt );

private:
	// turbulence amplitude
	Vector3 m_amplitude;
	// turbulence frequency (w - time)
	Vector4 m_frequency;
	// turbulence noise level (octave count)
	uint32_t m_noiseLevel;
	// turbulence noise ratio (octave amplitude coefficient)
	float m_noiseRatio;
	// current time
	float m_time;
};

TYPEDEF_BLUECLASS( Tr2ParticleTurbulenceForce );
BLUE_DECLARE_VECTOR( Tr2ParticleTurbulenceForce );

#endif