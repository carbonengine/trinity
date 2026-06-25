// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "Tr2ScalarFader.h"

BLUE_DEFINE( Tr2ScalarFader );

const Be::ClassInfo* Tr2ScalarFader::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ScalarFader, "" )
		MAP_INTERFACE( Tr2ScalarFader )

		MAP_ATTRIBUTE( "value", m_value, "The actual value of this fader", Be::READWRITE )
		MAP_ATTRIBUTE( "fading", m_fading, "Holds the current fading speed", Be::READWRITE )
		MAP_ATTRIBUTE( "fadeTime", m_fadeTime, "Internal timer of this fader", Be::READ )

	EXPOSURE_END()
}