////////////////////////////////////////////////////////////
//
//    Created:   April 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "BehaviorGroupBooster.h"


BLUE_DEFINE( BehaviorGroupBooster );

const Be::ClassInfo* BehaviorGroupBooster::ExposeToBlue()
{
	EXPOSURE_BEGIN( BehaviorGroupBooster, "" )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify)
			   
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		
		MAP_ATTRIBUTE( "boosterOffset", m_boosterOffset, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "atlasIndex0", m_atlasIndex0, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "atlasIndex1", m_atlasIndex1, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boosterEffect", m_boosterEffect, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "haloFlareEffect", m_haloFlareEffect, "The effect for the halo flare. \n:jessica-group: Halo Flare", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "haloFlareOffset", m_haloFlareOffset, "The offset for the halo flare. \n:jessica-group: Halo Flare", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "haloFlareScale", m_haloFlareScale, "The scale for the halo flare. \n:jessica-group: Halo Flare", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "haloFlareRotation", m_haloFlareRotation, "The rotation for the halo flare. \n:jessica-group: Halo Flare", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "haloFlareBrightness", m_haloFlareBrightness, "The brightness for the halo flare \n:jessica-group: Halo Flare", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "haloFlareColor", m_haloFlareColor, "The color for the halo flare. \n:jessica-group: Halo Flare", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		
		MAP_ATTRIBUTE( "ambientFlareEffect", m_ambientFlareEffect, "The effect for the ambient flare. \n:jessica-group: Ambient Flare", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "ambientFlareOffset", m_ambientFlareOffset, "The offset for the ambient flare. \n:jessica-group: Ambient Flare", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "ambientFlareScale", m_ambientFlareScale, "The scale for the ambient flare. \n:jessica-group: Ambient Flare", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "ambientFlareBrightness", m_ambientFlareBrightness, "The brightness for the ambient flare \n:jessica-group: Ambient Flare", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "ambientFlareColor", m_ambientFlareColor, "The color for the ambient flare. \n:jessica-group: Ambient Flare", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		
		MAP_ATTRIBUTE( "lightRadius", m_lightRadius, "The radius of the light. \n:jessica-group: Light", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightColor", m_lightColor, "The color of the light. \n:jessica-group: Light", Be::READWRITE | Be::PERSIST )
		
		MAP_ATTRIBUTE( "displayBoosters", m_displayBoosters, "Should the boosters be displayed \njessica-group: Debug", Be::READWRITE )
		MAP_ATTRIBUTE( "displayHazeFlare", m_displayHazeFlare, "Should the haze flare be displayed \njessica-group: Debug", Be::READWRITE  )
		MAP_ATTRIBUTE( "displayAmbientFlare", m_displayAmbientFlare, "Should the ambient flare be displayed \njessica-group: Debug", Be::READWRITE )

		EXPOSURE_END()
};

