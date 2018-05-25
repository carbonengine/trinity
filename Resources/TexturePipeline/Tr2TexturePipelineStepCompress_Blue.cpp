////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2TexturePipelineStepCompress.h"


BLUE_DEFINE( Tr2TexturePipelineStepCompress );

const Be::ClassInfo* Tr2TexturePipelineStepCompress::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TexturePipelineStepCompress, "" )
		MAP_INTERFACE( Tr2TexturePipelineStepCompress )
		MAP_INTERFACE( ITr2TexturePipelineStep )

		MAP_ATTRIBUTE_WITH_CHOOSER( "format", m_format, "Output compressed format", Be::READWRITE | Be::PERSIST | Be::ENUM, Tr2RenderContextEnum_PixelFormat_Chooser )
		MAP_ATTRIBUTE( "r", m_rWeight, "Red channel weight", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "g", m_gWeight, "Green channel weight", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "b", m_bWeight, "Blue channel weight", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
