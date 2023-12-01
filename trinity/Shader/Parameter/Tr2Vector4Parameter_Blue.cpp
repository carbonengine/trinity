#include "StdAfx.h"
#include "Tr2Vector4Parameter.h"

BLUE_DEFINE( Tr2Vector4Parameter );

const Be::ClassInfo* Tr2Vector4Parameter::ExposeToBlue()
{
	EXPOSURE_BEGIN(Tr2Vector4Parameter, "" )
		MAP_INTERFACE( ITriEffectParameter )
		MAP_INTERFACE( Tr2Vector4Parameter )
		MAP_INTERFACE( ITriReroutable )
		MAP_INTERFACE( IInitialize )

		////////////////////////////////////////////////////////////////////////////
		//               name
		MAP_ATTRIBUTE
		( 
			"name",    
			m_name,    
			"na", 
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)
		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE
		(
			"value",
			m_value,
			"",
			Be::PERSISTONLY
		)

		MAP_PROPERTY
		(
			"value", 
			GetValue, 
			SetValue, 
			"Value of the Vector3 parameter" 
		)

		MAP_PROPERTY
		(
			"x",
			GetX,
			SetX,
			"X component value"
		)

		MAP_PROPERTY
		(
			"y",
			GetY,
			SetY,
			"Y component value"
		)

		MAP_PROPERTY
		(
			"z",
			GetZ,
			SetZ,
			"Z component value"
		)

		MAP_PROPERTY
		(
			"w",
			GetW,
			SetW,
			"W component value"
		)

		MAP_PROPERTY
		(
			"v1",
			GetX,
			SetX,
			"X component value"
		)

		MAP_PROPERTY
		(
			"v2",
			GetY,
			SetY,
			"Y component value"
		)

		MAP_PROPERTY
		(
			"v3",
			GetZ,
			SetZ,
			"Z component value"
		)

		MAP_PROPERTY
		(
			"v4",
			GetW,
			SetW,
			"W component value"
		)

		////////////////////////////////////////////////////////////////////////////
		MAP_ATTRIBUTE( "usedByCurrentTechnique", m_isUsedByEffect, "na", Be::READ )
		MAP_ATTRIBUTE( "usedByCurrentEffect", m_isUsedByEffect, "na", Be::READ )
		MAP_ATTRIBUTE( "isSrgb", m_isSrgb, "indicates if (rgb) values will be converted to linear space", Be::READ )
		

	EXPOSURE_END()
}
