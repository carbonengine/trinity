////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPTaaEffect.h"

BLUE_DEFINE( Tr2PPTaaEffect );


const Be::VarChooser Tr2PPTaaEffectQualityChooser[] = {
	{ "Low", BeCast( Tr2PPTaaEffect::Quality::TAA_LOW ), "Low Quality" },
	{ "Medium", BeCast( Tr2PPTaaEffect::Quality::TAA_MEDIUM ), "Medium Quality" },
	{ "High", BeCast( Tr2PPTaaEffect::Quality::TAA_HIGH ), "High Quality" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "TaaQuality", Tr2PPTaaEffect::Quality, Tr2PPTaaEffectQualityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );


const Be::ClassInfo* Tr2PPTaaEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPTaaEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE_WITH_CHOOSER( "quality", m_quality, "Taa Quality", Be::READWRITE | Be::NOTIFY | Be::ENUM, Tr2PPTaaEffectQualityChooser )
		MAP_ATTRIBUTE( "earlyOutThreshold", m_earlyOutThreshold, "Controls the threshold used to skip calculations when a larger area is close to a flat color", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "showMotionVectors", m_showMotionVectors, "Shows motion vectors used by the new TAA algorithm", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "showEarlyOutMask", m_showEarlyOutMask, "Shows the early out mask of the new TAA algorithm", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		
		MAP_ATTRIBUTE( "jitterX", m_jitterX, "x jittering", Be::READ )
		MAP_ATTRIBUTE( "jitterY", m_jitterY, "y jittering", Be::READ )

		
		EXPOSURE_CHAINTO( Tr2PPEffect )


}

