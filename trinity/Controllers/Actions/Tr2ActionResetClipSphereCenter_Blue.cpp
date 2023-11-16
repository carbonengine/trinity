#include "StdAfx.h"
#include "Tr2ActionResetClipSphereCenter.h"


BLUE_DEFINE( Tr2ActionResetClipSphereCenter );
namespace
{
	Be::VarChooser ResetBehaviorChooser[] =
	{
		{ "Object Center", BeCast( Tr2ActionResetClipSphereCenter::OBJECT_CENTER ), "Sets the clipsphere to the center of the object" },
		{ "Last Hit Damage Locator", BeCast( Tr2ActionResetClipSphereCenter::LAST_DAMAGELOCATOR_HIT ), "Sets the clipsphere to the last damage locator hit. If there has no damage locator been hit, then a random one is selected" },
		{ "Custom", BeCast( Tr2ActionResetClipSphereCenter::CUSTOM ), "Sets the clipsphere to the locator defined in 'locatorSetName' and 'locatorIndex'" },
		{ 0 }
	};
}

const Be::ClassInfo* Tr2ActionResetClipSphereCenter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionResetClipSphereCenter, "" )
		MAP_INTERFACE( Tr2ActionResetClipSphereCenter )
		MAP_INTERFACE( ITr2ControllerAction )
		MAP_ATTRIBUTE( "locatorSetName", m_locatorSetName, "Locator set name", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "locatorIndex", m_locatorIndex, "Locator Index", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE_WITH_CHOOSER( "resetBehavior", m_resetBehavior, "", Be::READWRITE | Be::PERSIST | Be::ENUM, ResetBehaviorChooser )
	EXPOSURE_END()
}
