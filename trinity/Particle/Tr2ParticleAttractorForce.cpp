////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#include "StdAfx.h"
#include "Tr2ParticleAttractorForce.h"

Tr2ParticleAttractorForce::Tr2ParticleAttractorForce( IRoot* lockobj ):
	m_magnitude( 1.f ),
	m_position( 0.f, 0.f, 0.f )
{
}

Tr2ParticleAttractorForce::~Tr2ParticleAttractorForce()
{
}

// -------------------------------------------------------------
// Description:
//   Applies a force to a particle.
// Arguments:
//   position - Particle position
//   velocity - Particle velocity (not used)
//   dt - Frame time (not used)
//   mass - Particle mass (not used)
// Return value:
//   A force to apply to a particle
// -------------------------------------------------------------
XMVECTOR Tr2ParticleAttractorForce::GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass )
{
	XMVECTOR direction = XMVectorSubtract( m_position, position );
	XMVECTOR length = XMVector3ReciprocalLengthEst( direction );
	XMVECTOR isInf = XMVectorEqual( length, g_XMInfinity );
	direction = XMVectorSelect( XMVectorMultiply( direction, length ), g_XMZero, isInf );
	return XMVectorScale( direction, m_magnitude );
}