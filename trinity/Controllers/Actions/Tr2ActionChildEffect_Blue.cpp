// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionChildEffect.h"


BLUE_DEFINE( Tr2ActionChildEffect );

const Be::ClassInfo* Tr2ActionChildEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionChildEffect, "" )
		MAP_INTERFACE( Tr2ActionChildEffect )
		MAP_INTERFACE( ITr2ControllerAction )

		MAP_ATTRIBUTE(
			"path",
			m_path,
			"Overlay .red file path\n"
			":jessica-widget: filepath\n"
			":jessica-file-filter: redfile",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "childName", m_childName, "Name to assign to the loaded child", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "targetAnotherOwner", m_targetAnotherOwner, "Designate a child or a MultiEffect parameter to target for the load (defaults to controller's owner)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "addOnStart", m_addOnStart, "Add the effect on action start", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "removeOnStop", m_removeOnStop, "Remove the effect on action stop", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
