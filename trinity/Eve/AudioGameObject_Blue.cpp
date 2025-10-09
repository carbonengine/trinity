#include "StdAfx.h"
#include "AudioGameObject.h"

BLUE_DEFINE( AudioGameObject );

const Be::ClassInfo* AudioGameObject::ExposeToBlue()
{
    EXPOSURE_BEGIN( AudioGameObject, "Standalone audio object for 3D positioning without being attached to assets." )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IWorldPosition )
		MAP_INTERFACE( ITr2DebugRenderable )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE
		(
			"name",
			m_name,
			"Name of the audio object",
			Be::READWRITE | Be::PERSIST
		)
		
		MAP_ATTRIBUTE
		( 
			"mute",
			m_mute, 
			"Mute state of the audio emitter",
			Be::READWRITE | Be::NOTIFY
		)
		
		MAP_ATTRIBUTE
		(
			"rotation",
			m_rotation,
			"Local rotation of the object",
			Be::READWRITE | Be::PERSIST
		)
		
		MAP_ATTRIBUTE
		(
			"translation",
			m_translation,
			"Local translation of the object",
			Be::READWRITE | Be::PERSIST
		)
		
		MAP_ATTRIBUTE
		(
			"ballPosition",
			m_ballPosition,
			"Function for animated position updates",
			Be::READWRITE | Be::PERSIST
		)
		
		MAP_ATTRIBUTE
		(
			"ballRotation",
			m_ballRotation,
			"Function for animated rotation updates",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"externalParameters",
			m_externalParameters,
			"List of external parameters to bind to object elements",
			Be::READ | Be::PERSIST
		)

		MAP_METHOD_AND_WRAP( "SetEventName", SetEventName, "Sets the event name to play on wake" )

		MAP_METHOD_AND_WRAP( "SetPlayOnWake", SetPlayOnWake, "Sets whether to automatically play eventName when Initialize() is called" )

		MAP_METHOD_AND_WRAP( "GetAudioEmitter", GetAudioEmitter, "Gets the audio emitter for this object" )

		MAP_METHOD_AND_WRAP( "SetEmitterName", SetEmitterName, "Sets the name of this object's audio emitter" )

		MAP_METHOD_AND_WRAP( "PlayAudioEvent", PlayAudioEvent, "Plays an audio event on this object's emitter" )

    EXPOSURE_END();
}