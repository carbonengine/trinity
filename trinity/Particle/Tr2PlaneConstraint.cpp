// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2PlaneConstraint.h"
#include "ITr2AttributeGenerator.h"
#include "Tr2ParticleSystem.h"
#include "ITr2GenericEmitter.h"

#ifdef BLUE_USE_LOCAL_ITr2DebugRenderer2
// This is only needed for py2 as the file now belongs in blue.
// Unfortunatly the blue py2 branch cannot be updated at present due to security vulnerability work.
// The file version in the older blue versions had diverged from this one is incompatible.
#include "Include/ITr2DebugRenderer2.h"
#else
#include <ITr2DebugRenderer2.h>
#endif

// --------------------------------------------------------------------------------------
// Description:
//   Tr2PlaneConstraint default constructor
// --------------------------------------------------------------------------------------
Tr2PlaneConstraint::Tr2PlaneConstraint( IRoot* lockobj ) :
	m_plane( 0.f, 1.f, 0.f, 0.f ),
	m_normalizedPlane( 0.f, 1.f, 0.f, 0.f ),
	m_friction( 1.f ),
	m_elasticity( 1.f ),
	m_reflectionNoise( 0.f ),
	m_particleRadiusCoefficient( 1.f, 0.f, 0.f, 0.f ),
	m_affectPosition( true ),
	m_affectVelocity( true ),
	m_isValid( false ),
	PARENTLOCK( m_generators ),
	PARENTLOCK( m_onCollisionEmitters )
{
	m_positionElement.m_offset = -1;
	m_velocityElement.m_offset = -1;
	m_radiusElement.m_offset = -1;

	m_onCollisionEmitters.SetNotify( this );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface. Updates normalized plane value.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2PlaneConstraint::Initialize()
{
	m_normalizedPlane = Normalize( reinterpret_cast<Plane&>( m_plane ) );
	for( auto it = m_onCollisionEmitters.begin(); it != m_onCollisionEmitters.end(); ++it )
	{
		( *it )->SetThreadSafeFlag();
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements INotify interface. Updates normalized plane value if the m_plane value
//   is modified.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2PlaneConstraint::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_plane ) )
	{
		m_normalizedPlane = Normalize( reinterpret_cast<Plane&>( m_plane ) );
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IListNotify interface. Notifies inserted emitters that they are going
//   to be updated in a multi-threaded scenario.
// Arguments:
//   event - List event
//   value - List element modified
//   theList - List
// --------------------------------------------------------------------------------------
void Tr2PlaneConstraint::OnListModified(
	long event,
	ssize_t,
	ssize_t,
	IRoot* value,
	const IList* theList )
{
	if( ( event & BELIST_LOADING ) == 0 && ( event & BELIST_EVENTMASK ) == BELIST_INSERTED && theList == &m_onCollisionEmitters && value )
	{
		ITr2GenericEmitterPtr emitter( BlueCastPtr( value ) );
		if( emitter )
		{
			emitter->SetThreadSafeFlag();
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericParticleConstraint interface. Checks for collision between a
//   particle and a half-space defined by m_plane. If collision is detected optionally
//   apply physical response to particle position and/or velocity and run generators
//   to re-generate particle data components. This method can be called asyncronously.
// Arguments:
//   arguments - Child emitter update arguments
//   particles - Particle data: Tr2ParticleElementData::COUNT of float arrays. The
//		constraint can modify any data element of a particle.
//   strides - Sizes of particle data in each of "particles" arrays (in floats).
//   count - Number of particles.
//   dt - (unused) Frame time.
// --------------------------------------------------------------------------------------
void Tr2PlaneConstraint::ApplyConstraint( const ITr2GenericEmitter::UpdateArguments& arguments, float** particles, unsigned* strides, unsigned count, float /* dt */ )
{
	if( !m_isValid )
	{
		return;
	}

	Tr2ParticleStreamIterator<Vector3> position( particles, strides, m_positionElement );
	Tr2ParticleStreamIterator<Vector4> radiusStream( particles, strides, m_radiusElement );
	Tr2ParticleStreamIterator<Vector3> velocity( particles, strides, m_velocityElement );

	for( unsigned i = 0; i < count; ++i, ++position, ++radiusStream, ++velocity )
	{
		float radius = 0;
		if( m_radiusElement.m_offset != -1 )
		{
			radius = Dot( *radiusStream, m_particleRadiusCoefficient );
		}

		float dot = DotCoord( m_normalizedPlane, *position ) - radius;
		const Vector3& planeNormal = reinterpret_cast<const Vector3&>( m_normalizedPlane );
		float velocityDot = -1.f;
		if( m_velocityElement.m_offset != -1 )
		{
			velocityDot = Dot( *velocity, planeNormal );
		}
		if( dot <= 0 && velocityDot < 0 )
		{
			if( m_affectPosition )
			{
				Vector3 shift = planeNormal * dot;
				*position -= shift;
			}
			if( m_affectVelocity && m_velocityElement.m_offset != -1 )
			{
				Vector3 bounce = planeNormal * velocityDot;
				Vector3 slide = *velocity - bounce;
				bounce *= -m_elasticity;
				slide *= m_friction;
				*velocity = bounce + slide;
				if( m_reflectionNoise > 0 )
				{
					Vector3 reflectionNoise(
						Tr2ParticleSystem::RandFloat() * 2 - 1,
						Tr2ParticleSystem::RandFloat() * 2 - 1,
						Tr2ParticleSystem::RandFloat() * 2 - 1 );
					reflectionNoise *= m_reflectionNoise;
					reflectionNoise -= planeNormal * Dot( reflectionNoise, planeNormal );
					float speed = Length( *velocity );
					reflectionNoise *= speed;
					*velocity += reflectionNoise;
				}
			}
			if( !m_generators.empty() )
			{
				float* particle[Tr2ParticleElementData::COUNT];
				for( unsigned j = 0; j < Tr2ParticleElementData::COUNT; ++j )
				{
					particle[j] = particles[j] + strides[j] * i;
				}
				for( auto it = m_generators.begin(); it != m_generators.end(); ++it )
				{
					( *it )->Generate( position,
									   m_velocityElement.m_offset != -1 ? static_cast<Vector3*>( velocity ) : nullptr,
									   particle );
				}
			}
			for( auto it = m_onCollisionEmitters.begin(); it != m_onCollisionEmitters.end(); ++it )
			{
				( *it )->SpawnParticles( arguments, position, m_velocityElement.m_offset != -1 ? static_cast<Vector3*>( velocity ) : nullptr );
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericParticleConstraint interface. A chance for constraint to bind
//   itself to a particle system. Called when constaint is added to the particle system
//   or when system is re-binded.
// Arguments:
//   system - Particle system the constaint is attached to.
// --------------------------------------------------------------------------------------
void Tr2PlaneConstraint::Bind( Tr2ParticleSystem* system )
{
	m_isValid = false;
	const Tr2ParticleElementDataMap& declaration = system->GetElementDeclaration();

	Tr2ParticleElementDeclarationName position( Tr2ParticleElementDeclarationName::POSITION );
	auto i = declaration.find( position );
	if( i == declaration.end() )
	{
		CCP_LOGERR( "Tr2PlaneConstraint needs POSITION particle element in the system" );
		return;
	}
	else
	{
		m_positionElement = i->second;
	}

	Tr2ParticleElementDeclarationName velocity( Tr2ParticleElementDeclarationName::VELOCITY );
	i = declaration.find( velocity );
	if( i == declaration.end() )
	{
		m_velocityElement.m_bufferType = m_velocityElement.GPU;
		m_velocityElement.m_offset = -1;
	}
	else
	{
		m_velocityElement = i->second;
	}

	if( !m_particleRadiusComponent.empty() )
	{
		Tr2ParticleElementDeclarationName size( Tr2ParticleElementDeclarationName::CUSTOM, m_particleRadiusComponent.c_str() );
		i = declaration.find( size );
		if( i == declaration.end() )
		{
			CCP_LOGERR( "Tr2PlaneConstraint can not find particle size component \"%s\" in thesystem", m_particleRadiusComponent.c_str() );
			return;
		}
		else
		{
			m_radiusElement = i->second;
		}
	}
	else
	{
		m_radiusElement.m_bufferType = m_radiusElement.GPU;
		m_radiusElement.m_offset = -1;
	}

	std::set<Tr2ParticleElementDeclarationName> boundElements;
	for( auto it = m_generators.begin(); it != m_generators.end(); ++it )
	{
		if( !( *it )->Bind( declaration, boundElements ) )
		{
			return;
		}
	}
	m_isValid = true;
}

void Tr2PlaneConstraint::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldTransform, const CcpMath::AxisAlignedBox& aabb ) const
{
	if( !m_isValid )
	{
		return;
	}
	auto local = TranslationMatrix( 0, 0, -m_normalizedPlane.d ) * OrthoNormalBasisZ( Vector3( m_normalizedPlane.a, m_normalizedPlane.b, m_normalizedPlane.c ) );
	auto world = local * worldTransform;
	auto center = TransformCoord( aabb.Center(), Inverse( local ) );
	center.z = 0;
	float radius = std::max( 10.f, Length( aabb.Size() ) * 0.25f );
	float step = radius * 0.2f;
	for( int i = -5; i <= 5; ++i )
	{
		Vector3 p0( float( i ) * step, -radius, 0 );
		Vector3 p1( float( i ) * step, radius, 0 );
		renderer.DrawLine( this, TransformCoord( p0 + center, world ), TransformCoord( p1 + center, world ), 0xffff4444 );
	}
	for( int i = -5; i <= 5; ++i )
	{
		Vector3 p0( -radius, float( i ) * step, 0 );
		Vector3 p1( radius, float( i ) * step, 0 );
		renderer.DrawLine( this, TransformCoord( p0 + center, world ), TransformCoord( p1 + center, world ), 0xffff4444 );
	}
}