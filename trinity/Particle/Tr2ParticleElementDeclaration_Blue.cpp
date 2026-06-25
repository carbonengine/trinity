// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2ParticleElementDeclaration.h"

Be::VarChooser Tr2ParticleElementDeclarationNameTypeChooser[] = {
	{ "LIFETIME",
	  BeCast( Tr2ParticleElementDeclarationName::LIFETIME ),
	  "Particle life time (2D float)" },
	{ "POSITION",
	  BeCast( Tr2ParticleElementDeclarationName::POSITION ),
	  "Particle position (3D vector)" },
	{ "VELOCITY",
	  BeCast( Tr2ParticleElementDeclarationName::VELOCITY ),
	  "Particle velocity (3D vector)" },
	{ "MASS",
	  BeCast( Tr2ParticleElementDeclarationName::MASS ),
	  "Particle mass (1D float)" },
	{ "CUSTOM",
	  BeCast( Tr2ParticleElementDeclarationName::CUSTOM ),
	  "Custom data" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX(
	"PARTICLE_ELEMENT_TYPE",
	Tr2ParticleElementDeclarationName::Type,
	Tr2ParticleElementDeclarationNameTypeChooser,
	ENUM_REG_ENUM_OBJECT_ON_MODULE );


BLUE_DEFINE( Tr2ParticleElementDeclaration );

const Be::ClassInfo* Tr2ParticleElementDeclaration::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ParticleElementDeclaration, "" )
		MAP_INTERFACE( Tr2ParticleElementDeclaration )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"elementType",
			m_name.m_type,
			"Pre-defined type of particle element",
			Be::READWRITE | Be::PERSIST | Be::ENUM,
			Tr2ParticleElementDeclarationNameTypeChooser )
		MAP_PROPERTY_READONLY( "name", GetName, "Element name (composed of type and customName)" )
		MAP_ATTRIBUTE( "customName", m_name.m_name, "Name for custom type element", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "dimension", m_dimension, "Data dimentsion for custom type element", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "usageIndex", m_usageIndex, "Shader usage index for custom type element", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "usedByGPU", m_usedByGPU, "Is this element used by shaders", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
