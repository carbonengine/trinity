// Copyright © 2011 CCP ehf.

#pragma once
#ifndef Tr2DynamicEmitter_H
#define Tr2DynamicEmitter_H

#include "ITr2GenericEmitter.h"

BLUE_DECLARE( Tr2ParticleSystem );
BLUE_DECLARE_INTERFACE( ITr2AttributeGenerator );
BLUE_DECLARE_IVECTOR( ITr2AttributeGenerator );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2DynamicEmitter is a particle emitter for Tr2ParticleSystem. It dynamically emits
//   particles at a specified constant rate.
// See Also:
//   ITr2GenericEmitter, Tr2ParticleSystem
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2DynamicEmitter ) :
	public ITr2GenericEmitter,
	public INotify,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	Tr2DynamicEmitter( IRoot* lockobj = 0 );
	~Tr2DynamicEmitter();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * val );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2GenericEmitter
	void Update( const UpdateArguments& arguments );
	void SpawnParticles( const UpdateArguments& arguments, const Vector3* position = nullptr, const Vector3* velocity = nullptr, float rateModifier = 1.0f );
	void SpawnParticles(
		const UpdateArguments& arguments,
		const Vector3* positionStart,
		const Vector3* positionEnd,
		const Vector3* velocityStart,
		const Vector3* velocityEnd,
		float deltaTime );
	void SetThreadSafeFlag();

	bool IsValid() const;
	void Rebind();

	void ResetEmittedParticleCount();
	uint32_t GetEmittedParticleCount() const;

private:
	void UpdateSimulation( float dt );
	// Just a name
	std::string m_name;
	// List of particle element generators
	PITr2AttributeGeneratorVector m_generators;
	// Was the emitter successfully bound to the particle system
	bool m_isValid;
	// Does the emitter need synchronization when adding particles
	bool m_isThreadSafe;
	// Particle system the emitter emits to
	Tr2ParticleSystemPtr m_particleSystem;
	// Element declaration hash from last call to Rebind
	unsigned m_declarationHash;
	// Number of particles to emit per second
	float m_rate;
	// Time since last particle emitted
	float m_accumulatedRate;
	// Time of last update
	Be::Time m_lastUpdate;
	// Position of last emit
	Vector4 m_lastEmitterPos;

	int32_t m_maxParticles;
	CcpAtomic<uint32_t> m_emittedParticles;
};

TYPEDEF_BLUECLASS( Tr2DynamicEmitter );

#endif // Tr2DynamicEmitter_H