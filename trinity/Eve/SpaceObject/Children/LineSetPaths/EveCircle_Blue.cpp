// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveCircle.h"

BLUE_DEFINE( EveCircle );

const Be::ClassInfo* EveCircle::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveCircle, "" )
		MAP_INTERFACE( EveCircle )
		MAP_INTERFACE( IEveLineSetPath )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "isVisible", m_isVisible, "", Be::READ )

		MAP_ATTRIBUTE( "circleRadius", m_circleRadius, "the circle's radius\n"
													   ":jessica-group: Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "circleDistort", m_circleDistort, "use this to bend/transform/animate the circle on the y axis\n"
														 ":jessica-group: Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "numSegments", m_segments, "nuber of segments that the circle is split up into\n"
												  ":jessica-group: Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "completeness", m_completeness, "attribute to move the ends conecting to either side making an arc [0-2]\n"
													   ":jessica-group: Options",
					   Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "startPoint", m_startPoint, "used with completeness to control where the gap is [0-1]\n"
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
		MAP_ATTRIBUTE( "billboardObjects", m_billboardObjects, ":jessica-group: Mesh-Options", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
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