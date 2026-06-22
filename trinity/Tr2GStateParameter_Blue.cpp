// Copyright © 2020 CCP ehf.

#include "StdAfx.h"

#if GSTATE_ENABLED

#include "Tr2GStateParameter.h"

BLUE_DEFINE( Tr2GStateParameter );

const Be::ClassInfo* Tr2GStateParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GStateParameter, "" )
		MAP_INTERFACE( Tr2GStateParameter )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "value", m_value, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "nodename", m_nodeName, "", Be::READWRITE | Be::PERSIST )
		MAP_METHOD_AND_WRAP(
			"SetName", SetName, "SetName( name )\n\n"
								"Sets the name of the parameter.\n"
								":param name: string with the newname" )
		MAP_METHOD_AND_WRAP(
			"SetValue", SetValue, "SetValue( value )\n\n"
								  "Sets the value of the parameter.\n"
								  ":param value: Double new value" )
		MAP_METHOD_AND_WRAP(
			"GetName", GetName, "GetName()\n\n"
								"Returns a Dobule with the value of the paramter.\n" )
		MAP_METHOD_AND_WRAP(
			"GetValue", GetValue, "GetValue()\n\n"
								  "Returns a string with the name of the paramter.\n" )
		MAP_METHOD_AND_WRAP(
			"SetNodeName", SetNodeName, "SetNodeName( name )\n\n"
										"Sets the name of the parent node.\n"
										":param name: string with the newname" )
		MAP_METHOD_AND_WRAP(
			"GetNodeName", GetNodeName, "GetNodeName()\n\n"
										"Returns a string with the name of the parent node.\n" )

	EXPOSURE_END()
}
#endif