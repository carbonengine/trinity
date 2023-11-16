////////////////////////////////////////////////////////////
//
//    Created:   July 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2SphereConstraint.h"
#include "ITr2AttributeGenerator.h"
#include "Tr2ParticleSystem.h"
#include "ITr2GenericEmitter.h"
#include "TbbStub.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2SphereConstraint default constructor
// --------------------------------------------------------------------------------------
Tr2SphereConstraint::Tr2SphereConstraint( IRoot* lockobj )
:	m_position( 0.f, 0.f, 0.f ),
	m_radius( 1.f ),
	m_invertSphere( false ),
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
//   Implements IInitialize interface. 
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2SphereConstraint::Initialize()
{
	for( auto it = m_onCollisionEmitters.begin(); it != m_onCollisionEmitters.end(); ++it )
	{
		( *it )->SetThreadSafeFlag();
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
void Tr2SphereConstraint::OnListModified(
	long event,
	ssize_t,
	ssize_t,
	IRoot* value,
	const IList* theList
	)
{
	if( (event & BELIST_LOADING) == 0 && ( event & BELIST_EVENTMASK ) == BELIST_INSERTED && theList == &m_onCollisionEmitters && value )
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
//   particle and a sphere. If collision is detected optionally
//   applies physical response to particle position and/or velocity and run generators
//   to re-generate particle data components. This method can be called asyncronously.
// Arguments:
//   arguments - Child emitter update arguments
//   particles - Particle data: Tr2ParticleElementData::COUNT of float arrays. The 
//		constraint can modify any data element of a particle. 
//   strides - Sizes of particle data in each of "particles" arrays (in floats).
//   count - Number of particles.
//   dt - Frame time.
// --------------------------------------------------------------------------------------
void Tr2SphereConstraint::ApplyConstraint( const ITr2GenericEmitter::UpdateArguments& arguments, float** particles, unsigned* strides, unsigned count, float dt )
{
	if( !m_isValid )
	{
		return;
	}

	Tr2ParallelFor( 
		Tr2BlockedRange<size_t>( 0, count, 100 ), 
		[=]( const Tr2BlockedRange<size_t>& range ) -> void
	{
		XMVECTOR refPosition = m_position;
		XMVECTOR refRadius = XMVectorReplicate( m_radius );
		XMVECTOR invert = XMVectorReplicate( m_invertSphere ? -1.f : 1.f );
		XMVECTOR radiusCmp = XMVectorMultiply( invert, XMVectorMultiply( refRadius, refRadius ) );
		XMVECTOR particleRadiusCoefficient = m_particleRadiusCoefficient;

		Tr2ParticleStreamIterator<XMFLOAT3> position( particles, strides, m_positionElement );
		position += int( range.begin() );
		Tr2ParticleStreamIterator<XMFLOAT4> radiusStream( particles, strides, m_radiusElement );
		radiusStream += int( range.begin() );
		Tr2ParticleStreamIterator<XMFLOAT3> velocity( particles, strides, m_velocityElement );
		velocity += int( range.begin() );

		for( auto i = range.begin(); i < range.end(); ++i, ++position, ++radiusStream, ++velocity )
		{
			XMVECTOR radius = refRadius;
			if( m_radiusElement.m_offset != -1 )
			{
				if( m_invertSphere )
				{
					radius = XMVectorSubtract( radius, XMVector4Dot( 
						XMLoadFloat4( radiusStream ), 
						particleRadiusCoefficient ) );
				}
				else
				{
					radius = XMVectorAdd( radius, XMVector4Dot( 
						XMLoadFloat4( radiusStream ), 
						particleRadiusCoefficient ) );
				}
			}
			XMVECTOR curPosition = XMLoadFloat3( position );
			XMVECTOR curVelocity = XMLoadFloat3( velocity );
			XMVECTOR offset = XMVectorSubtract( curPosition, refPosition );
			XMVECTOR offsetLengthSq = XMVector3LengthSq( offset );

			// Check if the particle is inside the "prohibited space"
			if( XMVectorGetIntX( XMVectorLess( XMVectorMultiply( offsetLengthSq, invert ), radiusCmp ) ) )
			{
				if( m_affectPosition )
				{
					offset = XMVectorMultiply( offset, XMVectorReciprocal( XMVectorSqrt( offsetLengthSq ) ) );
					XMVECTOR newPosition = XMVectorMultiplyAdd( offset, radius, refPosition );
					XMStoreFloat3( position, newPosition );

					if( m_affectVelocity && m_velocityElement.m_offset != -1 )
					{
						offset = XMVectorMultiply( offset, invert );

						XMVECTOR velocityDot = XMVector3Dot( curVelocity, offset );
						if( XMVectorGetIntX( XMVectorLess( velocityDot, XMVectorZero() ) ) )
						{
							XMVECTOR bounce = XMVectorMultiply( offset, XMVectorNegate( velocityDot ) );
							XMVECTOR slide = XMVectorAdd( curVelocity, bounce );
							bounce = XMVectorScale( bounce, m_elasticity );
							slide = XMVectorScale( slide, m_friction );
							XMStoreFloat3( velocity, XMVectorAdd( bounce, slide ) );
							if( m_reflectionNoise > 0 )
							{
								XMVECTOR reflectionNoise;
								reflectionNoise = Vector3( 
									Tr2ParticleSystem::RandFloat() * 2 - 1, 
									Tr2ParticleSystem::RandFloat() * 2 - 1, 
									Tr2ParticleSystem::RandFloat() * 2 - 1 );
								reflectionNoise = XMVectorScale( reflectionNoise, m_reflectionNoise );
								reflectionNoise = XMVectorSubtract( reflectionNoise, XMVectorMultiply( offset, XMVector3Dot( reflectionNoise, offset ) ) );
								XMVECTOR speed = XMVector3LengthEst( XMLoadFloat3( velocity ) );
								reflectionNoise = XMVectorMultiply( reflectionNoise, speed );
								XMStoreFloat3( velocity, reflectionNoise );
							}
						}
					}
				}
			}
			else if( m_velocityElement.m_offset != -1 )
			{
				float a = XMVectorGetX( XMVector3Dot( curVelocity, curVelocity ) );
				float b = XMVectorGetX( XMVector3Dot( curVelocity, offset ) ) * 2.f;
				float c = XMVectorGetX( XMVector3Dot( offset, offset ) ) - m_radius * m_radius;

				float det = b * b - 4.f * a * c;
				if( det < 0.f || a == 0.f )
				{
					continue;
				}
				float t;
				if( m_invertSphere )
				{
					t = ( -b + sqrt( det ) ) / ( 2.f * a );
				}
				else
				{
					t = ( -b - sqrt( det ) ) / ( 2.f * a );
				}
				if( t > dt || t < 0.f )
				{
					continue;
				}

				if( m_affectPosition )
				{
					XMVECTOR newPosition = XMVectorMultiplyAdd( curVelocity, XMVectorReplicate( t ), curPosition );
					XMStoreFloat3( position, newPosition );

					if( m_affectVelocity )
					{
						offset = XMVectorSubtract( newPosition, m_position );
						offset = XMVector3Normalize( offset );
						offset = XMVectorMultiply( offset, invert );

						XMVECTOR velocityDot = XMVector3Dot( curVelocity, offset );
						if( XMVectorGetIntX( XMVectorLess( velocityDot, XMVectorZero() ) ) )
						{
							XMVECTOR bounce = XMVectorMultiply( offset, XMVectorNegate( velocityDot ) );
							XMVECTOR slide = XMVectorAdd( curVelocity, bounce );
							bounce = XMVectorScale( bounce, m_elasticity );
							slide = XMVectorScale( slide, m_friction );
							XMStoreFloat3( velocity, XMVectorAdd( bounce, slide ) );
							if( m_reflectionNoise > 0 )
							{
								XMVECTOR reflectionNoise;
								reflectionNoise = Vector3( 
									Tr2ParticleSystem::RandFloat() * 2 - 1, 
									Tr2ParticleSystem::RandFloat() * 2 - 1, 
									Tr2ParticleSystem::RandFloat() * 2 - 1 );
								reflectionNoise = XMVectorScale( reflectionNoise, m_reflectionNoise );
								reflectionNoise = XMVectorSubtract( reflectionNoise, XMVectorMultiply( offset, XMVector3Dot( reflectionNoise, offset ) ) );
								XMVECTOR speed = XMVector3LengthEst( curVelocity );
								reflectionNoise = XMVectorMultiply( reflectionNoise, speed );
								XMStoreFloat3( velocity, reflectionNoise );
							}
						}
					}
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
					( *it )->Generate( reinterpret_cast<Vector3*>( position.Get() ), 
									   m_velocityElement.m_offset != -1 ? reinterpret_cast<Vector3*>( velocity.Get() ) : nullptr, 
									   particle );
				}
			}
			for( auto it = m_onCollisionEmitters.begin(); it != m_onCollisionEmitters.end(); ++it )
			{
				( *it )->SpawnParticles( arguments, reinterpret_cast<Vector3*>( position.Get() ), 
										 m_velocityElement.m_offset != -1 ? reinterpret_cast<Vector3*>( velocity.Get() ) : nullptr );
			}
		}
	} );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericParticleConstraint interface. A chance for constraint to bind 
//   itself to a particle system. Called when constaint is added to the particle system 
//   or when system is re-binded.
// Arguments:
//   system - Particle system the constaint is attached to.
// --------------------------------------------------------------------------------------
void Tr2SphereConstraint::Bind( Tr2ParticleSystem* system )
{
	m_isValid = false;
	const Tr2ParticleElementDataMap &declaration = system->GetElementDeclaration();

	Tr2ParticleElementDeclarationName position( Tr2ParticleElementDeclarationName::POSITION );
	auto i = declaration.find( position );
	if( i == declaration.end() )
	{
		CCP_LOGERR( "Tr2SphereConstraint needs POSITION particle element in the system" );
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
			CCP_LOGERR( "Tr2SphereConstraint can not find particle size component \"%s\" in thesystem", m_particleRadiusComponent.c_str() );
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
