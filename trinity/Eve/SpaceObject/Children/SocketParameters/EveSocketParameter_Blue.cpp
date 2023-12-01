////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//
#include "StdAfx.h"
#include "EveSocketParameter.h"


BLUE_DEFINE_INTERFACE( IEveSocketParameter );
BLUE_DEFINE_ABSTRACT( EveSocketParameterBindingBase );


const Be::ClassInfo* EveSocketParameterBindingBase::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSocketParameterBindingBase, "" )
	EXPOSURE_END()
}

#define SOCKET_PARAM_EXPOSE_TO_BLUE( _className, _valueDescription )\
	BLUE_DEFINE( _className );\
	const Be::ClassInfo* _className::ExposeToBlue()\
	{\
		EXPOSURE_BEGIN( _className, "\n:jessica-icon: fal-paragraph\n:jessica-icon-color: (123, 28, 212)\n:jessica-help-url: https://wiki.ccpgames.com/pages/viewpage.action?spaceKey=TTL&title=Plugs+and+Sockets \n" )\
			MAP_INTERFACE( IEveSocketParameter )\
			MAP_ATTRIBUTE( "name", m_name, "Attribute name.", Be::READWRITE | Be::PERSIST )\
			MAP_ATTRIBUTE( "value", m_value, _valueDescription, Be::READWRITE | Be::PERSIST )\
			MAP_METHOD_AND_WRAP( "Used", Used, "")\
			MAP_METHOD_AND_WRAP(\
				"SetValueToDefault",\
				SetValueToDefault,\
				"Reset the value to the default value (whatever the first external parameter bound to is in the original plug."\
				":jessica-placement:\n"\
			)\
			EXPOSURE_END()\
	}

SOCKET_PARAM_EXPOSE_TO_BLUE( EveSocketParameterBool, "Attribute value.\n:jessica-widget: checkbox\n" );
SOCKET_PARAM_EXPOSE_TO_BLUE( EveSocketParameterInt, "Attribute value.\n:jessica-widget: int\n" );
SOCKET_PARAM_EXPOSE_TO_BLUE( EveSocketParameterFloat, "Attribute value.\n:jessica-widget: float\n" );
SOCKET_PARAM_EXPOSE_TO_BLUE( EveSocketParameterVector2, "Attribute value.\n:jessica-widget: float2\n" );
SOCKET_PARAM_EXPOSE_TO_BLUE( EveSocketParameterVector3, "Attribute value.\n:jessica-widget: float3\n" );
SOCKET_PARAM_EXPOSE_TO_BLUE( EveSocketParameterVector4, "Attribute value.\n:jessica-widget: float4\n" );
SOCKET_PARAM_EXPOSE_TO_BLUE( EveSocketParameterColor, "Attribute value.\n:jessica-widget: color\n" );
SOCKET_PARAM_EXPOSE_TO_BLUE( EveSocketParameterString, "Attribute value.\n:jessica-widget: string\n" );
SOCKET_PARAM_EXPOSE_TO_BLUE( EveSocketParameterFilePath, "Attribute value.\n:jessica-widget: filepath\n" );
