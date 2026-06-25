// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Matrix4Parameter.h"

BLUE_DEFINE( Tr2Matrix4Parameter );

const Be::ClassInfo* Tr2Matrix4Parameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Matrix4Parameter, "" )
		MAP_INTERFACE( ITriEffectParameter )
		MAP_INTERFACE( Tr2Matrix4Parameter )
		MAP_INTERFACE( ITriReroutable )
		////////////////////////////////////////////////////////////////////////////
		//               name
		MAP_ATTRIBUTE(
			"name",
			m_name,
			"na",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE( "value", m_value, "", Be::PERSISTONLY )
		MAP_PROPERTY( "value", GetValue, SetValue, "4x4 Matrix" )

		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE( "usedByCurrentTechnique", m_isUsedByEffect, "na", Be::READ )
		MAP_ATTRIBUTE( "usedByCurrentEffect", m_isUsedByEffect, "na", Be::READ )

	EXPOSURE_END()
}
