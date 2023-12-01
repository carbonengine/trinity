////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef Tr2StaticEmitter_H
#define Tr2StaticEmitter_H

#include "ITr2GenericEmitter.h"
#include "Tr2ParticleElementDeclaration.h"

BLUE_DECLARE( Tr2ParticleSystem );
BLUE_DECLARE( TriGrannyRes );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2StaticEmitter is a particle emitter for Tr2ParticleSystem. It emits particles
//   only once using .gr2 file for particle data.
// See Also:
//   ITr2GenericEmitter, Tr2ParticleSystem
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2StaticEmitter ): 
	public ITr2GenericEmitter,
	public INotify, 
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	Tr2StaticEmitter( IRoot* lockobj = 0 );
	~Tr2StaticEmitter();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2GenericEmitter
	void Update( const ITr2GenericEmitter::UpdateArguments& arguments );
	void SpawnParticles( const ITr2GenericEmitter::UpdateArguments& arguments, 
						 const Vector3* position = nullptr, const Vector3* velocity = nullptr, float rateModifier = 1.0f );
	void SpawnParticles( const ITr2GenericEmitter::UpdateArguments& arguments, 
						 const Vector3 *positionStart, const Vector3 *positionEnd,
						 const Vector3 *velocityStart, const Vector3 *velocityEnd,
						 float deltaTime );
	void SetThreadSafeFlag();

	void Spawn();
private:
	// --------------------------------------------------------------------------------------
	// Description:
	//   Mapping between particle element declaration and D3D vertex declaration in particle
	//   data vertex buffer.
	// --------------------------------------------------------------------------------------
	struct DeclarationMapping
	{
		// Element offset in vertex buffer
		unsigned inOffset;
		// Buffer type for particle element
		Tr2ParticleElementData::BufferType buffer;
		// Element offset in the particle buffer
		unsigned offset;
		// Length/dimension of particle element
		unsigned length;
		// Is the element in vertex buffer type FLOAT16 (needs convertion to 32 bit float)
		bool isFloat16;
		bool isEmpty;
	};

	void DoSpawn();

	// Just a name
	std::string m_name;
	// Path to geometry resource
	std::string m_geometryResourcePath;
	// Geometry resource containing particle data
	TriGrannyResPtr m_geometryResource;
	// System to emit particles to
	Tr2ParticleSystemPtr m_particleSystem;
	// Did the emitter already emit particles
	bool m_hasSpawnedParticles;
	// Does the emitter need synchronization when adding particles
	bool m_isThreadSafe;
	// Index of the mesh containing particle data
	uint32_t m_meshIndex;
};

TYPEDEF_BLUECLASS( Tr2StaticEmitter );

#endif // Tr2StaticEmitter_H