// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveVirtualCameraSystem.h"
#include "EveVirtualCamera.h"

BLUE_DEFINE( EveVirtualCameraSystem );

const Be::ClassInfo* EveVirtualCameraSystem::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveVirtualCameraSystem, "Manages a set of virtual cameras to be consumed by a real camera." )
		MAP_ATTRIBUTE( "cameras", m_cameras, "The cameras.", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "transition", m_transition, "The current transition taking place.\n:jessica-hidden: true\n", Be::READ )
		MAP_ATTRIBUTE( "mainCamera", m_mainCamera, "The current camera according to the system.\n:jessica-widget: objectref\n", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "externalCamera", m_externalCamera, "A camera to be manipulated by external systems, usually through UpdateExternal, to act as a camera to control entering and exiting the VCS.\n:jessica-widget: objectref\n", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "GetCurrentCamera", GetCurrentCamera, "Get the current camera, taking into account any temporary, transition cameras." )
		MAP_METHOD_AND_WRAP( "GetMainCamera", GetMainCamera, "Get whichever camera is either the main one or is marked to become it after a transition completes." )

		MAP_METHOD_AND_WRAP(
			"CutToCamera",
			CutToCamera,
			"Cut to the target camera.\n"
			":param camera: A camera to cut to, cannot be None.\n"
			":type camera: trinity.EveVirtualCamera\n" )
		MAP_METHOD_AND_WRAP(
			"LerpToCamera",
			LerpToCamera,
			"Lerp to the target camera.\n"
			":param camera: A camera to cut to, cannot be None.\n"
			":type camera: trinity.EveVirtualCamera\n"
			":param lerpTime: the time to perform the lerp transition over.\n"
			":type lerpTime: float\n" )

		MAP_METHOD_AND_WRAP( "IsExternallyControlled", IsExternallyControlled, "True if the current camera in use is the \"externalCamera\", thus the system is expected to be getting driven by an external system via this camera." )

		MAP_METHOD_AND_WRAP(
			"GetCameraByName",
			GetCameraByName,
			"Get the first camera found matching the given name.\n"
			":param cameraName: The name of the camera to find.\n"
			":type cameraName: str\n" )
	EXPOSURE_END()
}
