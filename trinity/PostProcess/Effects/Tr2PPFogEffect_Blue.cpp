////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPFogEffect.h"

BLUE_DEFINE( Tr2PPFogEffect );

const Be::ClassInfo* Tr2PPFogEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPFogEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "intensity", m_intensity, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "color", m_color, "Fog color", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "colorInfluence", m_colorInfluence, "Constant color vs original+nebula color coefficient", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "nebulaInfluence", m_nebulaInfluence, "Nebula influence coefficient", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "nebulaBlur", m_nebulaBlur, "Nebula mip level to take", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "originalBrightenOnly", m_originalBrightenOnly, "Only brighten the original image with nebula", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "totalAmount", m_totalAmount, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "totalPower", m_totalPower, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "backgroundOcclusion", m_backgroundOcclusion, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "brightnessThreshold0", m_brightnessThreshold0, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "brightnessThreshold1", m_brightnessThreshold1, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "brightnessAdjustmentAmount", m_brightnessAdjustmentAmount, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendDistance0", m_blendDistance0, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendBias0", m_blendBias0, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendAmount0", m_blendAmount0, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendPower0", m_blendPower0, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendDistance1", m_blendDistance1, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendBias1", m_blendBias1, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendAmount1", m_blendAmount1, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendPower1", m_blendPower1, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendDistance2", m_blendDistance2, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendBias2", m_blendBias2, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendAmount2", m_blendAmount2, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "blendPower2", m_blendPower2, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "areaSize", m_areaSize, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "areaScale", m_areaScale, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "areaCenter", m_areaCenter, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		
		EXPOSURE_CHAINTO( Tr2PPEffect )
}

