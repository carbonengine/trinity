#include "StdAfx.h"
#include "FollowASpline.h"

BLUE_DEFINE_INTERFACE( IBehavior );

Be::VarChooser FollowASplineTunnelGroupTypeChooser[] =
{
	{ "Exit_Tunnels", BeCast( SplineTunnelGroup::EXIT_TUNNELS ), "Tunnels Drones flock to when set to exit the scene" },
	{ "Entrance_Tunnels", BeCast( SplineTunnelGroup::ENTRANCE_TUNNELS ), "Tunnels Drones flock to when entering the scene" },
	{ "Other_Tunnels", BeCast( SplineTunnelGroup::OTHER_TUNNELS ), "pathways in the scene (hallways etc)" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "setTunnelType", SplineTunnelGroup::TunnelGroupType, FollowASplineTunnelGroupTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );


BLUE_DEFINE( FollowASpline );

const Be::ClassInfo* FollowASpline::ExposeToBlue()
{
	EXPOSURE_BEGIN( FollowASpline, "" )
		MAP_INTERFACE( FollowASpline )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE_WITH_CHOOSER( "tunnelGroupType", m_tunnelGroupType, "designate what TunnelGroup this behavior Should Look for",
			Be::READWRITE | Be::PERSIST | Be::ENUM, FollowASplineTunnelGroupTypeChooser )

		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, "here you configure how hard it is for other behaviors to interact with this one", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "smoothPullFactor", m_smoothPullFactor, "0: pull-to-start point is the same everywhere, 1: pulling force grows the closer you get to the entrance, [0-1] a blend (this var is heavily affected by behavior weight) ", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "cornerSmoothener", m_cornerSmoothener, "The lower this one is the more it factors the next splinePoint, looks more natural when lower but is more reliable for complex tunnels (and sharp turns)", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}