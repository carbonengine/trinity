// Copyright © 2021 CCP ehf.

#include "EveProceduralMethodRandomParameter.h"


BLUE_DEFINE( EveProceduralMethodRandomParameter );

const Be::ClassInfo* EveProceduralMethodRandomParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveProceduralMethodRandomParameter, "" )
		MAP_INTERFACE( EveProceduralMethodRandomParameter )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "a descriptive name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weighting", m_weighting, "adjust the likelihood of this child being chosen relative to total weighting", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "child", m_child, ":jessica-icon: fa-suitcase-rolling", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_END()
}
