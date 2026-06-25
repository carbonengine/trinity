// Copyright © 2016 CCP ehf.

#include "StdAfx.h"
#include "Tr2ImpostorManager.h"

BLUE_DEFINE_INTERFACE( ITr2ImpostorSource );
BLUE_DEFINE( Tr2ImpostorManager );

const Be::ClassInfo* Tr2ImpostorManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ImpostorManager, "Manages object impostor billboards" )
		MAP_INTERFACE( Tr2ImpostorManager )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "width", m_width, "Atlas width in pixels", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "height", m_height, "Atlas height in pixels", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "itemWidth", m_itemWidth, "Billboard width in pixels", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "itemHeight", m_itemHeight, "Billboard height in pixels", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "maxUpdates", m_maxUpdates, "Maximum number of impostor updates per frame", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "atlas", m_rt, "Billboard atlas", Be::READ );
		MAP_ATTRIBUTE( "effect", m_effect, "Effect used to render impostors", Be::READ );
		MAP_PROPERTY_READONLY( "count", GetImpostorCount, "Current number of impostors" );

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			Create,
			4,
			"Creates the manager.\n"
			":param width: optional atlas width\n"
			":param height: optional atlas height\n"
			":param itemWidth: optional billboard width\n"
			":param itemHeight: optional billboard width" );
	EXPOSURE_END()
}
