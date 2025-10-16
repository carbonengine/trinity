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
			"translationCurve",
			m_ballPosition,
			"Function for animated position updates",
			Be::READWRITE | Be::PERSIST
		)
		
		MAP_ATTRIBUTE
		(
			"rotationCurve",
			m_ballRotation,
			"Function for animated rotation updates",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"display",
			m_display,
			"Not really used for audio objects, but here for consistency with the EveSpaceObject interface",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"externalParameters",
			m_externalParameters,
			"List of external parameters to bind to object elements",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"audioEmitter",
			m_audioEmitter,
			"Exposure of the audio emitter object",
			Be::READWRITE | Be::PERSIST
		)

		MAP_METHOD_AND_WRAP( "GetAudioEmitter", GetAudioEmitter, "Gets the audio emitter for this object" )

		MAP_METHOD_AND_WRAP( "SetEmitterName", SetEmitterName, "Sets the name of this object's audio emitter" )

		MAP_METHOD_AND_WRAP( "PlayAudioEvent", PlayAudioEvent, "Plays an audio event on this object's emitter" )

	MAP_METHOD_AND_WRAP( "__init__", py__init__, "Initialize the audio emitter after construction/deserialization" )

    EXPOSURE_END();
}