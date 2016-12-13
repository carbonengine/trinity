////////////////////////////////////////////////////////////
//
//    Created:   October 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EveMobile.h"

BLUE_DEFINE( EveMobile );

const Be::ClassInfo* EveMobile::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveMobile, "" )
        MAP_INTERFACE( EveMobile )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "activationStrengthCurve", m_activationStrengthCurve, "This one can be used to animated the activationStrength parameter", Be::READWRITE )
		MAP_ATTRIBUTE( "playActivationCurve", m_playActivationCurve, "This one can be used to animated the activationStrength parameter", Be::READWRITE )
		MAP_ATTRIBUTE( "activationDelta", m_activationDelta, "Activation strength curve progress", Be::READ )
		MAP_ATTRIBUTE( "activationStrength", m_activationStrenght, "Ship's activation strength", Be::READWRITE )
		MAP_ATTRIBUTE( "clipSphereFactor", m_clipSphereFactor, "Ship's clip state", Be::READWRITE )
		MAP_ATTRIBUTE( "clipSphereCenter", m_clipSphereCenter, "Ship's clip sphere center", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "playClipSphereFactorCurve", m_playClipSphereFactorCurve, "Set whether or not the clip sphere factorcurve is playing", Be::READWRITE )
		MAP_ATTRIBUTE( "clipSphereFactorCurve", m_clipSphereFactorCurve, "Set the curve used to play the clip sphere factor animation", Be::READWRITE )
		MAP_ATTRIBUTE( "clipSphereFactorDelta", m_clipSphereFactorDelta, "Clip sphere factir progress", Be::READ )

		MAP_ATTRIBUTE( "turretSets", m_turretSets, "a list of all the turret sets on this ship", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_METHOD_AND_WRAP( "GetTurretLocatorCount", GetTurretLocatorCount, "Returns the turret locator count of locators and bones matching the correct naming scheme." )
        MAP_METHOD_AND_WRAP( "RebuildTurretPositions", RebuildTurretPositions, "Re-positions all the turrets on this ship" )
		MAP_METHOD_AND_WRAP( "PlayActivationCurve", PlayActivationCurve, "Play the object's activation strength curve" )
		MAP_METHOD_AND_WRAP( "PlayClipSphereFactorCurve", PlayClipSphereFactorCurve, "Play the object's clip sphere factor curve" )
		MAP_METHOD_AND_WRAP( 
			"ModifyClipSphereCurve", 
			ModifyClipSphereCurve, 
			"Modifies the clip sphere curve by changing it's length and current time\n"
			":param parameters: a strange way to pass arguments to a function"
			)


    EXPOSURE_CHAINTO( EveSpaceObject2 )
}
