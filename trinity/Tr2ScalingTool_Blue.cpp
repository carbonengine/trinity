#include "StdAfx.h"

#include "Tr2ScalingTool.h"

BLUE_DEFINE( Tr2ScalingTool );

const Be::ClassInfo* Tr2ScalingTool::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ScalingTool, "" )
		MAP_INTERFACE( Tr2ScalingTool )
		MAP_INTERFACE( Tr2ManipulationTool )

		MAP_METHOD_AND_WRAP( "ResetPrimitives", ResetPrimitives, 
		"Resets the primitives of the Scaling tool")

		MAP_ATTRIBUTE( "scale", m_scale, 
		"The output scale value", 
		Be::READ
		)

		EXPOSURE_CHAINTO( Tr2ManipulationTool )
}
