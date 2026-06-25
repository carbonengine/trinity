// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriFloatArrayParameter.h"

BLUE_DEFINE( TriVector4 );

const Be::ClassInfo* TriVector4::ExposeToBlue(){
	EXPOSURE_BEGIN( TriVector4, "" )
		MAP_INTERFACE( TriVector4 )

			MAP_ATTRIBUTE( "data", m_data, "na", Be::READWRITE | Be::PERSIST )
				EXPOSURE_END()
}



BLUE_DEFINE( TriFloatArrayParameter );

const Be::ClassInfo* TriFloatArrayParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriFloatArrayParameter, "" )
		MAP_INTERFACE( ITriEffectParameter )
		MAP_INTERFACE( TriFloatArrayParameter )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "na", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "value", m_value, "na", Be::READ | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "usedByCurrentTechnique", m_isUsedByEffect, "na", Be::READ )
		MAP_ATTRIBUTE( "usedByCurrentEffect", m_isUsedByEffect, "na", Be::READ )
	EXPOSURE_END()
}
