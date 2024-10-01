////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PostProcessRenderInfo.h"
#include "Tr2RenderTarget.h"

BLUE_DEFINE( Tr2PostProcessRenderInfo );

const Be::ClassInfo* Tr2PostProcessRenderInfo::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PostProcessRenderInfo, "" )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "sourceBuffer", m_sourceBuffer, "The source buffer of the post process", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "debugTextures", m_debugTextures, "Prevent sharing of textures", Be::READWRITE | Be::NOTIFY )
		MAP_METHOD_AND_WRAP( "GetAllTempTextures", GetAllTempTextures, "Returns all temp textures used by post-process. For debugging purposes." )
	EXPOSURE_END()
}

