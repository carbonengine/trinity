// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2TranslationTool.h"

BLUE_DEFINE( Tr2TranslationTool );


const Be::ClassInfo* Tr2TranslationTool::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TranslationTool, "" )
		MAP_INTERFACE( Tr2TranslationTool )
		MAP_INTERFACE( Tr2ManipulationTool )
		MAP_ATTRIBUTE( "translation", m_translation, "The output translation value", Be::READ )
	EXPOSURE_CHAINTO( Tr2ManipulationTool )
}
