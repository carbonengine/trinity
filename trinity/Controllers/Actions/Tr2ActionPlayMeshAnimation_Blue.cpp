////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ActionPlayMeshAnimation.h"


BLUE_DEFINE( Tr2ActionPlayMeshAnimation );

namespace
{
	Be::VarChooser PlayActionChooser[] =
	{
		{ "Play", BeCast( Tr2ActionPlayMeshAnimation::PLAY ), "Play animation immediately" },
		{ "Enqueue Play", BeCast( Tr2ActionPlayMeshAnimation::ENQUEUE_PLAY ), "Play animation after all other animations complete" },
		{ 0 }
	};
	Be::VarChooser StopActionChooser[] =
	{
		{ "Stop", BeCast( Tr2ActionPlayMeshAnimation::STOP ), "Stop animation immediately" },
		{ "Enqueue Stop", BeCast( Tr2ActionPlayMeshAnimation::ENQUEUE_STOP ), "Stop animation after loop finishes" },
		{ "None", BeCast( Tr2ActionPlayMeshAnimation::NONE ), "Do not stop animation" },
		{ 0 }
	};
	Be::VarChooser DestinationTypeChooser[] = {
		{ "Owner", BeCast( Tr2ActionPlayMeshAnimation::DestinationType::OWNER ), "Action affects the owner" },
		{ "Child", BeCast( Tr2ActionPlayMeshAnimation::DestinationType::CHILD ), "Action affect a child object" },
		{ 0 }
	};

	}

const Be::ClassInfo* Tr2ActionPlayMeshAnimation::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionPlayMeshAnimation, "" )
		MAP_INTERFACE( Tr2ActionPlayMeshAnimation )
		MAP_INTERFACE( ITr2ControllerAction )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "animation", m_animation, "Animation name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "mask", m_mask, "Animation mask/layer name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "playAction", m_playAction, "", Be::READWRITE | Be::PERSIST | Be::ENUM, PlayActionChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "stopAction", m_stopAction, "", Be::READWRITE | Be::PERSIST | Be::ENUM, StopActionChooser )
		MAP_ATTRIBUTE( "loops", m_loops, "Number of loops animation is supposed to play (<=0 for infinite)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "delay", m_delay, "Delay animation start in seconds", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "speed", m_speed, "Animation speed factor", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"destinationType",
			m_destinationType,
			"Is the action affecting the owner of a child object\n"
			":jessica-group: Destination",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST | Be::ENUM,
			DestinationTypeChooser )
		MAP_ATTRIBUTE(
			"path",
			m_destination.m_path,
			"Path to the destination child mesh for shared controllers.\n"
			":jessica-group: Destination",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"destination",
			m_destination.m_object,
			"Destination child mesh.\n"
			":jessica-group: Destination",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"delayBinding",
			m_delayBinding,
			"If binding of the destination path needs to be delayed until the action is executed, rather than \n"
			"binding when the parent controller is linked with the owner. This attribute needs to be turned off most\n"
			"of the time for performance. Valid usages of this are for changing dynamically created\n"
			"subobjects of the parent.\n"
			":jessica-group: Destination",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_PROPERTY_READONLY(
			"isBindingValid",
			IsBindingValid,
			"Is destination binding valid\n"
			":jessica-group: Destination" )

		MAP_METHOD_AND_WRAP( "GetDestination", GetDestination, "Returns destination object" )
	EXPOSURE_END()
}
