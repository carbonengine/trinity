////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#include "StdAfx.h"
#include "Tr2ParticleSystem.h"

BLUE_DEFINE_INTERFACE( ITr2Emitter );
BLUE_DEFINE_INTERFACE( ITr2ParticleForce );

BLUE_DEFINE_INTERFACE( ITr2AttributeGenerator );
BLUE_DEFINE_INTERFACE( ITr2GenericEmitter );
BLUE_DEFINE_INTERFACE( ITr2GenericParticleConstraint );

BLUE_DEFINE( Tr2ParticleSystem );

const Be::ClassInfo* Tr2ParticleSystem::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ParticleSystem, "" )
		MAP_INTERFACE( Tr2ParticleSystem )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2InstanceData )
		MAP_INTERFACE( ITr2GpuBuffer )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		
		MAP_ATTRIBUTE( "aliveCount", m_aliveCount, "The number of particles currently alive", Be::READ )
		MAP_ATTRIBUTE
		(
			"emitParticleOnDeathEmitter",  
			m_emissionOnDeathEmitter,
			"An emitter that spawns (count) particles when a particle in this system dies.", 
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"emitParticleDuringLifeEmitter",  
			m_emissionWhileAliveEmitter,
			"An emitter that spawns particles with a chance based on the rate from the "
			"positions of particles in this system during their lifetime.", 
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"elements",  
			m_elements,
			"", 
			Be::READ | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"isValid",  
			m_isValid,
			"", 
			Be::READ
		)

		MAP_ATTRIBUTE( 
			"requiresSorting", 
			m_requiresSorting, 
			"If the system requires particles to be sorted relative to the viewer. Not required for additive particles.", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( 
			"updateSimulation", 
			m_updateSimulation, 
			"If simulation of the particles is being run. Typically turned off for static particle clouds.", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"applyForce", 
			m_applyForce, 
			"If set, forces will be applied to particles.", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"applyAging", 
			m_applyAging, 
			"If set, lifespan will be taken down, so particles can die .", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"isGlobal", 
			m_isGlobal, 
			"If set, this particle system is global i.e. all particles live in world space", 
			Be::READWRITE | Be::PERSIST )
		
		MAP_ATTRIBUTE( 
			"forces", 
			m_forces, 
			"A list of forces to be applied to the particles", 
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"constraints", 
			m_constraints, 
			"A list of constraints to be applied to the particles", 
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "aabbMin", m_AabbMin, "Minimum of the AABB", Be::READ );
		MAP_ATTRIBUTE( "aabbMax", m_AabbMax, "Maximum of the AABB", Be::READ );

		MAP_ATTRIBUTE( "peakAliveCount", m_peakAliveCount, "Peak live particle count", Be::READ )

		MAP_ATTRIBUTE( 
			"useSimTimeRebase", 
			m_useSimTimeRebase, 
			"Should the particle system rebase time when sim time is rebased", 
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )
			
		MAP_ATTRIBUTE(
			"maxParticleCount",
			m_maxParticleCount,
			"The maximum number of particles to spawn",
			Be::PERSISTONLY )

		MAP_ATTRIBUTE(
			"originalMaxParticles",
			m_originalMaxParticles,
			"The original max particle value. Used for Lodding",
			Be::READ )

		MAP_PROPERTY( 
			"maxParticleCount", 
			GetMaxParticleCount,
			SetMaxParticleCount,
			"The maximum number of particles to spawn. Setting this clears the particle system." )

		MAP_PROPERTY_READONLY( 
			"gpuStride", 
			GetGpuStride, 
			"Size of one element in GPU buffer in bytes" )
		
		MAP_METHOD_AND_WRAP( "UpdateElementDeclaration", UpdateElementDeclaration, "Updates internal particle element declaration" )
		MAP_METHOD_AND_WRAP( "ClearParticles", ClearParticles, "Clear all of the active particles from the system." )
		MAP_METHOD_AND_WRAP( 
			"SaveToGranny", 
			SaveToGranny, 
			"Saves particle data into a granny file\n"
			":param path: res path to granny file where to save particle data" )
		MAP_METHOD_AND_WRAP( "RebindConstraints", RebindConstraints, "Rebinds all system constraints." )
		MAP_METHOD_AND_WRAP( 
			"UpdateSimulation", 
			UpdateSimulationScript, 
			"Updates particle system manually.\n" 
			":param dt: time inteval (in seconds)" )

	EXPOSURE_END()
}