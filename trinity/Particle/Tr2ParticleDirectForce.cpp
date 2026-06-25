// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "Tr2ParticleDirectForce.h"

#ifdef BLUE_USE_LOCAL_ITr2DebugRenderer2
// This is only needed for py2 as the file now belongs in blue.
// Unfortunatly the blue py2 branch cannot be updated at present due to security vulnerability work.
// The file version in the older blue versions had diverged from this one is incompatible.
#include "Include/ITr2DebugRenderer2.h"
#else
#include <ITr2DebugRenderer2.h>
#endif

Tr2ParticleDirectForce::Tr2ParticleDirectForce( IRoot* lockobj ) :
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

void Tr2ParticleDirectForce::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldTransform, const CcpMath::AxisAlignedBox& aabb ) const
{
	auto p0 = TransformCoord( Vector3( 0, 0, 0 ), worldTransform );
	auto p1 = TransformCoord( m_force, worldTransform );
	auto len = Length( p0 - p1 );
	renderer.DrawArrow( this, p0, p1, std::max( len * 0.1f, 0.1f ), 0.4f, 10, ITr2DebugRenderer2::Wireframe, 0xffaaaa00 );
}
