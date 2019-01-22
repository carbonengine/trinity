////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PostProcess2.h"

BLUE_DEFINE( Tr2PostProcess2 );

const Be::ClassInfo* Tr2PostProcess2::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2PostProcess2, "" )

		MAP_INTERFACE( Tr2PostProcess2 )

		MAP_ATTRIBUTE( "signalLoss", m_signalLoss, "Accesses the Signal loss", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "godRays", m_godRays, "Accesses the God Rays effect", Be::READWRITE | Be::PERSIST )

    EXPOSURE_END()
}
