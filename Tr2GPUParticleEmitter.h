#pragma once

#ifndef Tr2GPUParticleEmitter_h
#define Tr2GPUParticleEmitter_h

#include "Particle/ITr2GenericEmitter.h"

BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2GPUParticlePool );
BLUE_DECLARE( Tr2GPUParticleType );
BLUE_DECLARE_VECTOR( Tr2GPUParticleType );
BLUE_DECLARE( Tr2GPUParticlePoolManager );

//Each particle emitter may contain many sub-emitters, with unique spawn rates
// and different particle types.
class Tr2GPUParticleSubEmitter : public IRoot  {
public:
    EXPOSE_TO_BLUE();
	Tr2GPUParticleSubEmitter(IRoot *lockobj = NULL);
	//update returns the total number of particles emitted by this sub-emitter during this time period
	unsigned Update( Be::Time time, const Vector3 &translation, const float rateModifier );
	unsigned Update( float deltaTime, const Vector3 &translation, const float rateModifier );

	//emission
	void SetEmissionRate( const float rate ) { m_emissionRate = rate; }
	void SetEmissionDensity( const float density ) { m_emissionDensity = density; }

	//named behaviour
	const char *GetBehaviour() const { return m_behaviourName.c_str(); }	
	void SetBehaviour( const char *name ) { m_behaviourName = name; }

	//more boring accessors
	float GetInheritedVelocity() const { return m_inheritVelocity; }
	float GetVelocityScale() const { return m_velocityScale; }
	float GetPositionScale() const { return m_positionScale; }

	//hmm
	void SetPool( Tr2GPUParticlePool *pool ) { m_pool = pool; }
	Tr2GPUParticlePool *GetPool() const { return m_pool; }
	void SetBehaviourIndex( int b ) { m_behaviourIndex = b; }
	int GetBehaviourIndex() const { return m_behaviourIndex; }

	Vector3 m_offset;
private:
	std::string m_name;
	//particle type to spawn
	std::string m_behaviourName;

	//emit bursts at time offset burstTime
	float m_burstTime, m_burstTimeLocal;
	//number of particles to emit during burst
	unsigned m_burstCount;
	//if > 0, repeat the burst with this period, forever.
	float m_burstCycle;

	//emit per unit time, distance:
	float m_emissionRate, m_emissionDensity;

	//horrible internal state
	bool m_previousTimeValid;
	Be::Time m_previousTime;
	float m_residualEmission;

	//interesting things for this sub-emitter (multipliers for emitter values in pos. & vel. scale cases)
	float m_inheritVelocity;
	float m_velocityScale;
	float m_positionScale;

	//hmm
	Tr2GPUParticlePool *m_pool;
	int m_behaviourIndex;

};

TYPEDEF_BLUECLASS( Tr2GPUParticleSubEmitter );
BLUE_DECLARE( Tr2GPUParticleSubEmitter );
BLUE_DECLARE_VECTOR( Tr2GPUParticleSubEmitter );

//The emitter spawns GPU particles. Mostly this is handled by Update( time, pos, vel, transform ),
// but it also supports the ITr2GenericEmitter interface for use as a particleLife or onParticleDeath emitter,
// using CPU particles as emission sources.
//Emitter shape is determined by textures, which can store any sensible function (the default is a spherical shell).
class Tr2GPUParticleEmitter: public INotify, public ITr2GenericEmitter
{ 
public:
    EXPOSE_TO_BLUE();
	void UpdateTransform( const Vector3 &parentPos, const Vector3 &parentVel, const Vector3 &egoLocation, const Matrix &parentT = s_identity );
	Tr2GPUParticleEmitter(IRoot *lockobj = NULL);

	bool OnModified(Be::Var* value);

	const Vector3 &GetLastTranslation() const { return m_translation; }
	const bool IsLastTranslationValid() const { return m_lastPosValid; }

	void UpdateVisibilityBasedDensity( float density );

	// enable/disable emitting
	void Enable( bool enable );
	
	//ITr2GenericEmitter
	void Update( Be::Time time );
	void SpawnParticles( const Vector3* position = nullptr, 
						 const Vector3* velocity = nullptr, 
						 float rateModifier = 1.0f );
	void SpawnParticles( const Vector3 *positionStart, const Vector3 *positionEnd,
						 const Vector3 *velocityStart, const Vector3 *velocityEnd,
						 float deltaTime );
	void SetThreadSafeFlag();

	void ApplyPool( Tr2GPUParticlePoolManager* manager );

private:
	// Just a name
	std::string m_name;

	// can be disabled
	bool m_enabled;

	bool m_lastPosValid;
	Vector3 m_lastUpdatePosition, m_lastUpdateVelocity;

	PTr2GPUParticleSubEmitterVector m_subEmitters;
	float m_positionScale, m_velocityScale;

	void LoadResources();
	std::wstring m_positionTexturePath, m_velocityTexturePath;
	std::wstring m_currentPositionPath, m_currentVelocityPath;
	TriTextureResPtr m_positionTexture, m_velocityTexture;

	Vector3 m_offset, m_translation;
	float m_maximumTranslationPerFrame;
	const static Matrix s_identity;

	PTr2GPUParticleTypeVector m_particleTypes;
	
	//emitting particles during the lifespan/death of another particle
	// requires keeping some state around
	Be::Time m_lastUpdateTime;
	bool m_lastUpdateTimeValid;
	float m_deltaTime;
	
	//for distanced-based continuous LOD/frustum partial culling
	float m_visibilityBasedDensity;

	void SetUpEmitter( Tr2GPUParticleSubEmitter *emitter );
};

TYPEDEF_BLUECLASS( Tr2GPUParticleEmitter );
BLUE_DECLARE( Tr2GPUParticleEmitter );
BLUE_DECLARE_VECTOR( Tr2GPUParticleEmitter );

#endif//Tr2GPUParticleEmitter_h