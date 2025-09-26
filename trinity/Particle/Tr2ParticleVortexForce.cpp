////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#include "StdAfx.h"
#include "Tr2ParticleVortexForce.h"
#include "../Include/ITr2DebugRenderer2.h"

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

void Tr2ParticleVortexForce::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldTransform, const CcpMath::AxisAlignedBox& aabb ) const
{
	auto world = OrthoNormalBasisZ( m_axis ) * TranslationMatrix( m_position ) * worldTransform;
	auto radius = 20.f;

	renderer.DrawLine( this, TransformCoord( Vector3( 0, 0, -radius ), world ), TransformCoord( Vector3( 0, 0, radius ), world ), 0xffaaaa00 );
	for( int i = 0; i < 16; ++i )
	{
		float angle0 = (float)i / 16.f * XM_2PI;
		float angle1 = (float)( i + 1 ) / 16.f * XM_2PI;
		Vector3 p0( cosf( angle0 ) * radius, sinf( angle0 ) * radius, 0 );
		Vector3 p1( cosf( angle1 ) * radius, sinf( angle1 ) * radius, 0 );
		renderer.DrawLine( this, TransformCoord( p0, world ), TransformCoord( p1, world ), 0xffaaaa00 );
	}
}
