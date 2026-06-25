// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2ConsecutiveIntegerAttributeGenerator.h"

extern Be::VarChooser Tr2ParticleElementDeclarationNameTypeChooser[];

BLUE_DEFINE( Tr2ConsecutiveIntegerAttributeGenerator );

const Be::ClassInfo* Tr2ConsecutiveIntegerAttributeGenerator::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ConsecutiveIntegerAttributeGenerator, "" )
		MAP_INTERFACE( Tr2ConsecutiveIntegerAttributeGenerator )
		MAP_INTERFACE( ITr2AttributeGenerator )

		MAP_PROPERTY_READONLY( "name", GetName, "Element name (composed of type and customName)" )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"elementType",
			m_name.m_type,
			"Pre-defined type of particle element",
			Be::READWRITE | Be::PERSIST | Be::ENUM,
			Tr2ParticleElementDeclarationNameTypeChooser )
		MAP_ATTRIBUTE( "customName", m_name.m_name, "Name for custom type element", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minRange", m_minRange, "Min bound for value range", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxRange", m_maxRange, "Max bound for value range", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "valid", m_valid, "Was the generator successfully bound to particle system", Be::READ )
		MAP_PROPERTY_READONLY( "dimension", GetDimension, "Dimension of bound particle element" )

	EXPOSURE_END()
}
