// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "Tr2ParticleSpring.h"

#ifdef BLUE_USE_LOCAL_ITr2DebugRenderer2
// This is only needed for py2 as the file now belongs in blue.
// Unfortunatly the blue py2 branch cannot be updated at present due to security vulnerability work.
// The file version in the older blue versions had diverged from this one is incompatible.
#include "Include/ITr2DebugRenderer2.h"
#else
#include <ITr2DebugRenderer2.h>
#endif

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

void Tr2ParticleSpring::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldTransform, const CcpMath::AxisAlignedBox& aabb ) const
{
	auto center = TransformCoord( m_position, worldTransform );
	auto size = 10.f;
	renderer.DrawLine( this, center - Vector3( size, size, size ), center + Vector3( size, size, size ), 0xffaaaa00 );
	renderer.DrawLine( this, center - Vector3( -size, size, size ), center + Vector3( -size, size, size ), 0xffaaaa00 );
	renderer.DrawLine( this, center - Vector3( size, -size, size ), center + Vector3( size, -size, size ), 0xffaaaa00 );
	renderer.DrawLine( this, center - Vector3( size, size, -size ), center + Vector3( size, size, -size ), 0xffaaaa00 );
}
