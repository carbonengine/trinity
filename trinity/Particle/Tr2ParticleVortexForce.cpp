////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#include "StdAfx.h"
#include "Tr2ParticleVortexForce.h"

Tr2ParticleVortexForce::Tr2ParticleVortexForce( IRoot* lockobj ):
	m_magnitude( 1.f ),
	m_position( 0.f, 0.f, 0.f ),
	m_axis( 0.f, 1.f, 0.f )
{
}

Tr2ParticleVortexForce::~Tr2ParticleVortexForce()
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
XMVECTOR Tr2ParticleVortexForce::GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass )
{
	XMVECTOR direction = XMVectorSubtract( m_position, position );
	direction = XMVector3Cross( direction, m_axis );

	XMVECTOR length = XMVector3ReciprocalLengthEst( direction );
	XMVECTOR isInf = XMVectorEqual( length, g_XMInfinity );
	direction = XMVectorSelect( XMVectorMultiply( direction, length ), g_XMZero, isInf );

	return XMVectorScale( direction, m_magnitude );
}