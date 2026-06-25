// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveBezierCurve.h"

BLUE_DEFINE_INTERFACE( IEveLineSetPath );

BLUE_DEFINE( EveBezierCurve );

const Be::ClassInfo* EveBezierCurve::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveBezierCurve, "" )
		MAP_INTERFACE( EveBezierCurve )
		MAP_INTERFACE( IEveLineSetPath )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "isVisible", m_isVisible, "", Be::READ )

		MAP_ATTRIBUTE( "point1", m_point1, "endpoint for the curve (multiplied by scale)\n"
										   ":jessica-group: Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "point2", m_point2, "endpoint for the curve (multiplied by scale)\n"
										   ":jessica-group: Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "bezierPoint", m_bezierPoint, "use this to bend the curve\n"
													 ":jessica-group: Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "completeness", m_completeness, "can be animated to have the line only partialy complete it's path\n"
													   ":jessica-group: Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "segments", m_segments, "nuber of segments that the curve is split up into\n"
											   ":jessica-group: Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "segmentOffset", m_segmentOffset, "offset to precicely position points\n"
														 ":jessica-group: Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "lineWidth", m_lineWidth, "embolden the lines as you wish \n"
												 ":jessica-group: lineRender-Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "scaleSegmentsByCompleteness", m_scaleSegmentsByCompleteness, "reduce number of segments when completeness != 1\n"
																					 ":jessica-group: Mesh-Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "scaleEndpoints", m_scaleEndpoints, "reduce mesh size of objects at the ends\n"
														   ":jessica-group: Mesh-Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "billboardObjects", m_billboardObjects, "should meshes face the camera \n"
															   ":jessica-group: Mesh-Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "objectScale", m_objectScale, "enlarge/reduce object mesh size\n"
													 ":jessica-group: Mesh-Options",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "movementSpeed", m_movementSpeed, "how fast objects move\n"
														 ":jessica-group: Mesh-Options",
					   Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "animValue", m_animValue, "debug location in relation to the 'movementSpeed' var\n"
												 ":jessica-group: Mesh-Options",
					   Be::READ )

	EXPOSURE_END()
}