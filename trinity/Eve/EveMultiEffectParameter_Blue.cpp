// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "EveMultiEffectParameter.h"

BLUE_DEFINE( EveMultiEffectParameter );

Be::VarChooser EveMultiEffectParameterTypeChooser[] = {
	{ "EveSpaceObject2", BeCast( EveMultiEffectParameter::TYPE_EVESPACEOBJECT ), "The parameter is of EveSpaceObject2 type" },
	{ "EveEffectRoot2", BeCast( EveMultiEffectParameter::TYPE_EVEEFFECTROOT ), "The parameter is of EveEffectRoot2 type" },
	{ "Anything", BeCast( EveMultiEffectParameter::TYPE_ANYTHING ), "The parameter can be anything" },
	{ "Undefined", BeCast( EveMultiEffectParameter::TYPE_UNDEFINED ), "The parameter is of an undefined type" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( "EveMultiEffectParameterType", EveMultiEffectParameter::ParameterType, EveMultiEffectParameterTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* EveMultiEffectParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveMultiEffectParameter, "" )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "object", m_object, "The object to add bindings to", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE_WITH_CHOOSER( "type", m_type, "The type of object that this parameter is supposed to be bound to", Be::READWRITE | Be::ENUM, EveMultiEffectParameterTypeChooser )
		MAP_PROPERTY_READONLY( "isValid", IsValid, "Is the parameter valid" )

	EXPOSURE_END()
}
