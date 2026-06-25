// Copyright © 2016 CCP ehf.

#include "StdAfx.h"
#include "Tr2ExternalParameter.h"

BLUE_DEFINE( Tr2ExternalParameter );

const Be::ClassInfo* Tr2ExternalParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ExternalParameter, "" )
		MAP_INTERFACE( Tr2ExternalParameter )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "destinationObject", m_destinationObject, "Destination blue object", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "destinationAttribute", m_destinationAttribute, "Destination object\'s attribute name", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "valid", m_valid, "Is the parameter valid", Be::READ )

		MAP_METHOD_AND_WRAP(
			"GetValue",
			GetValue,
			"Returns current value of the parameter.\n"
			":raises RuntimeError: if parameter is invalid" )
		MAP_METHOD_AND_WRAP(
			"SetValue",
			SetValue,
			"Sets new value for the parameter.\n"
			":param value: value to set\n"
			":raises RuntimeError: if parameter is invalid\n"
			":raises TypeError: if value is of incorrect type" )
	EXPOSURE_END()
}
