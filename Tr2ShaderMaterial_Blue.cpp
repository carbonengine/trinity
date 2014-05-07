////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//
#include "StdAfx.h"
#include "Tr2ShaderMaterial.h"

BLUE_DEFINE( Tr2ShaderMaterial );

const Be::ClassInfo* Tr2ShaderMaterial::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ShaderMaterial, "" )
		
	MAP_INTERFACE( Tr2ShaderMaterial )
	MAP_INTERFACE( ITr2ShaderMaterial )
	MAP_INTERFACE( INotify ) // for when mapped attributes change
	MAP_INTERFACE( IInitialize ) // AFTER .red file is read and base object is created (create 'runtime data' from persisted data)
	
		MAP_ATTRIBUTE
		( 
			"name",      
			m_materialName,
			"name of this material", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"highLevelShaderName",
			m_highLevelShaderName,
			"name of the highlevel shader.",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"highLevelShader",
			m_highLevelShader,
			"The shader that drives this material and situations",
			Be::READ   
		)

		MAP_ATTRIBUTE
		(
			"lowLevelShader",
			m_lowLevelShader,
			"The currently bound shader",
			Be::READ   
		)

		MAP_ATTRIBUTE
		( 
			"parameters",      
			m_parameters,
			"parameters that drive this material instance", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"defaultSituation",
			m_defaultSituation,
			"Space-delimited string of situations that are assumed to be part of this material",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)

		MAP_METHOD_AND_WRAP
		(
			"PopulateParameters",
			PopulateParameters,
			"Populates the parameter list with the appropriate parameters"
		)

		MAP_METHOD_AND_WRAP
		(
			"PruneParameters",
			PruneParameters,
			"Prune parameters from the list that aren't in use by the low-level shader"
		)

		MAP_METHOD_AND_WRAP
		(
			"PopulateDefaultParameters",
			PopulateDefaultParameters,
			"Populates default parameter values from the high-level shader parameter descriptions"
		)

		MAP_METHOD_AND_WRAP
		(
			"ResetDefaultParameters",
			ResetDefaultParameters,
			"Clears the parameter list and resets parameters to their defaults, based on the high-level shader parameter descriptions"
		)

		MAP_METHOD_AND_WRAP
		(
			"RebuildCachedDataInternal",
			RebuildCachedDataInternal,
			"Call this after adding/removing parameters/resources"
		)

		MAP_METHOD_AND_WRAP( "BindLowLevelShader", PyBindLowLevelShader, "Directly binds shader with the give situation" )

	
	EXPOSURE_END()
}


