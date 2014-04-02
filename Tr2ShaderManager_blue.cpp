////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"
#include "Tr2ShaderManager.h"

BLUE_DEFINE( Tr2ShaderManager );

const Be::ClassInfo* Tr2ShaderManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ShaderManager, "" )
		
	MAP_INTERFACE(Tr2ShaderManager)

	MAP_ATTRIBUTE
	( 
		"shaderLibrary",      
		m_shaderLibrary,
		"the list of all high level shaders in use", 
		Be::READWRITE
	)

	EXPOSURE_END()
}

MAP_FUNCTION_AND_WRAP( "GetShaderManager", GetShaderManager, "Gets the shader manager singleton" );

