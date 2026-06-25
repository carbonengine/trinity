// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepTestBlocking.h"

BLUE_DEFINE( TriStepTestBlocking );


const Be::ClassInfo* TriStepTestBlocking::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepTestBlocking, "" )
		MAP_INTERFACE( TriStepTestBlocking )

		MAP_ATTRIBUTE(
			"inProgress",
			m_inProgress,
			"This render step will return in progress while this is set",
			Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( TriRenderStep )
}