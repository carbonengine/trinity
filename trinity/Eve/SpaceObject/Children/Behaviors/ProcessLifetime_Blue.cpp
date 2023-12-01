#include "StdAfx.h"
#include "ProcessLifetime.h"

Be::VarChooser ProcessLifetimeTunnelGroupTypeChooser[] =
{
	{ "Exit_Tunnels", BeCast( SplineTunnelGroup::EXIT_TUNNELS ), "Tunnels Drones flock to when set to exit the scene" },
	{ "Entrance_Tunnels", BeCast( SplineTunnelGroup::ENTRANCE_TUNNELS ), "Tunnels Drones flock to when entering the scene" },
	{ "Other_Tunnels", BeCast( SplineTunnelGroup::OTHER_TUNNELS ), "pathways in the scene (hallways etc)" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "setTunnelType", SplineTunnelGroup::TunnelGroupType, ProcessLifetimeTunnelGroupTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );


BLUE_DEFINE( ProcessLifetime );

extern Be::VarChooser BehaviorPriorityChooser[];

const Be::ClassInfo* ProcessLifetime::ExposeToBlue()
{
	EXPOSURE_BEGIN( ProcessLifetime, "" )
		MAP_INTERFACE( ProcessLifetime )
		MAP_INTERFACE( IBehavior )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE_WITH_CHOOSER( "behaviorPriority", m_priority, "control what priority this behavior should have", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BehaviorPriorityChooser )

		MAP_ATTRIBUTE( "exit", m_exit, "Trigger to have drones search for an exit tunnel", Be::READWRITE )
		MAP_ATTRIBUTE( "firstAgentLifetime", m_firstAgentLifetime, "Current lifetime for first agent in the group", Be::READ )
		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, "pull force multiplier", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "returningAge", m_returningAge, "designate when agent should exit the scene", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "wanderAmount", m_wanderAmount, "How much randomness should the drones wander around the tunnel, value between [0-1]", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "respawnAgentsOnDeath", m_respawnAgentsOnDeath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "splineTunnels", m_splineTunnels, "", Be::READ | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "firstSpawnAtRandomPlaces", m_firstSpawnAtRandomPlaces, "If true, on load drones will spawn around the tunnel", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		

	EXPOSURE_END()
}