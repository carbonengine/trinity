// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Eve/EveCamera.h"

BLUE_DEFINE( EveCamera );

const Be::ClassInfo* EveCamera::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveCamera, "" )
		MAP_INTERFACE( EveCamera )
		MAP_INTERFACE( INotify )

		////////////////////////////////////////////////////////////////////////////
		//               parent
		MAP_ATTRIBUTE(
			"parent",
			m_parentTranslationCurve,
			"na",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"audio2Listener",
			m_audio2Listener,
			"Audio2 listener",
			Be::READWRITE )

		////////////////////////////////////////////////////////////////////////////
		//               translationFromParent
		MAP_ATTRIBUTE(
			"translationFromParent",
			m_translationFromParent.z,
			"na",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               rotationAroundParent
		MAP_ATTRIBUTE(
			"rotationAroundParent",
			m_rotationAroundParent,
			"na",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               interest
		MAP_ATTRIBUTE(
			"interest",
			m_interestTranslationCurve,
			"na",
			Be::READWRITE | Be::NOTIFY )

		////////////////////////////////////////////////////////////////////////////
		//               rotationOfInterest
		MAP_ATTRIBUTE(
			"rotationOfInterest",
			m_rotationOfInterest,
			"na",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		////////////////////////////////////////////////////////////////////////////
		//               minPitch
		MAP_ATTRIBUTE(
			"minPitch",
			m_minPitch,
			"na",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               maxPitch
		MAP_ATTRIBUTE(
			"maxPitch",
			m_maxPitch,
			"na",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               minYaw
		MAP_ATTRIBUTE(
			"minYaw",
			m_minYaw,
			"na",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               maxYaw
		MAP_ATTRIBUTE(
			"maxYaw",
			m_maxYaw,
			"na",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               alignment
		MAP_ATTRIBUTE(
			"alignment",
			m_alignment,
			"This is the base up vector for the view transform",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               fieldOfView
		MAP_ATTRIBUTE(
			"fieldOfView",
			m_fieldOfView,
			"na",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               frontClip
		MAP_ATTRIBUTE(
			"frontClip",
			m_frontClip,
			"na",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               backClip
		MAP_ATTRIBUTE(
			"backClip",
			m_backClip,
			"na",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               friction
		MAP_ATTRIBUTE(
			"friction",
			m_friction,
			"na",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               Idle movement on or off
		MAP_ATTRIBUTE(
			"idleMove",
			m_idleMove,
			"Whether camera should rock gently while idle",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               Amplitude of idle hover
		MAP_ATTRIBUTE(
			"idleScale",
			m_idleScale,
			"The amplitude of the idle movement",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               Speed of idle move
		MAP_ATTRIBUTE(
			"idleSpeed",
			m_idleSpeed,
			"How fast is the movement",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               camera noise on/off
		MAP_ATTRIBUTE(
			"noise",
			m_noise,
			"Camera noise on off",
			Be::READWRITE | Be::PERSIST )


		////////////////////////////////////////////////////////////////////////////
		//               Amplitude of camera noise
		MAP_ATTRIBUTE(
			"noiseScale",
			m_noiseScale,
			"Amplitude of camera noise",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               Damping of noise
		MAP_ATTRIBUTE(
			"noiseDamp",
			m_noiseDamp,
			"How spastic is your noise",
			Be::READWRITE | Be::PERSIST )
		////////////////////////////////////////////////////////////////////////////
		//               noiseCurve
		MAP_ATTRIBUTE(
			"noiseCurve",
			m_noiseCurve,
			"na",
			Be::READWRITE | Be::PERSIST )
		////////////////////////////////////////////////////////////////////////////
		//               noiseScaleCurve
		MAP_ATTRIBUTE(
			"noiseScaleCurve",
			m_noiseScaleCurve,
			"na",
			Be::READWRITE | Be::PERSIST )
		////////////////////////////////////////////////////////////////////////////
		//               noiseDampCurve
		MAP_ATTRIBUTE(
			"noiseDampCurve",
			m_noiseDampCurve,
			"na",
			Be::READWRITE | Be::PERSIST )


		////////////////////////////////////////////////////////////////////////////
		//               maxSpeed
		MAP_ATTRIBUTE(
			"maxSpeed",
			m_maxSpeed,
			"na",
			Be::READWRITE | Be::PERSIST )


		////////////////////////////////////////////////////////////////////////////
		//               update
		MAP_ATTRIBUTE(
			"update",
			m_update,
			"na",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               pos
		MAP_ATTRIBUTE(
			"pos",
			m_pos,
			"na",
			Be::READ | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               intr
		MAP_ATTRIBUTE(
			"intr",
			m_intr,
			"na",
			Be::READ | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               viewVec
		MAP_ATTRIBUTE(
			"viewVec",
			m_viewVec,
			"na",
			Be::READ )

		////////////////////////////////////////////////////////////////////////////
		//               rightVec
		MAP_ATTRIBUTE(
			"rightVec",
			m_rightVec,
			"na",
			Be::READ )

		////////////////////////////////////////////////////////////////////////////
		//               upVec
		MAP_ATTRIBUTE(
			"upVec",
			m_upVec,
			"na",
			Be::READ )

		////////////////////////////////////////////////////////////////////////////
		//               zoomCurve
		MAP_ATTRIBUTE(
			"zoomCurve",
			m_zoomCurve,
			"na",
			Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		// camera plane shift
		MAP_ATTRIBUTE(
			"centerOffset",
			m_projectionCenterOffset,
			"Camera's center offset",
			Be::READWRITE )


		MAP_ATTRIBUTE( "extraTranslation", m_extraParentTranslation, "Extra offset from lookat target(parent)", Be::READWRITE );
		MAP_ATTRIBUTE( "useExtraTranslation", m_useExtraParentTranslation, "Toggle weather extraTranslation should be used", Be::READWRITE );

		////////////////////////////////////////////////////////////////////////////
		// the camera's current pitch and yaw
		MAP_ATTRIBUTE(
			"pitch",
			m_pitch,
			"Camera's current pitch",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"yaw",
			m_yaw,
			"Camera's current yaw",
			Be::READ | Be::PERSIST )

		// Handler for doing error reporting/raising
		MAP_ATTRIBUTE( "errorHandler", m_errorListener, "A listener that gets called if look at target is invalid", Be::READWRITE );

		MAP_METHOD_AND_WRAP(
			"SetRotationOnOrbit",
			SetRotationOnOrbit,
			"Sets the rotation away from the parent look-at direction in terms of yaw and pitch\n"
			":param yaw: yaw in radians\n"
			":param pitch: pitch in radians" )

		MAP_METHOD_AND_WRAP(
			"SetOrbit",
			SetOrbit,
			"Sets the orbit around parent in terms of yaw and pitch\n"
			":param yaw: yaw in radians\n"
			":param pitch: pitch in radians" )

		MAP_ATTRIBUTE(
			"projectionMatrix",
			m_projectionMatrix,
			"TriProjection representing the camera's current projection",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"viewMatrix",
			m_viewMatrix,
			"TriView representing the camera's current view matrix",
			Be::READ )

		MAP_METHOD_AND_WRAP(
			"OrbitParent",
			OrbitParent,
			":param horizontal: \n"
			":param vertical: \n" )
		MAP_METHOD_AND_WRAP(
			"RotateOnOrbit",
			RotateOnOrbit,
			":param horizontal: \n"
			":param vertical: \n" )
		MAP_METHOD_AND_WRAP(
			"Dolly",
			Dolly,
			":param factor: " )
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"Zoom",
			Zoom,
			1,
			":param key: \n" )

		MAP_METHOD_AND_WRAP(
			"ResetStartTime",
			ResetStartTime,
			"Resets start time for noise curves" )

	EXPOSURE_END();
}