// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "Tr2DataTextureManager.h"

BLUE_DEFINE( Tr2DataTextureManager );

const Be::ClassInfo* Tr2DataTextureManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2DataTextureManager, "" )
		MAP_INTERFACE( Tr2DataTextureManager )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "textureWidth", m_textureWidth, "", Be::READ )
		MAP_ATTRIBUTE( "textureHeight", m_textureHeight, "", Be::READ )
		MAP_ATTRIBUTE( "blockDataNextIdx", m_blockDataNextIdx, "", Be::READ )
		MAP_ATTRIBUTE( "maxBlockCount", m_maxBlockCount, "", Be::READ )
		MAP_ATTRIBUTE( "maxPixelCount", m_maxPixelCount, "", Be::READ )

	EXPOSURE_END()
}