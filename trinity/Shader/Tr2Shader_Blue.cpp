#include "StdAfx.h"
#include "Tr2Shader.h"


BLUE_DEFINE( Tr2Shader );


const Be::ClassInfo* Tr2Shader::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2Shader, "" )
        MAP_INTERFACE( Tr2Shader )
	EXPOSURE_END()
}