#include "StdAfx.h"
#include "Tr2GPUParticleEmitter.h"
#include "Tr2GPUParticleType.h"

//===================
//Tr2GPUParticleSubEmitter
Tr2GPUParticleSubEmitter::Tr2GPUParticleSubEmitter(IRoot *lockobj) :
	m_emissionRate(10.f), m_emissionDensity(0.f), m_residualEmission(0.f),
	m_inheritVelocity(0.f),
	m_velocityScale(1.f),
	m_positionScale(1.f),
	m_previousTimeValid(false),
	m_previousTime(0),
	m_burstTime(-1.f), 
	m_burstTimeLocal(0),
	m_burstCount(0),
	m_burstCycle(0),
	m_offset(0,0,0),
	m_pool(nullptr),
	m_behaviourIndex(0)
{
}
	
// --------------------------------------------------------------------------------
// Description:
//   The core update method for sub-emitters. Will produce particles based on distance 
//   traveled in the previous frame, duration of the frame, and any periodic 'bursty'
//   emission 
// Returns:
//   Number of particles this emitter should spawn this frame.
// --------------------------------------------------------------------------------
unsigned Tr2GPUParticleSubEmitter::Update( float deltaTime, const Vector3 &translation, const float rateModifier )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	float totalParticles = m_residualEmission;
	totalParticles += std::max( 0.f,  std::min( 1.f / 30.f, deltaTime ) * m_emissionRate * rateModifier );
	totalParticles += std::max( 0.f, D3DXVec3Length( &translation ) * m_emissionDensity * rateModifier );
	
	unsigned emitted = unsigned(totalParticles);
	m_residualEmission = totalParticles - float(emitted);

	if( m_burstTime > 0.f )
	{
		if( m_burstTimeLocal + deltaTime > m_burstTime && m_burstTimeLocal <= m_burstTime )
		{
			emitted += m_burstCount;
			if( m_burstCycle > 0.f )
			{
				m_burstTimeLocal = 0;
				m_burstTime = m_burstCycle + deltaTime;
			}
		}
		m_burstTimeLocal += deltaTime;
	}

	return emitted;
}

// --------------------------------------------------------------------------------
// Description:
//   Alternative update method, used for emitOverLife/emitOnDeath type behaviour.
// Returns:
//   Number of particles this sub-emitter should spawn this frame.
// --------------------------------------------------------------------------------
unsigned Tr2GPUParticleSubEmitter::Update( Be::Time time, const Vector3 &translation, const float rateModifier )
{
	const float deltaTime = m_previousTimeValid ? (float)TimeAsDouble(time - m_previousTime) : 0.f;
	m_previousTime = time;
	m_previousTimeValid = true;
	return Update( deltaTime, translation, rateModifier );
}

//==========================
//Tr2GPUParticleEmitter
Tr2GPUParticleEmitter::Tr2GPUParticleEmitter(IRoot* lockobj) :
	PARENTLOCK( m_subEmitters ),
		PARENTLOCK( m_particleTypes ),
		m_enabled( true ),
		m_lastPosValid(false), 
		m_lastUpdatePosition(0,0,0),
		m_lastUpdateVelocity(0,0,0),
		m_lastUpdateTime( 0 ),
		m_positionScale(1.f),
		m_velocityScale(1.f),
		m_offset(0,0,0),
		m_maximumTranslationPerFrame(2000.f),
		m_translation(0,0,0),
		m_lastUpdateTimeValid(false),
		m_deltaTime(0.f)
{
	LoadResources();
}

const Matrix Tr2GPUParticleEmitter::s_identity(1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1);
	
