// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionSetShaderOption.h"
#include "Shader/IShaderConfigurer.h"

BLUE_DEFINE_INTERFACE( IShaderConfigurer )

BLUE_DEFINE( Tr2ActionSetShaderOption );

const Be::ClassInfo* Tr2ActionSetShaderOption::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionSetShaderOption, "" )
		MAP_INTERFACE( Tr2ActionSetShaderOption )
		MAP_INTERFACE( ITr2ControllerAction )
		MAP_ATTRIBUTE( "key", m_optionKey, "The shader key to set for the option", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "value", m_optionValue, "They value to set for the shader option", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
