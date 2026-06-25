// Copyright © 2021 CCP ehf.

#include "EveProceduralMethodAttributeMapParameter.h"


BLUE_DEFINE( EveProceduralMethodAttributeMapParameter );

const Be::ClassInfo* EveProceduralMethodAttributeMapParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveProceduralMethodAttributeMapParameter, "" )
		MAP_INTERFACE( EveProceduralMethodAttributeMapParameter )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "a descriptive name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "child", m_child, ":jessica-icon: fa-suitcase-rolling", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_END()
}
