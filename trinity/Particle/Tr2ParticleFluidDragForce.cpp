// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2ParticleFluidDragForce.h"

Tr2ParticleFluidDragForce::Tr2ParticleFluidDragForce( IRoot* lockobj ) :
	m_dragConstant( 1.f )
{
}

Tr2ParticleFluidDragForce::~Tr2ParticleFluidDragForce()
{
}

// -------------------------------------------------------------
// Description:
//   Applies a force to a particle.
// Arguments:
//   position - Particle position
//   velocity - Particle velocity (not used)
//   dt - Frame time
//   mass - Particle mass
// Return value:
//   A force to apply to a particle
// -------------------------------------------------------------
XMVECTOR Tr2ParticleFluidDragForce::GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass )
{
	XMVECTOR force = XMVectorMultiply( velocity, XMVectorScale( XMVector3LengthEst( velocity ), -m_dragConstant ) );
	XMVECTOR newVelocity = XMVectorAdd( velocity, XMVectorScale( force, dt * mass ) );
	XMVECTOR comparison = XMVectorLess( XMVector3Dot( newVelocity, velocity ), XMVectorZero() );
	XMVECTOR maxForce = XMVectorScale( velocity, -1.0f / ( dt * mass ) );
	return XMVectorSelect( force, maxForce, comparison );
}