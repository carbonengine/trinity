////////////////////////////////////////////////////////////
//
//    Creator:   Reevan McKay
//    Created:   December 2010
//    Copyright: CCP 2010-2011
//

#include "StdAfx.h"
#include "Tr2MaterialParameterStore.h"

BLUE_DEFINE( Tr2MaterialParameterStore );

const Be::ClassInfo* Tr2MaterialParameterStore::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2MaterialParameterStore, "" )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( Tr2MaterialParameterStore )
		MAP_ATTRIBUTE( "name", m_name, "The name of the material", Be::READWRITE | Be::PERSIST ) 
		MAP_ATTRIBUTE( "parentPath", m_parentPath, "The path to our parent material", Be::READWRITE | Be::PERSIST | Be::NOTIFY ) 
		MAP_ATTRIBUTE( "parent", m_parentStore, "The materialStore we derive from", Be::READ ) 
		MAP_ATTRIBUTE( "parameters", m_parameters, "The shader parameters of this instance", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( 
			"FindParameter", 
			FindParameter, 
			"Finds the parameter in this or the parent's store.  Returns None if not found\n" 
			":param name: parameter name"
		)
	EXPOSURE_END()
}