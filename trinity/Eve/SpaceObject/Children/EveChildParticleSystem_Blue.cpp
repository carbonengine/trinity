// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveChildParticleSystem.h"

BLUE_DEFINE( EveChildParticleSystem );

const Be::ClassInfo* EveChildParticleSystem::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildParticleSystem, "" )
		MAP_INTERFACE( EveChildParticleSystem )
		MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "mesh", m_mesh, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "particleEmitters", m_particleEmitters, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "particleSystems", m_particleSystems, "", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "Should local transform be built from scaling, rotation and translation attributes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "Does local transform need to be rebuilt every frame.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "transformModifiers", m_transformModifiers, "", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE_WITH_CHOOSER( "reflectionMode", m_reflectionMode, "When is this object rendered into the cubemap", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, EntityComponents::ReflectionModeChooser );

		MAP_ATTRIBUTE(
			"useDynamicLod",
			m_useDynamicLod,
			"Dynamically change the max particle count \n"
			":jessica-group: LOD",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"lodClampLow",
			m_lodClampLow,
			"Amount of particles when the LOD is on Low. Set this to 0 for no particles. \n"
			":jessica-group: LOD",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"lodFactorLow",
			m_lodFactorLow,
			"Max particle multiplier when the LOD is low \n"
			":jessica-group: LOD",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"lodFactorMedium",
			m_lodFactorMedium,
			"Max particle multiplier when the LOD is medium \n"
			":jessica-group: LOD",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"lodSphereRadius",
			m_lodSphereRadius,
			"Bounding sphere radius for LOD calculations when mesh is not present.\n:jessica-group: LOD",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"minScreenSize",
			m_minScreenSize,
			"Minimal size of object on screen, objects smaller than this size are not rendered.\n:jessica-group: LOD",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "currentScreenSize", m_currentScreenSize, "Screen size for last frame\n:jessica-group: LOD", Be::READ )
		MAP_METHOD_AND_WRAP( "RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform if useSRT is set." )

	EXPOSURE_END()
}