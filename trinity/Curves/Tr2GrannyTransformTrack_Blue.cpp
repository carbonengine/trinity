#include "StdAfx.h"
#include "Tr2GrannyTransformTrack.h"

BLUE_DEFINE( Tr2GrannyTransformTrack );

const Be::ClassInfo* Tr2GrannyTransformTrack::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GrannyTransformTrack, ":jessica-deprecated: True\n:jessica-icon: tree/trisplttransform.png" )
		MAP_INTERFACE( Tr2GrannyTransformTrack )
		MAP_ATTRIBUTE( "translation", m_translation, "The position of the track", Be::READ )
		MAP_ATTRIBUTE( "rotation", m_rotation, "The orientation of the track", Be::READ )
		MAP_ATTRIBUTE( "scale", m_scale, "The scale of the track", Be::READ )
		MAP_ATTRIBUTE
		( 
			"compressCurves", 
			m_compressCurves, 
			"If set, the curves are compressed (fitted to splines). This is more\n"
			"efficient to work with but can introduce artifacts.",
			Be::READWRITE
		)
	EXPOSURE_CHAINTO( Tr2GrannyTrack )
}
