#include "StdAfx.h"
#include "Tr2TextureAtlasMan.h"

BLUE_DEFINE( Tr2TextureAtlasMan );

const Be::ClassInfo* Tr2TextureAtlasMan::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2TextureAtlasMan, "" )
        MAP_INTERFACE( Tr2TextureAtlasMan )

		MAP_ATTRIBUTE
		(
			"atlases",
			m_atlases,
			"List of atlases provided, each with a different texture format",
			Be::READ
		)

		MAP_METHOD_AND_WRAP
		(
			"AddAtlas",
			AddAtlasWrap,
			"Add a texture atlas with the given format and dimensions.\n\n"
			"Arguments:\n"
			":param fmt: The texture format (trinity.PIXEL_FORMAT)\n"
			":param width: The width of the texture atlas\n"
			":param height: The height of the texture atlas"
		)

		MAP_METHOD_AND_WRAP
		(
			"RemoveAtlas",
			RemoveAtlasWrap,
			"Removes a texture atlas with the given format.\n\n"
			":param fmt: The texture format"
		)

    EXPOSURE_END()
}
