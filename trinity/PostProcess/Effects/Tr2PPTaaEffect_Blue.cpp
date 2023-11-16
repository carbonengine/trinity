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

		MAP_ATTRIBUTE( "quality", m_quality, "Quality from 1 to 3", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "applyMipBias", m_applyMipBias, "Applies a -1.0 mip bias to improve texture sharpness", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "earlyOutThreshold", m_earlyOutThreshold, "Controls the threshold used to skip calculations when a larger area is close to a flat color", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "showMotionVectors", m_showMotionVectors, "Shows motion vectors used by the new TAA algorithm", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "showEarlyOutMask", m_showEarlyOutMask, "Shows the early out mask of the new TAA algorithm", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		
		EXPOSURE_CHAINTO( Tr2PPEffect )


}

