// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "EveCustomMask.h"

BLUE_DEFINE( EveCustomMask );
const Be::ClassInfo* EveCustomMask::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveCustomMask, "" )
		MAP_INTERFACE( EveCustomMask )

		MAP_ATTRIBUTE( "position", m_position, "data\n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "data\n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "data\n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "materialIndex", m_materialIndex, "data\n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "targetMaterials", m_targetMaterials, "Pattern goes onto material x,y,z,w\n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isMirrored", m_isMirrored, "data\n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "clampU", m_clampU, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "clampV", m_clampV, "", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
