// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2CapsuleShapeAttributeGenerator.h"


BLUE_DEFINE( Tr2CapsuleShapeAttributeGenerator );

const Be::ClassInfo* Tr2CapsuleShapeAttributeGenerator::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CapsuleShapeAttributeGenerator, "" )
		MAP_INTERFACE( Tr2CapsuleShapeAttributeGenerator )
		MAP_INTERFACE( ITr2AttributeGenerator )

		MAP_PROPERTY_READONLY( "name", GetName, "Element name (composed of type and customName)" )
		MAP_ATTRIBUTE( "positionStart", m_positionStart, "Generator starting position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "positionEnd", m_positionEnd, "Generator ending position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotationStart", m_rotationStart, "Generator starting orientation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotationEnd", m_rotationEnd, "Generator starting orientation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minRadius", m_minRadius, "Sphere min radius", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxRadius", m_maxRadius, "Sphere max radius", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minPhi", m_minPhi, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxPhi", m_maxPhi, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minTheta", m_minTheta, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxTheta", m_maxTheta, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"controlVelocity",
			m_controlVelocity,
			"Does the generator affect particle velocity",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minSpeed", m_minSpeed, "Minimal particle speed", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxSpeed", m_maxSpeed, "Maximal particle speed", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"parentVelocityFactor",
			m_parentVelocityFactor,
			"How much parent particle velocity affects new particle velocity (for emit during lifetime and on deth emitters)",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "valid", m_valid, "Was the generator successfully bound to particle system", Be::READ )

	EXPOSURE_END()
}
