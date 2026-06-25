// Copyright © 2016 CCP ehf.

#include "StdAfx.h"
#include "EveConnector.h"

Be::VarChooser EveConnectorTypeChooser[] = {
	{ "PointToPoint",
	  BeCast( EveConnector::PointToPoint ),
	  "Connection between two points" },
	{ "StraightAnchor",
	  BeCast( EveConnector::StraightAnchor ),
	  "Anchor line from dest to xz plane containing source" },
	{ "CurvedAnchor",
	  BeCast( EveConnector::CurvedAnchor ),
	  "Sphered line from dest to xz plane containing source using source as the center" },
	{ "XZ_Circle",
	  BeCast( EveConnector::XZ_Circle ),
	  "Circle in xz plane going through the 'CurvedAnchor' point with source as the center" },
	{ "XZ_CircleStraight",
	  BeCast( EveConnector::XZ_CircleStraight ),
	  "Circle in xz plane going through the 'StraightAnchor' point with source as the center" },
	{ "Circle",
	  BeCast( EveConnector::Circle ),
	  "Circle in the plane normal plane" },
	{ "Ellipse",
	  BeCast( EveConnector::Ellipse ),
	  "Ellipse in the plane normal plane, radiusX is destPosition.x, radiusY is destPosition.y and ellipse rotation is destPosition.z" },
	{ "Orbit",
	  BeCast( EveConnector::Orbit ),
	  "Draws an orbit using planeNormal and radius" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX(
	"EveConnectorStyle",
	EveConnector::ConnectorType,
	EveConnectorTypeChooser,
	ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE( EveConnector );

const Be::ClassInfo* EveConnector::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveConnector, "" )
		MAP_INTERFACE( EveConnector )

		MAP_ATTRIBUTE( "color", m_color, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "lineWidth", m_width, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "animationColor", m_animationColor, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "animationScale", m_animationScale, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "animationSpeed", m_animationSpeed, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "isAnimated", m_isAnimated, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "autoScaleAnimation", m_autoScaleAnimation, "", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "destPosition", m_destPosition, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "sourcePosition", m_sourcePosition, "", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "destObject", m_destObject, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "sourceObject", m_sourceObject, "", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "planeNormal", m_normal, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "length", m_length, "", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE_WITH_CHOOSER( "type", m_type, "", Be::READWRITE | Be::PERSIST | Be::ENUM, EveConnectorTypeChooser );

	EXPOSURE_END()
}