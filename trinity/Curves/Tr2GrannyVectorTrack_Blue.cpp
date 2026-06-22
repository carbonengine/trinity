// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2GrannyVectorTrack.h"

BLUE_DEFINE( Tr2GrannyVectorTrack );

const Be::ClassInfo* Tr2GrannyVectorTrack::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GrannyVectorTrack, ":jessica-deprecated: True\n:jessica-icon: tree/triscalarcurve.png" )
		MAP_INTERFACE( Tr2GrannyVectorTrack )
		MAP_ATTRIBUTE( "value", m_value, "The value of the track", Be::READ )
	EXPOSURE_CHAINTO( Tr2GrannyTrack )
}
