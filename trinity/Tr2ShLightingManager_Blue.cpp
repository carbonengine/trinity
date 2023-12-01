////////////////////////////////////////////////////////////
//
//    Created:   June 2014
//    Copyright: CCP 2014
//

#include "StdAfx.h"
#include "Tr2ShLightingManager.h"

BLUE_DEFINE_INTERFACE( ITr2SecondaryLightSource );
BLUE_DEFINE_INTERFACE( ITr2ShLightingReceiver );
BLUE_DEFINE( Tr2ShLightingManager );

const Be::VarChooser Tr2ShLightingManagerQualityChooser[] =
{
	{ "L1",	BeCast( Tr2ShLightingManager::L1 ),	"" }, 
	{ "L2",	BeCast( Tr2ShLightingManager::L2 ),	"" }, 
	{0}
};

BLUE_REGISTER_ENUM( "ShQuality", Tr2ShLightingManager::Quality, Tr2ShLightingManagerQualityChooser );

const Be::ClassInfo* Tr2ShLightingManager::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2ShLightingManager, "" )
        MAP_INTERFACE( Tr2ShLightingManager )

		MAP_ATTRIBUTE( 
			"primaryIntensity", 
			m_primaryIntensity, 
			"Overall intensity for primary SH lighting (from a list of point lights)", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"secondaryIntensity", 
			m_secondaryIntensity, 
			"Overall intensity for secondary SH lighting", 
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( 
			"quality", 
			m_quality, 
			"Lighting quality (SH order)", 
			Be::READWRITE | Be::PERSIST | Be::ENUM,
			Tr2ShLightingManagerQualityChooser )
		MAP_ATTRIBUTE( 
			"lights", 
			m_lights, 
			"Additional lights", 
			Be::READ | Be::PERSIST )
	EXPOSURE_END()
}