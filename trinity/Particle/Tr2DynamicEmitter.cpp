// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2DynamicEmitter.h"
#include "Tr2ParticleSystem.h"
#include "ITr2AttributeGenerator.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2DynamicEmitter default constructor
// --------------------------------------------------------------------------------------
Tr2DynamicEmitter::Tr2DynamicEmitter( IRoot* lockobj ) :
	PARENTLOCK( m_generators ),
	m_isValid( false ),
	m_isThreadSafe( false ),
	m_declarationHash( 0 ),
	m_rate( 0.0f ),
	m_accumulatedRate( 0.0f ),
	m_lastUpdate( 0 ),
	m_lastEmitterPos( 0.f, 0.f, 0.f, 0.f ),
	m_maxParticles( -1 ),
	m_emittedParticles( 0 )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2DynamicEmitter destructor
// --------------------------------------------------------------------------------------
Tr2DynamicEmitter::~Tr2DynamicEmitter()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface. Re-binds emitter to the particle system if it is
//   provided.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2DynamicEmitter::Initialize()
{
	if( m_particleSystem )
	{
		Rebind();
		if( m_isThreadSafe )
		{
			m_particleSystem->SetThreadSafeFlag();
		}
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements INotify interface.  Allows the emitter to respond to parameter changes
//   generated in Python.  If the particle system changes, the emitter re-binds to it.
// Arguments:
//   value - The Blue-exposed parameter that changed
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2DynamicEmitter::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_particleSystem ) )
	{
		Rebind();
		if( m_isThreadSafe && m_particleSystem )
		{
			m_particleSystem->SetThreadSafeFlag();
		}
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter. Notifies the linked particle system that it needs
//   additional syncronization when inserting particles.
// --------------------------------------------------------------------------------------
void Tr2DynamicEmitter::SetThreadSafeFlag()
{
	m_isThreadSafe = true;
	if( m_particleSystem )
	{
		m_particleSystem->SetThreadSafeFlag();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns if the emitter is sucessfully bound to the particle system.
// Return Value:
//   true If the emitter is sucessfully bound to the particle system
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2DynamicEmitter::IsValid() const
{
	return m_isValid;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter interface. Spawns particles every frame at a constant
//   rate.
// Arguments:
//   arguments - Update arguments
// --------------------------------------------------------------------------------------
void Tr2DynamicEmitter::Update( const UpdateArguments& arguments )
{
	if( !m_isValid || m_particleSystem == nullptr )
	{
		return;
	}
	if( m_lastUpdate == 0 )
	{
		m_lastUpdate = arguments.time;
	}
	float dt = std::min( TimeAsFloat( arguments.time - m_lastUpdate ), 0.3f );
	m_lastUpdate = arguments.time;

	UpdateSimulation( dt * arguments.emitCountFactor );
}

// --------------------------------------------------------------------------------------
// Description:
//   Spawns particles for a  frame of given length.
// Arguments:
//   dt - frame duraction (in seconds)
// --------------------------------------------------------------------------------------
void Tr2DynamicEmitter::UpdateSimulation( float dt )
{
	SpawnParticles( UpdateArguments(), nullptr, nullptr, dt );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter interface. Spawns particles.
// Arguments:
//   arguments - Update arguments
//   position - Position of the "parent" particle (if the emitter owning this generator
//		is "emit during life" or "emit on death" emitter and parent particle has
//		position element); otherwise is nullptr.
//   velocity - Velocity of the "parent" particle (if the emitter owning this generator
//		is "emit during life" or "emit on death" emitter and parent particle has
//		velocity element); otherwise is nullptr.
//   rateModifier - Modifies the number of particles spawned as opposed to emitter's
//		defined rate value.
// --------------------------------------------------------------------------------------
void Tr2DynamicEmitter::SpawnParticles( const UpdateArguments& arguments,
										const Vector3* position,
										const Vector3* velocity,
										float rateModifier )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_isValid || m_particleSystem == nullptr )
	{
		return;
	}
	if( m_declarationHash != m_particleSystem->GetElementDeclarationHash() )
	{
		m_isValid = false;
		return;
	}

	// spawn rate
	if( m_rate <= 0 )
	{
		return;
	}
	m_accumulatedRate += m_rate * rateModifier;
	int32_t count = int32_t( m_accumulatedRate );
	m_accumulatedRate -= floor( m_accumulatedRate );
	if( m_maxParticles >= 0 && int32_t( m_emittedParticles ) + count > m_maxParticles )
	{
		count = std::max( m_maxParticles - int32_t( m_emittedParticles ), 0 );
	}
	m_emittedParticles += count;

	// update position if we have any
	Vector3 transformedEmitterPos( 0.f, 0.f, 0.f );
	if( position )
	{
		transformedEmitterPos = *position;
	}

	float* particle[Tr2ParticleElementData::COUNT];
	for( int32_t i = 0; i < count; ++i )
	{
		if( !m_particleSystem->InsertParticle( particle ) )
		{
			// Particle system is full
			return;
		}

		// build an interpolated position "between frames"
		XMFLOAT4A interpolatedEmitterPos( transformedEmitterPos.x, transformedEmitterPos.y, transformedEmitterPos.z, 1.f );

		// let every attached generator have it's way with the new particle
		for( auto it = m_generators.begin(); it != m_generators.end(); ++it )
		{
			( *it )->Generate( reinterpret_cast<Vector3*>( &interpolatedEmitterPos ), velocity, particle );
		}
		m_particleSystem->DoneInsertingParticle();
	}

	// current pos will become last pos
	m_lastEmitterPos = Vector4( transformedEmitterPos, 1.f );
}


void Tr2DynamicEmitter::SpawnParticles( const UpdateArguments& arguments,
										const Vector3* positionStart,
										const Vector3* positionEnd,
										const Vector3* velocityStart,
										const Vector3* velocityEnd,
										float deltaTime )
{
	SpawnParticles( arguments, positionEnd, velocityEnd, deltaTime );
}

// --------------------------------------------------------------------------------------
// Description:
//   Re-binds the emitter to its particle system. Tries to bind each generator to the
//   corresponding element. IsValid can be checked after Rebind to see if the binding
//   succeeded.
// --------------------------------------------------------------------------------------
void Tr2DynamicEmitter::Rebind()
{
	m_emittedParticles = 0;
	m_isValid = false;
	if( m_particleSystem == nullptr )
	{
		return;
	}
	const Tr2ParticleElementDataMap& declaration = m_particleSystem->GetElementDeclaration();

	std::set<Tr2ParticleElementDeclarationName> boundElements;
	for( auto it = m_generators.begin(); it != m_generators.end(); ++it )
	{
		if( !( *it )->Bind( declaration, boundElements ) )
		{
			return;
		}
	}
	for( auto it = declaration.begin(); it != declaration.end(); ++it )
	{
		if( boundElements.find( it->first ) == boundElements.end() )
		{
			CCP_LOGERR( "Unbound particle element %s in a particle emitter", it->first.GetName().c_str() );
			return;
		}
	}
	m_isValid = true;
	m_declarationHash = m_particleSystem->GetElementDeclarationHash();
}

// --------------------------------------------------------------------------------------
// Description:
//   Resets the number of emitted particles.
// --------------------------------------------------------------------------------------
void Tr2DynamicEmitter::ResetEmittedParticleCount()
{
	m_emittedParticles = 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the number of emitted particles.
// --------------------------------------------------------------------------------------
uint32_t Tr2DynamicEmitter::GetEmittedParticleCount() const
{
	return m_emittedParticles;
}