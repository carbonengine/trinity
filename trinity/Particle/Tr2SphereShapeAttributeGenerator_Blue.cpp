////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#include "StdAfx.h"
#include "Tr2SphereShapeAttributeGenerator.h"


BLUE_DEFINE( Tr2SphereShapeAttributeGenerator );

const Be::ClassInfo* Tr2SphereShapeAttributeGenerator::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SphereShapeAttributeGenerator, "" )
		MAP_INTERFACE( Tr2SphereShapeAttributeGenerator )
		MAP_INTERFACE( ITr2AttributeGenerator )
		
		MAP_PROPERTY_READONLY( "name", GetName, "Element name (composed of type and customName)" )
		MAP_ATTRIBUTE( "position", m_position, "Generator position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "Generator orientation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minRadius", m_minRadius, "Sphere min radius", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxRadius", m_maxRadius, "Sphere max radius", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minPhi", m_minPhi, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxPhi", m_maxPhi, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minTheta", m_minTheta, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxTheta", m_maxTheta, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "distributionExponent", m_distributionExponent, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"controlPosition", 
			m_controlPosition, 
			"Does the generator affect particle position", 
			Be::READWRITE | Be::PERSIST )
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
