#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorPhysicalPortal.h"

#include "Tr2InteriorCell.h"
#include "Tr2InteriorPortalSocket.h"

BLUE_DEFINE( Tr2InteriorPhysicalPortal );

// ------------------------------------------------------------------------------------------------------
const Be::ClassInfo* Tr2InteriorPhysicalPortal::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2InteriorPhysicalPortal, "" )
		MAP_INTERFACE( Tr2InteriorPhysicalPortal )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "enabled", m_enabled, "Switch to enable/disable a portal", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "position", m_position, "Vector specifying the position of the portal relative to its cell", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rotation", m_rotation, "Quaternion specifying the rotation of the portal relative to its cell", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "Matrix specifying the position and rotation of the portal in world coordinates", Be::READ )

		MAP_ATTRIBUTE( "minBounds", m_minBounds, "Minimum bounds of the bounding box defining the portal", Be::READWRITE | Be::PERSIST | Be::NOTIFY );
		MAP_ATTRIBUTE( "maxBounds", m_maxBounds, "Maximum bounds of the bounding box defining the portal", Be::READWRITE | Be::PERSIST | Be::NOTIFY );
		MAP_ATTRIBUTE( "cellA", m_cellA, "The first cell this portal connects", Be::READ | Be::PERSIST );
		MAP_ATTRIBUTE( "cellB", m_cellB, "The second cell this portal connects", Be::READ | Be::PERSIST );
		MAP_ATTRIBUTE( "socketA", m_socketA, "Portal socket for the first cell", Be::READ | Be::PERSIST );
		MAP_ATTRIBUTE( "socketB", m_socketB, "Portal socket for the second cell", Be::READ | Be::PERSIST );
		MAP_ATTRIBUTE( "name", m_name, "The name of this portal", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "doorObject", m_doorObject, "A door for this portal: the portal will make sure the door is visible from both cells", Be::READWRITE | Be::PERSIST | Be::NOTIFY );

		MAP_METHOD_AND_WRAP( "ConnectCells", ConnectCells, "Connect two cells together" );
		MAP_METHOD_AND_WRAP( "ConnectSockets", ConnectSockets, "Connect two portal sockets together" );
		MAP_METHOD_AND_WRAP( "CreateSockets", CreateSockets, "Create portal sockets for both cells" );
		MAP_METHOD_AND_WRAP( "PositionFromSockets", PositionFromSockets, "Place portal so that encloses both connected cells" );

	EXPOSURE_END()
}

#endif
