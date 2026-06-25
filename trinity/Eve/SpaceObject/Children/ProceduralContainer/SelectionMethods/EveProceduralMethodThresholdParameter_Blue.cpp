// Copyright © 2021 CCP ehf.

#include "EveProceduralMethodThresholdParameter.h"


BLUE_DEFINE( EveProceduralMethodThresholdParameter );

const Be::ClassInfo* EveProceduralMethodThresholdParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveProceduralMethodThresholdParameter, "" )
		MAP_INTERFACE( EveProceduralMethodThresholdParameter )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "a descriptive name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "threshold", m_threshold, "parameter will be selected if seed is lower than this value but higher than others passing the threshold", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "child", m_child, ":jessica-icon: fa-suitcase-rolling", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_END()
}
