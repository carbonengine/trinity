////////////////////////////////////////////////////////////
//
//    Created:   September 2010
//    Copyright: CCP 2010
//
//    Refactored from EveParticleSpringAttractor.cpp

#include "StdAfx.h"
#include "Tr2ParticleSpring.h"

Tr2ParticleSpring::Tr2ParticleSpring( IRoot* lockobj ) :
	m_springConstant( 0.0f ),
	m_position( 0.f, 0.f, 0.f )
{
}

Tr2ParticleSpring::~Tr2ParticleSpring()
{

}

// -------------------------------------------------------------
// Description:
//   Applies spring force to a particle.
// Arguments:
//   position - Particle position
//   velocity - Particle velocity (not used)
//   dt - Frame time (not used)
//   mass - Particle mass (not used)
// Return value:
//   Spring force to apply to a particle
// -------------------------------------------------------------
XMVECTOR Tr2ParticleSpring::GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass )
{
	XMVECTOR direction = XMVectorSubtract( position, m_position );
	return XMVectorScale( direction, -m_springConstant );

}

