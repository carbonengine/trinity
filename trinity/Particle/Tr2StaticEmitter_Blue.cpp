// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2StaticEmitter.h"

BLUE_DEFINE( Tr2StaticEmitter );

const Be::ClassInfo* Tr2StaticEmitter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2StaticEmitter, "" )
		MAP_INTERFACE( Tr2StaticEmitter )
		MAP_INTERFACE( ITr2GenericEmitter )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"geometryResourcePath",
			m_geometryResourcePath,
			"Path to geometry resource with particle data",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE(
			"geometryResource",
			m_geometryResource,
			"Geometry resource with particle data",
			Be::READ )
		MAP_ATTRIBUTE(
			"particleSystem",
			m_particleSystem,
			"Particle system to emit particles to",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"meshIndex",
			m_meshIndex,
			"Mesh index in a geometry resource with particle data",
			Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "Spawn", Spawn, "(Re)spawns particles during next update" )
		MAP_METHOD_AND_WRAP( "ForceSpawn", DoSpawn, "Spawns particles immediately (emitter doesn\'t have to be in hierarhy." )

	EXPOSURE_END()
}
