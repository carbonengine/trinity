////////////////////////////////////////////////////////////
//
//    Created:   2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "EveChildLineSet.h"
#include "Behaviors/BehaviorGroup.h"

Be::VarChooser LineSetTypeChooser[] =
{
	{ "ObjectRender", BeCast( EveChildLineSet::OBJECT_RENDER ), "sprites or other objects are rendered at each segment" },
	{ "LineRender", BeCast( EveChildLineSet::LINE_RENDER ), "To render a 3dLine shader between the points" },
	{ "Both", BeCast( EveChildLineSet::BOTH ), "Both of the above" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "LineSetTypes", EveChildLineSet::lineSetType, LineSetTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

Be::VarChooser LineSetObjTypeChooser[] =
{
	{ "Circle", BeCast( EveChildLineSet::CIRCLE ), "to author simple vfx circles" },
	{ "BezierCurve", BeCast( EveChildLineSet::BEZIER_CURVE ), "to author simple vfx curves" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "LineSetObjTypes", EveChildLineSet::lineSetObjType, LineSetObjTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE(EveChildLineSet);

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
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		
		MAP_ATTRIBUTE_WITH_CHOOSER( "objectType", m_objType, "", Be::READWRITE | Be::PERSIST | Be::ENUM | Be::NOTIFY, LineSetObjTypeChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "renderType", m_type, "", Be::READWRITE | Be::PERSIST | Be::ENUM | Be::NOTIFY, LineSetTypeChooser )

		MAP_ATTRIBUTE( "circleRadius", m_circleRadius, "the circle's radius\n"":jessica-group: Circle", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "circleDistort", m_circleDistort, "use this if you want to author an elipsoid or bend the circle in 3d space (.y and .w -> curvature) (.x and .z -> distort on x/z-axis)\n"":jessica-group: Circle", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "numSegments", m_exposedNumSegments, "nuber of segments that the circle is split up into\n:jessica-group: Circle", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "completeness", m_completeness, "is the circle complete or is it half a circle (0.5) etc [0-1]\n:jessica-group: Circle", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "point1", m_point1, "endpoint for the curve\n"":jessica-group: Curve", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "point2", m_point2, "endpoint for the curve\n"":jessica-group: Curve", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "bezierPoint", m_bezierPoint, "use this to bend the curve\n"":jessica-group: Curve", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "curveSegments", m_exposedCurveSegments, "nuber of segments that the curve is split up into\n:jessica-group: Curve", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		
		MAP_ATTRIBUTE( "lineWidth", m_lineWidth, "thickness of the circle's line\n:jessica-group: LineRender", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "baseColor", m_baseColor, "color for lines\n:jessica-group: LineRender", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "animColor", m_animColor, "color for lines\n:jessica-group: LineRender", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "additiveBatches", m_additiveBatch, "Control for how the curveSet is rendered\n:jessica-group: LineRender", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "scrollSpeed", m_scrollSpeed, "controls the animation speed of the anim texture\n:jessica-group: LineRender", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "scrollSegmenting", m_scrollSegmenting, "size of each animated portion\n:jessica-group: LineRender", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "brightness", m_brightness, "a multiplier for the animColor render\n:jessica-group: LineRender", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		
		MAP_ATTRIBUTE( "objectScale", m_objectScale, "increase/decrease the size of the mesh render\n:jessica-group: ObjectRenderer", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "billboardObject", m_billboardObject, "should mesh face camera \n:jessica-group: ObjectRenderer", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "animSpeed", m_animSpeed, "how fast the circle rotates. ( 0.5 -> 1 segment per sec )\n:jessica-group: ObjectRenderer", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "animValue", m_animValue, "is the circle complete or is it half a circle (0.5) etc [0-1]\n:jessica-group: ObjectRenderer", Be::READ )

		// leafs
		MAP_ATTRIBUTE( "lineSet", m_lineSet, ":jessica-hidden: True", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "mesh", m_mesh, "the rendered mesh", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_END()
}