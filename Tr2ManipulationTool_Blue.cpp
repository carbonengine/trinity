#include "StdAfx.h"
#include "Tr2ManipulationTool.h"
#include "TriViewport.h"

BLUE_DEFINE_ABSTRACT( Tr2ManipulationTool );

const Be::ClassInfo* Tr2ManipulationTool::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2ManipulationTool, "" )
        MAP_INTERFACE( Tr2ManipulationTool )		
		MAP_METHOD_AND_WRAP( "Init", Init, 
			"( transform )\n"
			"Initilize the tool to the defaults and apply the base transform to work with.\n"
			"Also the starting mouse position which is needed for the trackball rotation.\n"
		)
		MAP_ATTRIBUTE( "captured", m_captured, 
			"Is the tool captured. Being used", 
			Be::READWRITE
		)
		MAP_ATTRIBUTE( "pivot", m_pivot, 
			"The manipulation tools pivot", 
			Be::READWRITE
		)
		MAP_ATTRIBUTE( "localTransform", m_localTransform, 
			"The manipulation tools local transform", 
			Be::READWRITE| Be::PERSIST 
		)
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, 
			"The manipulation tools world transform", 
			Be::READ 
		)
#if BLUE_WITH_PYTHON
		MAP_ATTRIBUTE( "_userData", m_pythonUserData,
			"Pure python user data attached to the primitive set",
			Be::READWRITE
		)
#endif
		MAP_METHOD_AND_WRAP( "SelectAxis", SelectAxis, 
			"( name )\n"
			"Set a particular primitive or axis as selected" 
		)
		MAP_METHOD_AND_WRAP( "Move", PyMove, 
			"( x, y, dx, dy, viewport, viewmatrix, projectionmatrix )\n"
			"Move the manipulator based on mouse and mouse delta values and matrices" 
		)
		MAP_METHOD_AND_WRAP( "SetMoveCallback", SetMoveCallback, 
			"( callable )\n"
			"Add a python callable that accepts two arguments." 
		)
    EXPOSURE_END()
}
