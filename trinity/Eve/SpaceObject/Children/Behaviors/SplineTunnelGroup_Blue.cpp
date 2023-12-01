#include "StdAfx.h"
#include "SplineTunnelGroup.h"
#include "Eve/SpaceObject/Children/EveChildContainer.h"


Be::VarChooser SplineTunnelGroupTypeChooser[] =
{
	// If you modify this make sure to go through the behaviors and add the same modification
	{ "Exit_Tunnels", BeCast( SplineTunnelGroup::EXIT_TUNNELS ), "Tunnels Drones flock to when set to exit the scene" },
	{ "Entrance_Tunnels", BeCast( SplineTunnelGroup::ENTRANCE_TUNNELS ), "Tunnels Drones flock to when entering the scene" },
	{ "Other_Tunnels", BeCast( SplineTunnelGroup::OTHER_TUNNELS ), "pathways in the scene (hallways etc)" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "setTunnelType", SplineTunnelGroup::TunnelGroupType, SplineTunnelGroupTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );


BLUE_DEFINE( SplineTunnelGroup );

const Be::ClassInfo* SplineTunnelGroup::ExposeToBlue()
{
	EXPOSURE_BEGIN( SplineTunnelGroup, "" )
		MAP_INTERFACE( SplineTunnelGroup )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "", Be::READ | Be::PERSIST | Be::NOTIFY )
		
		MAP_ATTRIBUTE_WITH_CHOOSER("tunnelGroupType", m_tunnelGroupType, "control when drones interact with these tunnels",
		                           Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, SplineTunnelGroupTypeChooser)
		MAP_ATTRIBUTE( "breakPoints", m_numBreakPoints, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "tunnelWidth", m_tunnelWidth, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "entrancePullSize", m_entrancePullSize, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "entrySize", m_entrySize, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_METHOD_AND_WRAP("createSplineTunnels", CreateSplineTunnels,
		                    "recreate all tunnels \n:jessica-placement: TOOLBAR\n:jessica-icon: far-bomb\n")

	EXPOSURE_END()
}