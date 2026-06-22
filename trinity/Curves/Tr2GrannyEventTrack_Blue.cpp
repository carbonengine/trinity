// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "Tr2GrannyEventTrack.h"

BLUE_DEFINE( Tr2GrannyEventTrack );

const Be::ClassInfo* Tr2GrannyEventTrack::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GrannyEventTrack, ":jessica-deprecated: True\n:jessica-icon: tree/trirotationcurve.png" )
		MAP_INTERFACE( Tr2GrannyEventTrack )
		MAP_ATTRIBUTE( "eventListener", m_eventListener, "Event listener for the track (blue.IBlueEventListener type)", Be::READWRITE )
	EXPOSURE_CHAINTO( Tr2GrannyTrack )
}
