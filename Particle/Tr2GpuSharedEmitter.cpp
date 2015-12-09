////////////////////////////////////////////////////////////
//
//    Created:   October 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "Tr2GpuSharedEmitter.h"

namespace
{
const float MAXIMUM_FRAME_TIME = 1.f / 15.f;
}

Tr2GpuSharedEmitter::Tr2GpuSharedEmitter( IRoot* lockObj )
	:m_rate( 0 ),
	m_id( 0 ),
	m_paramsHash( 0 ),
	m_previousTime( -1 ),
	m_carryOver( 0 ),
	m_emissionDensity( 0.f ),
	m_maxDensity( 10000.f ),
	m_maxDisplacement( 1000.f ),
	m_inheritVelocity( 1.f ),
	m_position( 0.f, 0.f, 0.f ),
	m_direction( 0.f, 1.f, 0.f ),
	m_prevPosition( 0.f, 0.f, 0.f ),
	m_continuousEmitter( true ),
	m_enabled( true )
{
	memset( &m_emitter, 0, sizeof( m_emitter ) );
	memset( &m_params, 0, sizeof( m_params ) );
	m_params.turbulenceFrequency = 1;
}


bool Tr2GpuSharedEmitter::Initialize()
{
	UpdateHash();
	GenerateID();
	return true;
}

bool Tr2GpuSharedEmitter::OnModified( Be::Var* )
{
	UpdateHash();
	GenerateID();
	return true;
}

void Tr2GpuSharedEmitter::Enable( bool enable )
{
	m_enabled = enable;
	if( !enable )
	{
		m_previousTime = -1;
	}
}

uintptr_t Tr2GpuSharedEmitter::GetHash( const Tr2GpuParticleSystem::EmitterParams& params ) const
{
	return CcpHashFNV1( &params, sizeof( params ) );
}

void Tr2GpuSharedEmitter::UpdateHash()
{
	m_paramsHash = GetHash( m_params );
}

void Tr2GpuSharedEmitter::GenerateID()
{
	m_id = GetID( m_paramsHash );
}

uintptr_t Tr2GpuSharedEmitter::GetID( uintptr_t hash ) const
{
	return hash & ~( 1 << ( sizeof( uintptr_t ) - 1 ) );
}

// --------------------------------------------------------------------------------
// Description:
//   Simple setter to access direction
// --------------------------------------------------------------------------------
void Tr2GpuSharedEmitter::SetDirection( const Vector3* direction )
{
	m_direction = *direction;
}

// --------------------------------------------------------------------------------
// Description:
//   Simple setter to access position
// --------------------------------------------------------------------------------
void Tr2GpuSharedEmitter::SetPosition( const Vector3* position )
{
	m_position = *position;
}

void Tr2GpuSharedEmitter::Update( const UpdateArguments& arguments )
{
	if( !arguments.system )
	{
		m_previousTime = Be::Time( -1 );
		return;
	}
	if( !m_enabled )
	{
		return;
	}
	const bool firstUpdate = m_previousTime == Be::Time( -1 );
	float dt = firstUpdate ? 0 : TimeAsFloat( arguments.time - m_previousTime );
	m_previousTime = arguments.time;

	Vector3 position( XMVector3TransformCoord( m_position, arguments.parentTransform ) );
	Vector3 velocity = Vector3( 0.f, 0.f, 0.f );
	if( !firstUpdate )
	{
		velocity = ( position - m_prevPosition - arguments.originShift ) / dt;
	}
	else
	{
		m_prevPosition = position - arguments.originShift;
	}

	if( m_continuousEmitter )
	{
		m_carryOver = SpawnParticles( arguments, m_prevPosition + arguments.originShift, position, m_prevVelocity, velocity, m_carryOver, std::min( dt, MAXIMUM_FRAME_TIME ) );
	}

	m_prevPosition = position;
	m_prevVelocity = velocity;
}

