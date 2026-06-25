// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "EveChildLineSet.h"
#include "Behaviors/BehaviorGroup.h"

Be::VarChooser LineSetTypeChooser[] = {
	{ "ObjectRender", BeCast( EveChildLineSet::OBJECT_RENDER ), "sprites or other objects are rendered at each segment" },
	{ "LineRender", BeCast( EveChildLineSet::LINE_RENDER ), "To render a 3dLine shader between the points" },
	{ "Both", BeCast( EveChildLineSet::BOTH ), "Both of the above" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "LineSetTypes", EveChildLineSet::lineSetType, LineSetTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE( EveChildLineSet );

const Be::ClassInfo* EveChildLineSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildLineSet, "" )
		MAP_INTERFACE( EveChildLineSet )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2Renderable )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "alwaysOn", m_isAlwaysOn, "If false this will be hidden if a spaceobjects activation strength < 0.5. If True then it is always on.", Be::READWRITE | Be::PERSIST )
		MAP_PROPERTY_READONLY( "isUpdating", IsUpdating, "Is the EveChildLineSet updating." )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE_WITH_CHOOSER( "renderType", m_type, "", Be::READWRITE | Be::PERSIST | Be::ENUM | Be::NOTIFY, LineSetTypeChooser )

		MAP_ATTRIBUTE( "minScreenSize", m_minScreenSize, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "currentScreenSize", m_currentScreenSize, "render-threshold", Be::READ )

		MAP_ATTRIBUTE( "additiveBatches", m_additiveBatch, "Control for how the curveSet is rendered\n:jessica-group: LineRender", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "scrollSpeed", m_scrollSpeed, "controls the animation speed of the anim texture\n:jessica-group: LineRender", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "brightness", m_brightness, "a multiplier for the animColor render\n:jessica-group: LineRender", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "baseColor", m_baseColor, "color for lines\n:jessica-group: LineRender", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "animColor", m_animColor, "color for lines\n:jessica-group: LineRender", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_METHOD_AND_WRAP( "GetVertexElementAddedThroughCode", GetVertexElementAddedThroughCode, "for validation and objects requiring vertex elements added to the shader through code\n:jessica-hidden: True" )

		// leafs
		MAP_ATTRIBUTE( "lines", m_lines, "define any number of lines/paths to draw", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "lineSet", m_lineSet, ":jessica-hidden: True", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "mesh", m_mesh, "the rendered mesh", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_END()
}