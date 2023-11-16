////////////////////////////////////////////////////////////
//
//    Created:   August 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"
#include "TriStepSetVisualizationMode.h"
#include "ITr2VisualizationModeRenderer.h"

BLUE_DEFINE( TriStepSetVisualizationMode );

const Be::ClassInfo* TriStepSetVisualizationMode::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepSetVisualizationMode, "" )
		MAP_INTERFACE( TriStepSetVisualizationMode )

		MAP_ATTRIBUTE( "object", m_object, "The object to set visualization mode for", Be::READWRITE)
		MAP_ATTRIBUTE( "mode", m_mode, "Visualization mode for the object", Be::READWRITE)
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS( 
			"__init__", 
			py__init__, 
			2, 
			":param obj: The object to set visualization mode for\n"
			":param mode: Visualization mode for the object"
			)

	EXPOSURE_CHAINTO( TriRenderStep )
}