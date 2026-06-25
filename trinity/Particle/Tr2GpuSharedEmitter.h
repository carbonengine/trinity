// Copyright © 2015 CCP ehf.

#pragma once
#ifndef Tr2GpuSharedEmitter_H
#define Tr2GpuSharedEmitter_H

#include "Tr2GpuParticleSystem.h"
#include "ITr2GenericEmitter.h"

// --------------------------------------------------------------------------------------
// Description:
//   Emitter class for GPU particle system. Shared emitters can share persistent particle
//   parameters between instances. It is preferable to use shared emitters (as opposed to
//   unique emitters) to conserve memory whenever possible. Since shared emitter
//   persistent parameters are shared they should not be animated. Also shared emitters
//   lack certain particle features (like attract forces).
// See Also:
//   Tr2GpuParticleSystem, Tr2GpuUniqueEmitter
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2GpuSharedEmitter ) :
	public IInitialize,
	public INotify,
	public ITr2GenericEmitter
{
public:
	Tr2GpuSharedEmitter( IRoot* lockObj = nullptr );

	EXPOSE_TO_BLUE();

	virtual bool Initialize();

	virtual bool OnModified( Be::Var * value );

	void Enable( bool enable );

	virtual void Update( const UpdateArguments& arguments );
	virtual void SpawnParticles( const UpdateArguments& arguments,
								 const Vector3* position = nullptr,
								 const Vector3* velocity = nullptr,
								 float rateModifier = 1.0f );
	virtual void SpawnParticles( const UpdateArguments& arguments,
								 const Vector3* positionStart,
								 const Vector3* positionEnd,
								 const Vector3* velocityStart,
								 const Vector3* velocityEnd,
								 float deltaTime );
	void SpawnOnce( const UpdateArguments& arguments, const Vector3& velocity, float scale = 1.f, float rateModifier = 1.f );
	virtual void SetThreadSafeFlag()
	{
	}

	// access
	void Setup( float rate, const Tr2GpuParticleSystem::Emitter* emitterData, const Tr2GpuParticleSystem::EmitterParams* paramsData );
	void SetDirection( const Vector3* direction );
	void SetPosition( const Vector3* position );

protected:
	virtual void GenerateID();
	uintptr_t GetID( uintptr_t hash ) const;

	void UpdateHash();
	uintptr_t GetHash( const Tr2GpuParticleSystem::EmitterParams& params ) const;

	float SpawnParticles(
		Tr2GpuParticleSystem::Emitter & emitter,
		const UpdateArguments& arguments,
		const Vector3& positionStart,
		const Vector3& positionEnd,
		const Vector3& velocityStart,
		const Vector3& velocityEnd,
		float carryOverCount,
		float deltaTime );

	std::string m_name;
	Be::Time m_previousTime;
	uintptr_t m_id;
	uintptr_t m_paramsHash;
	float m_rate;
	float m_carryOver;
	float m_inheritVelocity;
	float m_emissionDensity;
	float m_maxDensity;
	float m_maxDisplacement;
	Vector3 m_position;
	Vector3 m_direction;
	Vector3 m_prevPosition;
	Vector3 m_prevVelocity;
	Vector3 m_prevDirection;
	Tr2GpuParticleSystem::Emitter m_emitter;
	Tr2GpuParticleSystem::EmitterParams m_params;
	bool m_continuousEmitter;
	bool m_enabled;
};

TYPEDEF_BLUECLASS( Tr2GpuSharedEmitter );

#endif