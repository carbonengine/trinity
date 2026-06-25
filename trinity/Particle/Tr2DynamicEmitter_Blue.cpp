// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2DynamicEmitter.h"

BLUE_DEFINE( Tr2DynamicEmitter );

const Be::ClassInfo* Tr2DynamicEmitter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2DynamicEmitter, "" )
		MAP_INTERFACE( Tr2DynamicEmitter )
		MAP_INTERFACE( ITr2GenericEmitter )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "generators", m_generators, "List of generators for each particle element", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "isValid", m_isValid, "If the emitter successfully bound to the particle system", Be::READ )
		MAP_ATTRIBUTE( "particleSystem", m_particleSystem, "Particle system to emit particles to", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rate", m_rate, "Number of particles to emit per second", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxParticles", m_maxParticles, "Maximum number of particles to emit (set to <0 for no limit)", Be::READWRITE | Be::PERSIST )
		MAP_PROPERTY_READONLY( "emittedParticles", GetEmittedParticleCount, "Number of particles emitted" )

		MAP_METHOD_AND_WRAP( "Rebind", Rebind, "Re-binds emitter to the particle system" )
		MAP_METHOD_AND_WRAP(
			"UpdateSimulation",
			UpdateSimulation,
			"Updates emitter manually.\n"
			":param dt: time inteval (in seconds)" )

	EXPOSURE_END()
}
