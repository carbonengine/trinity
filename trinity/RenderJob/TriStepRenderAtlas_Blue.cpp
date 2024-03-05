#include "StdAfx.h"

#include "TriStepRenderAtlas.h"
#include "Tr2TextureAtlas.h"
#include "Tr2AtlasTexture.h"

BLUE_DEFINE( TriStepRenderAtlas );

const Be::ClassInfo* TriStepRenderAtlas::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepRenderAtlas, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRenderAtlas )

		MAP_ATTRIBUTE( "atlas", m_atlas, "Atlas that will be rendered", Be::READWRITE )
		MAP_ATTRIBUTE( "focus", m_focus, "", Be::READWRITE )
		MAP_ATTRIBUTE( "tlTexCoord", m_tlTexCoord, "top left texture coordinate", Be::READWRITE )
		MAP_ATTRIBUTE( "brTexCoord", m_brTexCoord, "bottom right texture coordinate", Be::READWRITE )
		MAP_ATTRIBUTE( "showFree", m_showFree, "show atlas areas currently free", Be::READWRITE )
		MAP_ATTRIBUTE( "showUsed", m_showUsed, "show atlas areas currently in use", Be::READWRITE )
		MAP_ATTRIBUTE( "borderColour", m_borderColour, "border colour for areas in use", Be::READWRITE )
		MAP_ATTRIBUTE( "focusColour", m_focusColour, "border colour for areas in use and selected (NYI)", Be::READWRITE )
		MAP_ATTRIBUTE( "freeColour", m_freeColour, "border colour for free areas", Be::READWRITE )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__", 
			py__init__, 
			2,
			"Creates a render step that renders an atlas\n" 
			":param atlas: an atlas (default None)\n"
			":param texture: texture in the atlas\n"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}