void Tr2GpuSharedEmitter::SpawnParticles( const UpdateArguments& arguments, const Vector3* position, const Vector3* velocity, float rateModifier )
{
	if( !arguments.system || !m_enabled )
	{
		return;
	}
	Vector3 pos( XMVector3TransformCoord( position ? *position : m_position, arguments.parentTransform ) );
	Vector3 vel = Vector3( 0.f, 0.f, 0.f );
	if( velocity )
	{
		vel = XMVector3TransformNormal( *velocity, arguments.parentTransform );
	}
	SpawnParticles( arguments, pos, pos, vel, vel, 0.f, rateModifier );
}

void Tr2GpuSharedEmitter::SpawnParticles( 
	const UpdateArguments& arguments,
	const Vector3 *positionStart, const Vector3 *positionEnd,
	const Vector3 *velocityStart, const Vector3 *velocityEnd,
	float deltaTime )
{
	if( !arguments.system || !m_enabled )
	{
		return;
	}
	Vector3 posEnd( XMVector3TransformCoord( positionEnd ? *positionEnd : m_position, arguments.parentTransform ) );
	Vector3 posStart( positionStart ? Vector3( XMVector3TransformCoord( *positionStart, arguments.parentTransform ) ) - arguments.originShift : posEnd );
	Vector3 velStart, velEnd;
	if( velocityStart && velocityEnd )
	{
		velStart = XMVector3TransformNormal( *velocityStart, arguments.parentTransform );
		velEnd = XMVector3TransformNormal( *velocityEnd, arguments.parentTransform ) - arguments.originShift;
	}
	else
	{
		velStart = velEnd = Vector3( 0.f, 0.f, 0.f );
	}
	m_carryOver = SpawnParticles( arguments, posStart, posEnd, velStart, velEnd, m_carryOver, std::min( deltaTime, MAXIMUM_FRAME_TIME ) );
}

float Tr2GpuSharedEmitter::SpawnParticles( 
	const UpdateArguments& arguments,
	const Vector3& positionStart, const Vector3& positionEnd,
	const Vector3& velocityStart, const Vector3& velocityEnd,
	float carryOverCount,
	float deltaTime )
{
	m_emitter.position = positionEnd;

	float total = carryOverCount + deltaTime * m_rate;

	Vector3 move = m_emitter.position - positionStart;
	float moveLength = XMVectorGetX( XMVector3Length( move ) );
	if( moveLength > m_maxDisplacement )
	{
		return 0;
	}

	if( m_emissionDensity > 0 )
	{
		total += std::min( m_maxDensity, moveLength * m_emissionDensity );
	}

	carryOverCount = total - std::floor( total );
	m_emitter.count = int( total );

	if( m_emitter.count )
	{
		m_emitter.positionPrevious = positionStart;

		m_emitter.velocity = velocityEnd * m_inheritVelocity;
		m_emitter.velocityPrevious = velocityStart * m_inheritVelocity;

		m_emitter.directionPrevious = m_emitter.direction;
		m_emitter.direction = XMVector3TransformNormal( m_direction, arguments.parentTransform );
		arguments.system->Emit( m_emitter, m_id, m_paramsHash, m_params );
	}
	return carryOverCount;
}

void Tr2GpuSharedEmitter::SpawnOnce( const UpdateArguments& arguments, const Vector3& velocity, float scale, float rateModifier)
{
	if( !arguments.system || !m_enabled )
	{
		return;
	}

	auto emitter = m_emitter;
	emitter.count = int( m_rate * rateModifier );

	if( emitter.count )
	{
		emitter.radius *= scale;
		emitter.minSpeed *= scale;
		emitter.maxSpeed *= scale;

		emitter.position = XMVector3TransformCoord( m_position, arguments.parentTransform );
		emitter.positionPrevious = emitter.position;

		emitter.velocity = velocity;
		emitter.velocityPrevious = emitter.velocity;

		emitter.direction = XMVector3TransformNormal( m_direction, arguments.parentTransform );
		emitter.directionPrevious = emitter.direction;

		auto id = m_id;
		auto hash = m_paramsHash;
		auto params = m_params;
		if( scale != 1.0f )
		{
			params.sizes *= scale;
			params.turbulenceAmplitude *= scale;
			params.turbulenceFrequency = (uint32_t)( (float)params.turbulenceFrequency / scale );

			hash = GetHash( params );
			id = GetID( hash );
		}

		arguments.system->Emit( emitter, id, hash, params );
	}
}