// --------------------------------------------------------------------------------
// Description:
//   Called when Jessica etc. modifies the properties of the emitter.
// --------------------------------------------------------------------------------
bool Tr2GPUParticleEmitter::OnModified( Be::Var *val )
{
	LoadResources();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Internal helper function, loads position/velocity textures as required.
// --------------------------------------------------------------------------------
void Tr2GPUParticleEmitter::LoadResources()
{
	m_positionTexture = nullptr;
	if( !m_positionTexturePath.empty() )
	{
		BeResMan->GetResourceW( m_positionTexturePath.c_str(), L"", BlueInterfaceIID<TriTextureRes>(), (void**)&m_positionTexture );
	}

	m_velocityTexture = nullptr;
	if( !m_velocityTexturePath.empty() )
	{
		BeResMan->GetResourceW( m_velocityTexturePath.c_str(), L"", BlueInterfaceIID<TriTextureRes>(), (void**)&m_velocityTexture );
	}

	m_currentVelocityPath = m_velocityTexturePath;
	m_currentPositionPath = m_positionTexturePath;
}

void Tr2GPUParticleEmitter::ApplyPool( Tr2GPUParticlePoolManager* manager )
{
	for( auto it = m_particleTypes.begin(); it != m_particleTypes.end(); ++it )
	{
		(*it)->ApplyPool( manager );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Initialises a sub-emitter, setting its pool and behaviour indices.
// --------------------------------------------------------------------------------
void Tr2GPUParticleEmitter::SetUpEmitter( Tr2GPUParticleSubEmitter *emitter )
{
	for(auto pt = m_particleTypes.begin(); pt != m_particleTypes.end(); ++pt )
	{
		Tr2GPUParticleType *particleType = *pt;
		if( particleType->GetBehaviourName() == emitter->GetBehaviour() )
		{				
			emitter->SetPool( particleType->GetPool() );
			emitter->SetBehaviourIndex( particleType->GetBehaviourIndex() );
			break;
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Called when Jessica etc. modifies the properties of the emitter.
// --------------------------------------------------------------------------------
void Tr2GPUParticleEmitter::UpdateTransform( const Vector3 &parentPosition, const Vector3 &parentVelocity,  const Vector3 &egoLocation, const Matrix &parentT )
{
	// is this emitter enabled?
	if( !m_enabled )
	{
		return;
	}
	CCP_STATS_ZONE( __FUNCTION__ );

	const Vector3 deltaEgo( egoLocation );

	const Vector3 posNow( m_offset + parentPosition );
	const Vector3 posThen( 
		m_lastPosValid ? 
			m_lastUpdatePosition - deltaEgo :
			posNow );
	m_translation = Vector3 ( posNow - posThen );
	
	const Vector3 velNow( parentVelocity );
	const Vector3 velThen( m_lastPosValid ? m_lastUpdateVelocity : velNow );

	m_lastUpdatePosition = posNow;
	m_lastUpdateVelocity = velNow;
	m_lastPosValid = true;

	//avoid drawing trails and the like with a large translation in a single frame
	const bool skipSpawn = m_maximumTranslationPerFrame > 0.f && m_maximumTranslationPerFrame < D3DXVec3Length( (const Vector3*)&m_translation );

	for( auto it = m_particleTypes.begin(); it != m_particleTypes.end(); ++it )
	{
		(*it)->CheckForUpdate();
	}

	//scale emission density based on visibility data that may be poked on to us from
	// update per-frame data. This checks against the frustum and a projected size,
	// and causes us to emit fewer particles when in the distance or out of the frustum.
	//we clamp it to 10% of our regular emission because both view distance and frustum
	// can change rapidly and we want *something* to be rendered in these casess.
	const float visDensityScale = std::min( 1.f, std::max( 0.1f, (float)std::abs( m_visibilityBasedDensity ) ) );
	m_visibilityBasedDensity = -1.f;

	//in Jessica, we only get ticked via this function. in order to get an accurate delta-time,
	// update this if we've not been ticked via one of the Update functions that takes an
	// accurate time value.
	//sub-emitters also store their last update time, so can properly calculate the frame delta.
	if( !m_lastUpdateTimeValid )
	{
		m_lastUpdateTime = BeOS->GetCurrentFrameTime();
	}
	m_lastUpdateTimeValid = false;

	for( auto it = m_subEmitters.begin(); it != m_subEmitters.end(); ++it )
	{
		Tr2GPUParticleSubEmitter *emitter = *it;
		
		if( !emitter->GetPool() || emitter->GetBehaviourIndex() <= 0 )
		{
			SetUpEmitter( emitter );
		}

		Tr2GPUParticlePool *pool = emitter->GetPool();
		if( !pool ) continue;
		
		const unsigned toSpawnMax = emitter->Update( m_lastUpdateTime, m_translation, pool->GetUsageDensityModifier() * visDensityScale );
		const unsigned toSpawn = skipSpawn ? 0 : toSpawnMax;

		if( toSpawn > 0 )
		{
			Vector3 offset = emitter->m_offset;
			if( offset.x != 0.f || offset.y != 0.f || offset.z != 0.f )
			{
				D3DXVec3TransformNormal( &offset, &offset, &parentT );
			}

			Matrix parentTT;
			D3DXMatrixTranspose( &parentTT, &parentT );

			const int behaviour = emitter->GetBehaviourIndex();

			if( pool && behaviour > 0 )
			{
				pool->SpawnParticles( 
					toSpawn,
					behaviour,
					emitter->GetInheritedVelocity(),
					m_positionScale * emitter->GetPositionScale(),
					m_velocityScale * emitter->GetVelocityScale(),
					posThen + offset,
					posNow + offset,
					velThen,
					velNow,
					parentTT,
					m_positionTexture,
					m_velocityTexture );
			}			
		}
	}
}



void Tr2GPUParticleEmitter::SpawnParticles( const Vector3* position, const Vector3* velocity, float externalDeltaTime )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	const Vector3 pos(position ? *position : Vector3(0,0,0));
	const Vector3 vel(velocity ? *velocity : Vector3(0,0,0));
	const Vector3 translation( vel * m_deltaTime );

	const Matrix localT( 1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1 );
	
	for( auto it = m_particleTypes.begin(); it != m_particleTypes.end(); ++it )
	{
		(*it)->CheckForUpdate();
	}

	for( auto it = m_subEmitters.begin(); it != m_subEmitters.end(); ++it )
	{
		Tr2GPUParticleSubEmitter *emitter = *it;
		if( !emitter->GetPool() || emitter->GetBehaviourIndex() <= 0 )
		{
			SetUpEmitter( emitter );
		}

		Tr2GPUParticlePool *pool = emitter->GetPool();
		if( !pool )
		{
			continue;
		}

		const unsigned toSpawn = emitter->Update( externalDeltaTime, translation, pool->GetUsageDensityModifier() );

		if( toSpawn > 0 )
		{
			const int behaviour = emitter->GetBehaviourIndex();
			if( pool && behaviour > 0 )
			{
				pool->SpawnParticles( 
					toSpawn,
					behaviour,
					emitter->GetInheritedVelocity(),
					m_positionScale * emitter->GetPositionScale(),
					m_velocityScale * emitter->GetVelocityScale(),
					pos - translation,
					pos,
					vel,
					vel,
					localT,
					m_positionTexture,
					m_velocityTexture );
			}
		}
	}
}


void Tr2GPUParticleEmitter::SpawnParticles( const Vector3 *positionStart, const Vector3 *positionEnd,
											     const Vector3 *velocityStart, const Vector3 *velocityEnd,
												 float deltaTime )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	const Vector3 translation( *positionEnd - *positionStart );

	const Matrix localT( 1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1 );
	
	for( auto it = m_particleTypes.begin(); it != m_particleTypes.end(); ++it )
	{
		(*it)->CheckForUpdate();
	}
	for( auto it = m_subEmitters.begin(); it != m_subEmitters.end(); ++it )
	{
		Tr2GPUParticleSubEmitter *emitter = *it;
		
		if( !emitter->GetPool() || emitter->GetBehaviourIndex() <= 0 )
		{
			SetUpEmitter( emitter );
		}
		Tr2GPUParticlePool *pool = emitter->GetPool();
		if( !pool )
		{
			continue;
		}

		const unsigned toSpawn = emitter->Update( deltaTime, translation, pool->GetUsageDensityModifier() );

		if( toSpawn > 0 )
		{
			const int behaviour = emitter->GetBehaviourIndex();
			
			if( pool && behaviour > 0 )
			{
				pool->SpawnParticles( 
					toSpawn,
					behaviour,
					emitter->GetInheritedVelocity(),
					m_positionScale * emitter->GetPositionScale(),
					m_velocityScale * emitter->GetVelocityScale(),
					*positionStart,
					*positionEnd,
					*velocityStart,
					*velocityEnd,
					localT,
					m_positionTexture,
					m_velocityTexture );
			}
		}
	}

}

// ----------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter. Notify the emitter that it's spawn functions are going 
//   to be called in multi-threaded scenario (during particle system update). Since this
//   emitter does not emit into Tr2ParticleSystem we don't need to do anything here.
// ----------------------------------------------------------------------------------------
void Tr2GPUParticleEmitter::SetThreadSafeFlag()
{
}

void Tr2GPUParticleEmitter::Update( Be::Time time )
{
	
	CCP_STATS_ZONE( __FUNCTION__ );
	if( m_lastUpdateTimeValid )
	{
		if( m_lastUpdateTime != time  )
		{
			m_deltaTime = (float)TimeAsDouble( time - m_lastUpdateTime );
		} 
	}
	else
	{
		m_deltaTime = 0.f;
	}

	m_lastUpdateTimeValid = true;
	m_lastUpdateTime = time;
}

void Tr2GPUParticleEmitter::UpdateVisibilityBasedDensity( float density ) 
{
	density = std::min( 1.f, std::max( 0.f, density ) );
	if( density > m_visibilityBasedDensity )
	{
		m_visibilityBasedDensity = density;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Called to enable or disable particle emitting from this emitter
// Arguments:
//   enable - true enables emitting, false disables it.
// --------------------------------------------------------------------------------
void Tr2GPUParticleEmitter::Enable( bool enable )
{
	m_enabled = enable;
	m_lastPosValid = false;
}
