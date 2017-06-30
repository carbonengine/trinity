#include "StdAfx.h"
#include "TriStepRenderFullScreenShader.h"
#include "Shader/Tr2Material.h"

BLUE_DEFINE( TriStepRenderFullScreenShader );

const Be::ClassInfo* TriStepRenderFullScreenShader::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepRenderFullScreenShader, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRenderFullScreenShader )

		MAP_ATTRIBUTE( "shader", m_shader, "na", Be::READWRITE )
		
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		( 
			"__init__", 
			py__init__,
			1,
			"Create a render step that draws a fullscreen quad using a specified shader\n"
			":param shader: A material (default None)"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}