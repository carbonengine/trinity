#include "StdAfx.h"
#include "Tr2SyncToAnimation.h"


BLUE_DEFINE( Tr2SyncToAnimation );


const Be::ClassInfo* Tr2SyncToAnimation::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SyncToAnimation, "" )
		MAP_INTERFACE( Tr2SyncToAnimation )
		MAP_INTERFACE( ITr2StateMachineStateFinalizer )

		MAP_ATTRIBUTE( "mask", m_mask, "Animation mask/layer name", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
