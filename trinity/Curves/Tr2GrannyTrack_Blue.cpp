#include "StdAfx.h"
#include "Tr2GrannyTrack.h"
#include "TriConstants.h"

BLUE_DEFINE_ABSTRACT( Tr2GrannyTrack );

const Be::ClassInfo* Tr2GrannyTrack::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2GrannyTrack, "" )
        MAP_INTERFACE( Tr2GrannyTrack )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_ATTRIBUTE_WITH_CHOOSER( "grannyResPath", m_grannyResPath, "A res path to the granny resource", Be::READWRITE | Be::PERSIST | Be::NOTIFY, TriGR2Chooser )
		MAP_ATTRIBUTE( "grannyRes", m_grannyRes, "The granny resource holding the curves", Be::READ )	
		MAP_ATTRIBUTE( "name", m_name, "The name of the transform track", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "group", m_group, "The name of the track group", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "duration", m_duration, "The duration of the animation", Be::READ )
		MAP_ATTRIBUTE( "cycle", m_cycle, "Should we cycle the animation", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}

