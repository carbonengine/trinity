#include "StdAfx.h"
#include "EvePlanet.h"
#include "EveTransform.h"

BLUE_DEFINE( EvePlanet );

const Be::ClassInfo* EvePlanet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EvePlanet, "" )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2SecondaryLightSource )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( IEveEffectChildrenOwner )
		MAP_INTERFACE( IShaderConfigurer )
		MAP_INTERFACE( ITr2SoundEmitterOwner )
		MAP_INTERFACE( IWorldPosition )
	
		MAP_ATTRIBUTE
		(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"lodLevel",
			m_lodLevel,
			"",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"translationCurve",
			m_ballPosition,
			"Vector function slot for attaching a destiny ball to set the position of an object",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"worldTransform",
			m_worldTransform,
			"The world transform of this planet.",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"rotationCurve",
			m_ballRotation,
			"Quaternion function slot for attaching a destiny ball to set the rotation of an object",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"radius",
			m_radius,
			"The planet's radius",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"albedoColor",
			m_albedoColor,
			"Planet albedo color, used for secondary lighting",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"minScreenSize",
			m_minScreenSize,
			"Minimum screen size for planet rendering",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"scaling",
			m_scaling,
			"Planet scaling.",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"rotation", 
			m_rotation,
			"Planet rotation.",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE
		(
			"effectChildren",
			m_effectChildren,
			"Any effect children on planet.",
			Be::READ | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"observers",
			m_observers,
			"Observers for pushing data between modules every frame. Currently used to push locator data out to the audio2 module.",
			Be::READ | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"curveSets",
			m_curveSets,
			"Curvesets for animating things",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"secondaryLightingEmissiveColor",
			m_secondaryLightingEmissiveColor,
			"Color of the secondary light source",
			Be::READWRITE | Be::PERSIST )
		MAP_METHOD_AND_WRAP(
			"Start",
			Start,
			"Play all top level curveSets\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/play.png" )
		MAP_METHOD_AND_WRAP(
			"Stop",
			Stop,
			"Stop all top level curveSets.\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/stop.png" )
		MAP_METHOD_AND_WRAP(
			"SetControllerVariable",
			SetControllerVariable,
			"Set variable for all applicable controllers\n"
			":param name: variable name\n"
			":param value: new variable value\n")
		MAP_METHOD_AND_WRAP(
			"HandleControllerEvent",
			HandleControllerEvent,
			"Pass an event to controllers\n"
			":param name: event name")
		MAP_METHOD_AND_WRAP(
			"StartControllers",
			StartControllers,
			"Start all controllers")
		MAP_METHOD_AND_WRAP
		(
			"FreezeHighDetailMesh",
			FreezeHighDetailMesh,
			"" )
	EXPOSURE_CHAINTO(EveEffectRoot2)
}
