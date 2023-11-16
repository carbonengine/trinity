////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2ElementBlendConstraint.h"

extern Be::VarChooser Tr2ParticleElementDeclarationNameTypeChooser[];

BLUE_DEFINE( Tr2ElementBlendConstraint );

const Be::ClassInfo* Tr2ElementBlendConstraint::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ElementBlendConstraint, "" )
        MAP_INTERFACE( Tr2ElementBlendConstraint )
		MAP_INTERFACE( ITr2GenericParticleConstraint )

		MAP_ATTRIBUTE( 
			"isValid", 
			m_isValid, 
			"Is the constraint bound to the particle system", 
			Be::READ )

		MAP_ATTRIBUTE_WITH_CHOOSER( 
			"elementType", 
			m_name.m_type, 
			"Pre-defined type of particle element", 
			Be::READWRITE | Be::PERSIST | Be::ENUM, 
			Tr2ParticleElementDeclarationNameTypeChooser )
		MAP_ATTRIBUTE( 
			"customName", 
			m_name.m_name, 
			"Name for custom type element", 
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( 
			"value", 
			m_value, 
			"Constant to add to element value", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"originalFactor", 
			m_originalFactor, 
			"Factor to scale element value", 
			Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}