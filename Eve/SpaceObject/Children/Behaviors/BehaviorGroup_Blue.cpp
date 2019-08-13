
#include "StdAfx.h"
#include "BehaviorGroup.h"

BLUE_DEFINE( BehaviorGroup );

const Be::ClassInfo* BehaviorGroup::ExposeToBlue()
{
	EXPOSURE_BEGIN( BehaviorGroup, "" )
		MAP_INTERFACE( BehaviorGroup )
		
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "count", m_count, "Number of ships", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "maxVelocity", m_maxVelocity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "behaviors", m_behaviors, "What the ships do etc", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "volumes", m_volumes, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "exclusionVolumes", m_exclusionVolumes, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "locatorSet", m_locatorSets, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "mesh", m_mesh, "the drone mesh", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "spriteMesh", m_spriteMesh, "this will replace the mesh when the camera is far away", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scale", m_scale, "Agent local scaling", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spriteScale", m_spriteScale, "Additional scaling when swapping to the sprite (LOD)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "currentScreenSize", m_currentScreenSize, "Current screen size for first agent in the group. Enable bounding-sphere primitive visualization for debugging.", Be::READ )
		MAP_ATTRIBUTE( "renderThreshold", m_renderThreshold, "If the screen-size of all agents is below this threshold the group will not be rendered. ", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blendScreenSizeMin", m_blendScreenSizeMin, "Agent will be drawn as a sprite if screen-size is less than this value.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blendScreenSizeMax", m_blendScreenSizeMax, "Agent will be drawn as a mesh if screeen-size is greater than this value.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boundingSphereCenter", m_boundingSphereCenter, "The center of the bounding sphere relative to each agent. Can be used as an offset.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphereRadius, "The radius of the bounding sphere, applied to each agent.", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "AddAgent", AddAgent, "Adds a drone to the swarm \n:jessica-placement: TOOLBAR\n:jessica-icon: far-drone-alt\n" )
		MAP_METHOD_AND_WRAP( "RemoveAgent", RemoveAgent, "removes a random drone from the swarm \n:jessica-placement: TOOLBAR\n:jessica-icon: far-dumpster\n" )
		MAP_METHOD_AND_WRAP( "SetCount", SetCount, "Specify a desired number of agents for the system \n:param count: number of agents\n:jessica-placement: TOOLBAR\n:jessica-icon: far-ball-pile\n" )
		MAP_METHOD_AND_WRAP( "ToggleMesh", ToggleMesh, "a temp DEV toggle \n:jessica-placement: TOOLBAR\n:jessica-icon: fab-dev\n" )
		MAP_METHOD_AND_WRAP( "CreateAgentTree", CreateAgentTree, "a temp DEV toggle \n:jessica-placement: TOOLBAR\n:jessica-icon: fab-dev\n" )


	EXPOSURE_END()
}