#include "StdAfx.h"
#include "EveCameraFxAttributes.h"

BLUE_DEFINE( EveCameraFxAttributes );

BLUE_DEFINE_INTERFACE( IEveFxAttribute );

const Be::ClassInfo* EveCameraFxAttributes::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveCameraFxAttributes, "" )
		MAP_INTERFACE( EveCameraFxAttributes )
		MAP_INTERFACE( IEveFxAttribute )
		
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "distanceToCamera", m_distanceToCamera, "spaceObject distance to camera\n:jessica-group: General", Be::READ )
		MAP_ATTRIBUTE( "lookAngleToObject", m_lookAngleToObject, "\n:jessica-group: General", Be::READ )

		MAP_ATTRIBUTE( "objectRotation", m_objectRotation, "\n:jessica-group: Rotation", Be::READ )
		MAP_ATTRIBUTE( "rotationWithChildTransform", m_rotationWithChildTransform, "\n:jessica-group: Rotation", Be::READ )
		MAP_ATTRIBUTE( "cameraRotation", m_cameraRotation, "\n:jessica-group: Rotation", Be::READ )
	
	EXPOSURE_END()
}