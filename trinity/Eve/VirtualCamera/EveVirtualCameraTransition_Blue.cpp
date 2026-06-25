// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveVirtualCameraTransition.h"

BLUE_DEFINE_ABSTRACT( EveVirtualCameraTransitionBase );

const Be::ClassInfo* EveVirtualCameraTransitionBase::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraTransitionBase, "" )
		EXPOSURE_END()
}

BLUE_DEFINE( EveVirtualCameraTransitionCut );

const Be::ClassInfo* EveVirtualCameraTransitionCut::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraTransitionCut, "A transition that cuts quickly to a target camera." )
		EXPOSURE_CHAINTO( EveVirtualCameraTransitionBase )
}

BLUE_DEFINE( EveVirtualCameraTransitionLerp );

const Be::ClassInfo* EveVirtualCameraTransitionLerp::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveVirtualCameraTransitionLerp, "A transition that lerps from a source to a target camera." )
		MAP_ATTRIBUTE( "tansitionTime", m_transitionTime, "The time to take to transition in seconds.", Be::READWRITE | Be::PERSIST )
	EXPOSURE_CHAINTO( EveVirtualCameraTransitionBase )
}
