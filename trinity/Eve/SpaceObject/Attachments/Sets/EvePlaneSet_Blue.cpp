// Copyright © 2013 CCP ehf.

#include "StdAfx.h"
#include "EvePlaneSet.h"

BLUE_DEFINE( EvePlaneSet );

const Be::ClassInfo* EvePlaneSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EvePlaneSet, "" )
		MAP_INTERFACE( EvePlaneSet )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IEveSpaceObjectAttachment )
		MAP_INTERFACE( ITr2LightOwner )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE( "display", m_display, "Specifies whether to render this set or not", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hideOnLowQuality", m_hideOnLowQuality, "Disables this whole planeset when low quaility is selected.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "Standard name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "pickBufferID", m_pickBufferID, "A user-specified ID which is used for click detection (acts as areaID)", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "skinned", m_isSkinned, "Is the plane set skinned (requires that the owner object is skinned)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "planes", m_planes, "The list of all plane items", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "effect", m_effect, "Effect to use for rendering planes", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "Rebuild", Rebuild, "Rebuild resources after adding/removing/changing individual items" )

	EXPOSURE_END()
}
