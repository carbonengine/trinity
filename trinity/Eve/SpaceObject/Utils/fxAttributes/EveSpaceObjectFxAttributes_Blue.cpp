// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveSpaceObjectFxAttributes.h"

BLUE_DEFINE( EveSpaceObjectFxAttributes );

const Be::ClassInfo* EveSpaceObjectFxAttributes::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSpaceObjectFxAttributes, "" )
		MAP_INTERFACE( EveSpaceObjectFxAttributes )
		MAP_INTERFACE( IEveFxAttribute )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "activationStrength", m_activationStrength, "activityStrength of the spaceObject parent \n:jessica-group: General", Be::READ )
		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphereRadius, "SpaceObject Parent bounding sphere radius \n:jessica-group: General", Be::READ )
		MAP_ATTRIBUTE( "generatedShapeEllipsoidCenter", m_generatedShapeEllipsoidCenter, "Generated or User-authored ellipsoid data for center \n:jessica-group: General", Be::READ )
		MAP_ATTRIBUTE( "generatedShapeEllipsoidRadius", m_generatedShapeEllipsoidRadius, "Generated or User-authored ellipsoid data for radii \n:jessica-group: General", Be::READ )
		MAP_ATTRIBUTE( "activeTurretCount", m_activeTurretCount, "Count of how many active turrets are on the spaceObject parent \n:jessica-group: General", Be::READ )
		MAP_ATTRIBUTE( "parentWorldTranslation", m_parentWorldTranslation, "the world translation of the spaceObject parent\n:jessica-group: General", Be::READ )
		MAP_ATTRIBUTE( "parentWorldRotation", m_parentWorldRotation, "the world rotation of the spaceObject parent\n:jessica-group: General", Be::READ )

		MAP_ATTRIBUTE( "ship", m_distanceToShip, "player ship distance to spaceObject-parent \n:jessica-group: DistanceTo", Be::READ )
		MAP_ATTRIBUTE( "childParent", m_distanceToChildParent, "distance to closest-parent \n:jessica-group: DistanceTo", Be::READ )

		MAP_ATTRIBUTE( "killCount", m_killCount, "How many ships has the SpaceObject owner killed \n:jessica-group: SpicyStuff", Be::READ )


	EXPOSURE_END()
}