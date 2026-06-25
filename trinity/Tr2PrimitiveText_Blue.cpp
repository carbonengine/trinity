// Copyright © 2016 CCP ehf.

#include "StdAfx.h"
#include "Tr2PrimitiveText.h"


BLUE_DEFINE( Tr2PrimitiveText );

Be::VarChooser TriDebugFontChooser[] = {
	{ "FontSmall", BeCast( TRI_DBG_FONT_SMALL ), "Small debug font" },
	{ "FontMedium", BeCast( TRI_DBG_FONT_MEDIUM ), "Medium debug font" },
	{ "FontLarge", BeCast( TRI_DBG_FONT_LARGE ), "Larget debug font" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "TriDebugFont", TriDebugFont, TriDebugFontChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Tr2PrimitiveText::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PrimitiveText, "" )
		MAP_INTERFACE( Tr2PrimitiveText )

		MAP_ATTRIBUTE( "text", m_text, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "font", m_font, "", Be::READWRITE | Be::PERSIST | Be::ENUM, TriDebugFontChooser )

	EXPOSURE_END()
}