////////////////////////////////////////////////////////////
//
//    Created:   July 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2ForceSphereVolume.h"

Tr2ForceSphereVolume::Tr2ForceSphereVolume( IRoot* lockobj )
	:PARENTLOCK( m_forces ),
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
	
	if( XMVectorGetX( XMVectorLess( distance, radius ) ) )
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
					XMVectorMultiply( distance, XMVectorReciprocal( radius ) )
					),
				XMVectorReplicate( m_exponent )
				)
			);
	}
	return force;
}
