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
			":param transform: transform matrix"
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
		MAP_ATTRIBUTE( "primitives", m_primitives, 
			"List of primitives this tool will render", 
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
			"Set a particular primitive or axis as selected\n" 
			":param name: axis name"
		)
		MAP_METHOD_AND_WRAP( "Move", PyMove, 
			"( x, y, dx, dy, viewport, viewmatrix, projectionmatrix )\n"
			"Move the manipulator based on mouse and mouse delta values and matrices\n" 
			":param x: \n"
			":param y: \n"
			":param dx: \n"
			":param dy: \n"
			":param viewport: \n"
			":param view: \n"
			":param projection: \n"
		)
		MAP_METHOD_AND_WRAP( "SetMoveCallback", SetMoveCallback, 
			"( callable )\n"
			"Add a python callable that accepts two arguments.\n" 
			":param cb: callback function"
		)
    EXPOSURE_END()
}
