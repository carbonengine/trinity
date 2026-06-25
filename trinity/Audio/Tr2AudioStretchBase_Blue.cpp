// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "Tr2AudioStretchBase.h"

BLUE_DEFINE( Tr2AudioStretchBase );
BLUE_DEFINE_INTERFACE( ITr2Audio );
BLUE_DEFINE_INTERFACE( ITr2AudEmitter );

const Be::ClassInfo* Tr2AudioStretchBase::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2AudioStretchBase, ":jessica-help-url: https://wiki.ccpgames.com/display/CAudio/Tr2AudioStretchBase" )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2DebugRenderable )
		MAP_INTERFACE( ITr2Audio )

		MAP_ATTRIBUTE(
			"sourceEmitter",
			m_sourceEmitter,
			"Source audio emitter, Follows the source position. The outburst event will come from this emitter.",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"destinationEmitter",
			m_destEmitter,
			"Destination audio emitter, follows the destination position. The impact event will come from this emitter.",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"stretchEmitter",
			m_stretchEmitter,
			"Audio emitter for the looping part of the stretch effect. It follows the camera as long as it is between the "
			"source and destination. The stretch event will come from this emitter.",
			Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
