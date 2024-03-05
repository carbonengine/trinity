#include "StdAfx.h"
#include "TriVariableParameter.h"

BLUE_DEFINE( TriVariableParameter );

const Be::ClassInfo* TriVariableParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriVariableParameter, "TriVariableParameter" )
		MAP_INTERFACE( ITriEffectParameter )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )

		////////////////////////////////////////////////////////////////////////////
		//	name
		MAP_ATTRIBUTE
		( 
			"name",    
			m_name,    
			"Effect parameter name from .fx file", 
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)
		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE( 
			"variableName",	
			m_variableName,				
			"Name of variable managed by TriDevice", 
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)
		////////////////////////////////////////////////////////////////////////////
		MAP_PROPERTY_READONLY( 
			"variableType",	
			GetVariableType,			
			"Type of variable (0 is invalid)"
		)
		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE("usedByCurrentTechnique", m_isUsedByEffect, "na", Be::READ)
		MAP_ATTRIBUTE("usedByCurrentEffect", m_isUsedByEffect, "na", Be::READ)

	EXPOSURE_END()
}