// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2RotationTool.h"

BLUE_DEFINE( Tr2RotationTool );

const Be::ClassInfo* Tr2RotationTool::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2RotationTool, "" )
		MAP_INTERFACE( Tr2RotationTool )
		MAP_INTERFACE( Tr2ManipulationTool )
		MAP_ATTRIBUTE( "rotation", m_rotation, "The output rotation value", Be::READ )
		MAP_ATTRIBUTE( "precision", m_precision, "Rotation precision", Be::READWRITE )
	EXPOSURE_CHAINTO( Tr2ManipulationTool )
}
