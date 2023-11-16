////////////////////////////////////////////////////////////
//
//    Created:   September 2010
//    Copyright: CCP 2010
//
//    Refactored from EveParticleDirectForce.cpp

#include "StdAfx.h"
#include "Tr2ParticleDirectForce.h"

Tr2ParticleDirectForce::Tr2ParticleDirectForce( IRoot* lockobj ):
	m_force( 1.f, 1.f, 1.f )
{
}

Tr2ParticleDirectForce::~Tr2ParticleDirectForce()
{
}

// -------------------------------------------------------------
// Description:
//   Applies a force to a particle.
// Arguments:
//   position - Particle position (not used)
//   velocity - Particle velocity (not used)
//   dt - Frame time (not used)
//   mass - Particle mass (not used)
// Return value:
//   A force to apply to a particle (in our case - a constant vector)
// -------------------------------------------------------------
XMVECTOR Tr2ParticleDirectForce::GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass )
{
	return m_force;
}