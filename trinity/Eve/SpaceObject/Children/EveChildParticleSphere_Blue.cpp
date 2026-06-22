// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "EveChildParticleSphere.h"

BLUE_DEFINE( EveChildParticleSphere );

const Be::ClassInfo* EveChildParticleSphere::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildParticleSphere, "" )
		MAP_INTERFACE( EveChildParticleSphere )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2Renderable )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Toggle visibility", Be::READWRITE | Be::PERSIST )
		MAP_PROPERTY_READONLY( "isValid", CheckBinding, "Do particle system and generators match" )

		MAP_ATTRIBUTE( "generators", m_generators, "Particle system element generators", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "particleSystem", m_particleSystem, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "mesh", m_mesh, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "radius", m_radius, "Effect sphere radius", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "egoSpeed", m_egoSpeed, "Ego speed", Be::READ )
		MAP_ATTRIBUTE( "movementScale", m_movementScale, "Scale how much particles move (applied after max speed)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxSpeed", m_maxSpeed, "Clamp movement of particles to this value", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "positionShift", m_positionShiftNormalized, "Sphere position shift length since the previous frame, normalized", Be::READ )
		MAP_ATTRIBUTE( "positionShiftMax", m_positionShiftMax, "Value of sphere shift length for positionShift to be 1", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "positionShiftMin", m_positionShiftMin, "Value of sphere shift length for positionShift to be 0", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "positionShiftIncreaseSpeed", m_positionShiftIncreaseSpeed, "Normalized position shift increase speed", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "positionShiftDecreaseSpeed", m_positionShiftDecreaseSpeed, "Normalized position shift decrease speed", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "useSpaceObjectData", m_useSpaceObjectData, "", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "Refresh", Refresh, "Re-binds particle system and re-adds particles" )
	EXPOSURE_END()
}
