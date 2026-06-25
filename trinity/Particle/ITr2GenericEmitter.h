// Copyright © 2011 CCP ehf.

#pragma once
#ifndef ITr2GenericEmitter_H
#define ITr2GenericEmitter_H


BLUE_DECLARE( Tr2GpuParticleSystem );

// --------------------------------------------------------------------------------------
// Description:
//   ITr2GenericEmitter is an interface for particle emitters used with
//   Tr2ParticleSystem.
// See Also:
//   Tr2DynamicEmitter, Tr2ParticleSystem
// --------------------------------------------------------------------------------------
BLUE_INTERFACE( ITr2GenericEmitter ) :
	public IRoot
{
	// Arguments passed to Update method
	struct UpdateArguments
	{
		UpdateArguments() :
			time( 0 ),
			system( nullptr ),
			parentTransform( XMMatrixIdentity() ),
			originShift( 0.f, 0.f, 0.f ),
			emitCountFactor( 1.f )
		{
		}

		UpdateArguments( Be::Time t, Tr2GpuParticleSystem* gpuSystem, const Matrix& transform, const Vector3& shift, float emitFactor = 1.f ) :
			time( t ),
			system( gpuSystem ),
			parentTransform( transform ),
			originShift( shift ),
			emitCountFactor( emitFactor )
		{
		}

		// current system time
		Be::Time time;
		// Scene's GPU particle system
		Tr2GpuParticleSystem* system;
		// Parent object world transform
		Matrix parentTransform;
		// World origin shift since previous frame
		Vector3 originShift;
		// Factor for the number of particles to emit (for LODing)
		float emitCountFactor;
	};

	// --------------------------------------------------------------------------------------
	// Description:
	//   Per-frame update method for emitter.
	// Arguments:
	//   time - Current system time.
	// --------------------------------------------------------------------------------------
	virtual void Update( const UpdateArguments& arguments ) = 0;

	// --------------------------------------------------------------------------------------
	// Description:
	//   Spawns new particles. Can be called from Tr2ParticleSystem for "emit during
	//   lifetime" or "emit on death" emitters.
	// Arguments:
	//   position - Position of the "parent" particle (if the emitter owning this generator
	//		is "emit during life" or "emit on death" emitter and parent particle has
	//		position element); otherwise is nullptr.
	//   velocity - Velocity of the "parent" particle (if the emitter owning this generator
	//		is "emit during life" or "emit on death" emitter and parent particle has
	//		velocity element); otherwise is nullptr.
	//   rateModifier - Modifies the number of particles spawned as opposed to emitter's
	//		defined value.
	// --------------------------------------------------------------------------------------
	virtual void SpawnParticles( const UpdateArguments& arguments,
								 const Vector3* position = nullptr,
								 const Vector3* velocity = nullptr,
								 float rateModifier = 1.0f ) = 0;


	// --------------------------------------------------------------------------------------
	// Description:
	//   Spawns new particles. Can be called from Tr2ParticleSystem for "emit during
	//   lifetime" or "emit on death" emitters. Particle position and velocity values
	//   are provided for both beginning and end of frame, for better distribution.
	// Arguments:
	//   positionStart - Position of the "parent" particle at t=0
	//   positionEnd - Position of the parent pareticle at t=dt
	//   velocityStart - Velocity of the "parent" particle at t=0
	//   velocityEnd - velocity of the parent particle at t=dt
	//   deltaTime - particle simulation delta-time
	// --------------------------------------------------------------------------------------
	virtual void SpawnParticles( const UpdateArguments& arguments,
								 const Vector3* positionStart,
								 const Vector3* positionEnd,
								 const Vector3* velocityStart,
								 const Vector3* velocityEnd,
								 float deltaTime ) = 0;

	// --------------------------------------------------------------------------------------
	// Description:
	//   Notify the emitter that it's spawn functions are going to be called in multi-
	//   threaded scenario (during particle system update).
	// --------------------------------------------------------------------------------------
	virtual void SetThreadSafeFlag() = 0;
};

#endif // ITr2GenericEmitter_H