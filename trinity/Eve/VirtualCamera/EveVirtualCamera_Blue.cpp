// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveVirtualCamera.h"

BLUE_DEFINE( EveVirtualCamera );

const Be::ClassInfo* EveVirtualCamera::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveVirtualCamera, "A virtual camera, managed by a VirtualCameraSystem which is, in turn, consumed by a real camera." )
		MAP_ATTRIBUTE( "name", m_name, "The name for the camera.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "animationTimelineLength", m_animationTimelineLength, "The length of time, in seconds, the camera's behaviours animate over.\n:jessica-widget: float\n:jessica-numeric-range: (0.0, 99999.0)", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "running", m_isRunning, "True if the camera is currently active.", Be::READ )

		MAP_ATTRIBUTE( "fov", m_fov, "The field of view.", Be::READ )
		MAP_ATTRIBUTE( "roll", m_roll, "The camera roll.", Be::READ )
		MAP_ATTRIBUTE( "position", m_position, "Camera position.", Be::READ )
		MAP_ATTRIBUTE( "pointOfInterest", m_pointOfInterest, "Point of interest.", Be::READ )

		MAP_ATTRIBUTE( "localElapsedTime", m_localElapsedTime, "Local elapsed time.", Be::READ )
		MAP_ATTRIBUTE( "positionAnchorCenter", m_positionAnchorCenter, "Center of the position anchor objects.", Be::READ )
		MAP_ATTRIBUTE( "positionAnchorRadius", m_positionAnchorRadius, "Radius of the position anchor objects bounding sphere.", Be::READ )
		MAP_ATTRIBUTE( "positionAnchorForwardDirection", m_positionAnchorForwardDirection, "Radius of the position anchor objects bounding sphere.", Be::READ )
		MAP_ATTRIBUTE( "pointOfInterestAnchorCenter", m_positionAnchorCenter, "Center of the point of interest anchor objects.", Be::READ )
		MAP_ATTRIBUTE( "pointOfInterestAnchorRadius", m_positionAnchorRadius, "Radius of the point of interest anchor objects bounding sphere.", Be::READ )
		MAP_ATTRIBUTE( "pointOfInterestAnchorForwardDirection", m_pointOfInterestAnchorForwardDirection, "Radius of the position anchor objects bounding sphere.", Be::READ )

		MAP_PROPERTY_READONLY( "forward", GetViewDirection, "The forward direction." )
		MAP_PROPERTY_READONLY( "right", GetRightDirection, "The right direction." )
		MAP_PROPERTY_READONLY( "up", GetUpDirection, "The up direction." )

		MAP_METHOD_AND_WRAP( "GetViewMatrix", GetViewMatrix, "The forward direction." )
		MAP_METHOD_AND_WRAP( "GetProjectionMatrix", GetProjectionMatrix, "The right direction." )

		MAP_METHOD_AND_WRAP(
			"UpdateExternal",
			UpdateExternal,
			"Not for general use, only for situations where you need a virtual camera bound to an external system. Do not use with behaviours.\n"
			":param position: the position of the camera.\n"
			":type position: tuple[float, float, float]\n"
			":param pointOfInterest: the position to look at.\n"
			":type pointOfInterest: tuple[float, float, float]\n"
			":param fov: the field of view.\n"
			":type fov: float\n"
			":param roll: the roll amount.\n"
			":type roll: float\n" )

		MAP_METHOD_AND_WRAP( "Play", Play, "Used for debugging, starting / stopping should be left up to the EveVirtualCameraSystem to which this camera belongs.\n:jessica-placement: BOTH\n:jessica-icon: far-play" )
		MAP_METHOD_AND_WRAP( "Pause", Pause, "Used for debugging, starting / stopping should be left up to the EveVirtualCameraSystem to which this camera belongs.\n:jessica-placement: BOTH\n:jessica-icon: far-pause" )
		MAP_METHOD_AND_WRAP( "Stop", Stop, "Used for debugging, starting / stopping should be left up to the EveVirtualCameraSystem to which this camera belongs.\n:jessica-placement: BOTH\n:jessica-icon: far-stop" )

		MAP_METHOD_AND_WRAP(
			"UpdateToLocalTime",
			UpdateToLocalTime,
			"Used to set the localElapsedTime. Simulates the camera moving through time to get to that localElapsedTime thus allowing behaviours to act appropriately.\n"
			":param time: the target playback time to scrub the camera to.\n"
			":type time: float\n" )

		MAP_ATTRIBUTE( "positionBehaviours", m_positionBehaviours, "The behaviours that drive the camera position.", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "pointOfInterestBehaviours", m_pointOfInterestBehaviours, "The behaviours that drive the camera point of interest.", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "fovBehaviours", m_fovBehaviours, "The behaviours that drive the camera fov.", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "rollBehaviours", m_rollBehaviours, "The behaviours that drive the camera roll.", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "positionAnchors", m_positionAnchors, "List of objects which act as the anchors for all position related behaviours.", Be::READ )
		MAP_ATTRIBUTE( "pointOfInterestAnchors", m_pointOfInterestAnchors, "List of objects which act as the anchors for all point of interest related behaviours.", Be::READ )
	EXPOSURE_END()
}
