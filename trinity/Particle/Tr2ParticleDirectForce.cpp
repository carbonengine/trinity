////////////////////////////////////////////////////////////
//
//    Created:   September 2010
//    Copyright: CCP 2010
//
//    Refactored from EveParticleDirectForce.cpp

#include "StdAfx.h"
#include "Tr2ParticleDirectForce.h"
#include "../Include/ITr2DebugRenderer2.h"

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

void Tr2ParticleDirectForce::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldTransform, const CcpMath::AxisAlignedBox& aabb ) const
{
	auto p0 = TransformCoord( Vector3( 0, 0, 0 ), worldTransform );
	auto p1 = TransformCoord( m_force, worldTransform );
	auto len = Length( p0 - p1 );
	renderer.DrawArrow( this, p0, p1, std::max( len * 0.1f, 0.1f ), 0.4f, 10, ITr2DebugRenderer2::Wireframe, 0xffaaaa00 );
}
