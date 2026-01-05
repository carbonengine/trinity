#include "StdAfx.h"
#include "EveChildBehaviorSystem.h"

BLUE_DEFINE( EveChildBehaviorSystem );
const Be::ClassInfo* EveChildBehaviorSystem::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildBehaviorSystem, "" )
		MAP_INTERFACE( EveChildBehaviorSystem )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "instanceCount", m_instanceCount, "", Be::READ )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "Should local transform be built from scaling, rotation and translation attributes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "Does local transform need to be rebuilt every frame.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "behaviorGroups", m_behaviorGroups, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "splineTunnels", m_splineTunnels, "", Be::READ | Be::PERSIST | Be::NOTIFY )

		MAP_METHOD_AND_WRAP( 
			"GetVertexElementAddedThroughCode", 
			GetVertexElementAddedThroughCode, 
			"for validation and objects requiring vertex elements added to the shader through code\n:jessica-hidden: True" 
		)
		
	EXPOSURE_END()
}