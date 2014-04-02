////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//
#include "StdAfx.h"
#include "Tr2HighLevelShader.h"


BLUE_DEFINE( Tr2HighLevelShader );

const Be::ClassInfo* Tr2HighLevelShader::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2HighLevelShader, "" )
		
	MAP_INTERFACE(Tr2HighLevelShader);
	MAP_INTERFACE(IInitialize);
	MAP_INTERFACE(INotify);

	MAP_ATTRIBUTE
	( 
		"name",      
		m_name,
		"The name of this shader", 
		Be::READ | Be::PERSIST
	)

	MAP_ATTRIBUTE
	( 
		"UIName",      
		m_UIName,
		"The name of this shader for the shader UI", 
		Be::READ | Be::PERSIST
	)

	MAP_ATTRIBUTE
	( 
		"shaderPath",      
		m_shaderFilePath,
		"path to .fx file that this high level shader uses to generate low level shaders", 
		Be::READ | Be::PERSIST | Be::NOTIFY
	)
	
	MAP_ATTRIBUTE
	( 
		"description",      
		m_description,
		"An overview of this shader", 
		Be::READ | Be::PERSIST
	)			

	MAP_ATTRIBUTE
	(
		"parameterDescriptions",      
		m_parameters,
		"parameter descriptions", 
		Be::READ | Be::PERSIST
	)

	MAP_ATTRIBUTE
	(
		"permuteTags",      
		m_permuteTags,
		"flags used in permutation", 
		Be::READ | Be::PERSIST
	)

	MAP_ATTRIBUTE
	(
		"renderClass",
		m_renderTypeString,
		"Get a string that reports the type of rendering, such as opaque, mirror, decal, and so on",
		Be::READ | Be::PERSIST
	)

	MAP_ATTRIBUTE
	(
		"alwaysCachePredicate",
		m_alwaysCachePredicate,
		"Python expression indicating whether this shader should always be added to the shader cache",
		Be::READ | Be::PERSIST
	)


	MAP_METHOD_AND_WRAP
	(
		"RebuildLowLevelShaders", 
		RebuildLowLevelShadersAfterCodeChange, 
		"Useful for reloading shaders after code has been changed"
	);

	MAP_METHOD_AND_WRAP
	(
		"PermuteIndexToString", 
		PermuteIndexToString, 
		"Returns define string for a given permute index"
	);

	MAP_METHOD_AND_WRAP
	(
		"GetLoadedShaders",
		GetLoadedShaders,
		"Returns a list of loaded low-level shaders"
	);

	MAP_METHOD_AND_WRAP
	(
		"CloseCacheFile", 
		CloseCacheFile, 
		"Closes compiled cache file handle, so that it can be overwritten outside Jessica."
		" Useful for debugging/recompiling shaders"
	);

	EXPOSURE_END()


}


