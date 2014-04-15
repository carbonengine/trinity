////////////////////////////////////////////////////////////
//
//    Created:   February 2014
//    Copyright: CCP 2014
//
#include "StdAfx.h"
#include "EveDynamicPlaneSet.h"

BLUE_DEFINE( EveDynamicPlaneSet );

const Be::ClassInfo* EveDynamicPlaneSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveDynamicPlaneSet, "" )
        MAP_INTERFACE( EveDynamicPlaneSet )

		MAP_ATTRIBUTE( "contentID", m_contentID, "A reference string to the content creation", Be::READWRITE | Be::PERSIST )

    EXPOSURE_CHAINTO( EvePlaneSet )}
