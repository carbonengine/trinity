// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2GrannyPrimitiveSet.h"
#include "TriConstants.h"

BLUE_DEFINE( Tr2GrannyPrimitiveSet );

const Be::ClassInfo* Tr2GrannyPrimitiveSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GrannyPrimitiveSet, "" )
		MAP_INTERFACE( Tr2GrannyPrimitiveSet )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE_WITH_CHOOSER( "grannyResPath", m_grannyResPath, "A res path to the granny resource", Be::READWRITE | Be::PERSIST | Be::NOTIFY, TriGR2Chooser )
		MAP_ATTRIBUTE( "grannyRes", m_grannyRes, "The granny resource holding the curves", Be::READ )
		MAP_ATTRIBUTE( "renderSolid", m_renderSolid, "Should we render the triangles? If not, we will render as lines.", Be::READWRITE | Be::PERSIST )
	EXPOSURE_CHAINTO( Tr2PrimitiveSet )
}
