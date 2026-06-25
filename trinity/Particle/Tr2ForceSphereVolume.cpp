// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2ForceSphereVolume.h"

#ifdef BLUE_USE_LOCAL_ITr2DebugRenderer2
// This is only needed for py2 as the file now belongs in blue.
// Unfortunatly the blue py2 branch cannot be updated at present due to security vulnerability work.
// The file version in the older blue versions had diverged from this one is incompatible.
#include "Include/ITr2DebugRenderer2.h"
#else
#include <ITr2DebugRenderer2.h>
#endif

Tr2ForceSphereVolume::Tr2ForceSphereVolume( IRoot* lockobj ) :
	PARENTLOCK( m_forces ),
	m_position( 0.f, 0.f, 0.f ),
	m_radius( 1.f ),
	m_exponent( 1.f )
{
}

Tr2ForceSphereVolume::~Tr2ForceSphereVolume()
{
}

// -------------------------------------------------------------
// Description:
//   Applies a forces to a particle limited by sphere volume.
// Arguments:
//   position - Particle position
//   velocity - Particle velocity
//   dt - Frame time
//   mass - Particle mass
// Return value:
//   A force to apply to a particle
// -------------------------------------------------------------
XMVECTOR FASTCALL Tr2ForceSphereVolume::GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass )
{
	XMVECTOR force = XMVectorReplicate( 0.0f );
	XMVECTOR distance = XMVector3Length( XMVectorSubtract( position, m_position ) );
	XMVECTOR radius = XMVectorReplicate( m_radius );

	if( XMVectorGetIntX( XMVectorLess( distance, radius ) ) )
	{
		for( auto it = m_forces.begin(); it != m_forces.end(); ++it )
		{
			force = XMVectorAdd( force, ( *it )->GetForce( position, velocity, dt, mass ) );
		}
		force = XMVectorMultiply(
			force,
			XMVectorPow(
				XMVectorSubtract(
					XMVectorReplicate( 1.f ),
					XMVectorMultiply( distance, XMVectorReciprocal( radius ) ) ),
				XMVectorReplicate( m_exponent ) ) );
	}
	return force;
}

void Tr2ForceSphereVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldTransform, const CcpMath::AxisAlignedBox& aabb ) const
{
	renderer.DrawSphere( this, worldTransform, m_position, m_radius, 20, ITr2DebugRenderer2::Wireframe, 0xffaaaa00 );
	for( auto& force : m_forces )
	{
		force->RenderDebugInfo( renderer, worldTransform, aabb );
	}
}
