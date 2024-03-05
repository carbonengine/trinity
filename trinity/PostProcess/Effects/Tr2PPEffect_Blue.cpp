////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPEffect.h"

BLUE_DEFINE( Tr2PPEffect );

const Be::ClassInfo* Tr2PPEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPEffect, "" )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "display", m_display, "Should this be rendered", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "dirty", m_isDirty, "", Be::READ )

	EXPOSURE_END()

}

