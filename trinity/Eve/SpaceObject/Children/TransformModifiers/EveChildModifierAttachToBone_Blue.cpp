// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierAttachToBone.h"


BLUE_DEFINE( EveChildModifierAttachToBone );


const Be::ClassInfo* EveChildModifierAttachToBone::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildModifierAttachToBone, "" )
		MAP_INTERFACE( EveChildModifierAttachToBone )
		MAP_INTERFACE( IEveChildTransformModifier )

		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, "the bone index this modifier is tight to\n:jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}