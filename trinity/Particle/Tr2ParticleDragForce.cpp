// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "Tr2ParticleDragForce.h"

Tr2ParticleDragForce::Tr2ParticleDragForce( IRoot* lockobj ) :
	m_dragConstant( 0.1f )
{
}

Tr2ParticleDragForce::~Tr2ParticleDragForce()
{
}

// -------------------------------------------------------------
// Description:
//   Applies drag force to a particle.
// Arguments:
//   position - Particle position (not used)
//   velocity - Particle velocity
//   dt - Frame time (not used)
//   mass - Particle mass (not used)
// Return value:
//   Drag force to apply to a particle
// -------------------------------------------------------------
XMVECTOR Tr2ParticleDragForce::GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass )
{
	return XMVectorScale( velocity, -m_dragConstant );
}