#include "StdAfx.h"
#include "BehaviorGroup.h"

BLUE_DEFINE( BehaviorGroup );

const Be::ClassInfo* BehaviorGroup::ExposeToBlue()
{
	EXPOSURE_BEGIN( BehaviorGroup, "" )
		MAP_INTERFACE( BehaviorGroup )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "name", m_behaviorGroupName, "The name of this behavior group", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "update", m_update, "Toggle off for static behaviors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "count", m_count, "Number of drones to spawn when loading up", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "actualCount", m_actualCount, "Number of drones to spawn when loading up", Be::READ )
		MAP_ATTRIBUTE( "spawnPosition", m_spawnPosition, "Where the drones should spawn, leave as (0,0,0) if process priority behavior should control spawn pos", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxVelocity", m_maxVelocity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "behaviors", m_behaviors, "What the ships do etc", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "mesh", m_mesh, "the drone mesh", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "boosters", m_booster, "The boosters of the mesh", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "scale", m_scale, "Agent local scaling", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "currentScreenSize", m_currentScreenSize, "Current screen size for first agent in the group. Enable bounding-sphere primitive visualization for debugging.", Be::READ )
		MAP_ATTRIBUTE( "renderThreshold", m_renderThreshold, "If the screen-size of all agents is below this threshold the group will not be rendered. ", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blendScreenSizeMin", m_blendScreenSizeMin, "Agent will be drawn as a sprite if screen-size is less than this value.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blendScreenSizeMax", m_blendScreenSizeMax, "Agent will be drawn as a mesh if screeen-size is greater than this value.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphereRadius, "The radius of the bounding sphere, applied to each agent.", Be::READWRITE | Be::PERSIST )
		
		MAP_METHOD_AND_WRAP( "AddAgent", AddAgent, "Adds a drone to the swarm \n:jessica-placement: TOOLBAR\n:jessica-icon: far-drone-alt\n" )
		MAP_METHOD_AND_WRAP( "RemoveAgent", RemoveAgent, "removes a random drone from the swarm \n:jessica-placement: TOOLBAR\n:jessica-icon: far-dumpster\n" )
		MAP_METHOD_AND_WRAP( "SetCount", SetCount, "Specify a desired number of agents for the system \n:param count: number of agents\n:jessica-placement: TOOLBAR\n:jessica-icon: far-ball-pile\n" )
		MAP_METHOD_AND_WRAP( "CreateAgentTree", CreateAgentTree, "a temp DEV toggle \n:jessica-placement: TOOLBAR\n:jessica-icon: fab-dev\n" )
		
		MAP_ATTRIBUTE( "debugMode", m_debugMode, "Toggle for debugging intensity/lod \njessica-group: Debug", Be::READWRITE )
		MAP_ATTRIBUTE( "debugLodLevel", m_debugLodLevel, "LOD level override (0 - 1) \njessica-group: Debug", Be::READWRITE )
		MAP_ATTRIBUTE( "debugIntensity", m_debugIntensity, "Intensity override (0 - 1) \njessica-group: Debug", Be::READWRITE )
		
	EXPOSURE_END()
}