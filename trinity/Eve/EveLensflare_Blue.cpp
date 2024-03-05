////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//
#include "StdAfx.h"
#include "EveLensflare.h"

BLUE_DEFINE( EveLensflare );

const Be::ClassInfo* EveLensflare::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveLensflare, "" )
        MAP_INTERFACE( EveLensflare )
		MAP_INTERFACE( ITr2ControllerOwner )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "A name for this decal", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "display", m_display, "Toggle visibility", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "update", m_update, "Toggle updates", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "cameraFactor", m_cameraFactor, "na", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE("mesh", m_mesh, "A mesh that is rendered as a flare", Be::READWRITE | Be::PERSIST)

		MAP_ATTRIBUTE( "position", m_position, "position of this lensflare", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translationCurve", m_translationCurve, "", Be::READWRITE )

		MAP_ATTRIBUTE( "flares", m_flares, "the actual content: each lensflare is made of a couple of single flares", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "occluders", m_occluders, "foreground (ships, stations, etc.) occluders module of this lensflare", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "backgroundOccluders", m_backgroundOccluders, "background (planets) occluders module of this lensflare", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"distanceToEdgeCurves", 
			m_distanceToEdgeCurves, 
			"List of curves that accept biased distance to closest edge (0 at center, 1 at edge, >1 ouside the screen) as an argument", 
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"distanceToCenterCurves", 
			m_distanceToCenterCurves, 
			"List of curves that accept distance to screen center as an argument", 
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"radialAngleCurves", 
			m_radialAngleCurves, 
			"List of curves that accept screen position polar coordinate angle as an argument", 
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"xDistanceToCenter", 
			m_xDistanceToCenter, 
			"List of curves that accept signed distance to screen center along x axis as an argument (biased by +10)", 
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"yDistanceToCenter", 
			m_yDistanceToCenter, 
			"List of curves that accept signed distance to screen center along y axis as an argument (biased by +10)", 
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"bindings", 
			m_bindings, 
			"List of value bindings", 
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"curveSets", 
			m_curveSets, 
			"List of curve sets", 
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "controllers", m_controllers, "List of object controllers", Be::READ | Be::PERSIST )
		MAP_METHOD_AND_WRAP(
			"SetControllerVariable",
			SetControllerVariable,
			"Set variable for all applicable controllers\n"
			":param name: variable name\n"
			":param value: new variable value\n"
		)
		MAP_METHOD_AND_WRAP(
			"StartControllers",
			StartControllers,
			"Start all controllers"
		)
		
	EXPOSURE_END()
}
