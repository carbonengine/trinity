////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPTaaEffect.h"

BLUE_DEFINE( Tr2PPTaaEffect );

const Be::ClassInfo* Tr2PPTaaEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPTaaEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "blendParams0", m_blendParams0, "", Be::READ | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendParams1", m_blendParams1, "", Be::READ | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendParams2", m_blendParams2, "", Be::READ | Be::NOTIFY )
		MAP_ATTRIBUTE( "distanceParams", m_distanceParams, "", Be::READ | Be::NOTIFY )
		MAP_ATTRIBUTE( "enhancementParams", m_enhancementParams, "", Be::READ | Be::NOTIFY )
		
		EXPOSURE_CHAINTO( Tr2PPEffect )


}

