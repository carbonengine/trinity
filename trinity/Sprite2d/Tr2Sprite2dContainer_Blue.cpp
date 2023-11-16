#include "StdAfx.h"
#include "Tr2Sprite2dContainer.h"

BLUE_DEFINE_ABSTRACT( Tr2Sprite2dContainerBase );
BLUE_DEFINE( Tr2Sprite2dContainer );

const Be::ClassInfo* Tr2Sprite2dContainerBase::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2Sprite2dContainerBase, "" )
		MAP_INTERFACE( ITr2SpriteObject )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE
		(
			"children",
			m_children,
			"Children of this container",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"background",
			m_background,
			"Background objects of this container. Background objects don't get alignment,\n"
			"but always fill the extent of the container. They are never considered for\n"
			"picking, either.",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"opacity",
			m_opacity,
			"Opacity of this container (and it's children)",
			Be::READWRITE | Be::NOTIFY
		)

	EXPOSURE_CHAINTO( Tr2SpriteObjectBase )
}

const Be::ClassInfo* Tr2Sprite2dContainer::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2Sprite2dContainer, "" )

		MAP_ATTRIBUTE
		(
			"depthMin", 
			m_depthMin, 
			"Minimum depth for children of this container. This is relative to parent"
			"\ndepth, a value from -1 to 1 (inclusive).",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"depthMax", 
			m_depthMax, 
			"Maximum depth for children of this container. This is relative to parent"
			"\ndepth, a value from -1 to 1 (inclusive).",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"absoluteCoordinates",
			m_absoluteCoordinates,
			"Used for absolute alignment - resets the transforms applied by parent to identity.",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"clip",
			m_clip,
			"If set, the children are clipped to fit within the clip rectangle of the container",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"pickRadius",
			m_pickRadius,
			"Radius to use when picking. If radius is 0 the container is treated as"
			"\na rectangular area.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"cacheContents",
			m_cacheContentsHint,
			"Should contents of this container be cached? This can drastically improve\n"
			"render performance of containers with static contents. This flag is only\n"
			"a hint, the container uses caching when it doesn't change too often.",
			Be::READWRITE | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"isCachingContents",
			m_cacheContents,
			"Is the contents of this container currently cached. Use cacheContents to\n"
			"turn caching on/off.",
			Be::READ
		)

	EXPOSURE_CHAINTO( Tr2Sprite2dContainerBase )
}
