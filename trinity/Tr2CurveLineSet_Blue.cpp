// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2CurveLineSet.h"

BLUE_DEFINE( Tr2CurveLineSet );

const Be::ClassInfo* Tr2CurveLineSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveLineSet, "" )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2Pickable )

		MAP_ATTRIBUTE( "lineEffect", m_lineEffect, "The effect to use to draw the 3d lines", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pickEffect", m_pickEffect, "The effect used to pick the 3d lines", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "lineWidthFactor", m_lineWidthFactor, "An additional factor for line width for this whole set", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "additive", m_additive, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "A name for this line set", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "depthOffset", m_depthOffset, "Depth offset for transparency sorting", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"AddStraightLine",
			AddStraightLine,
			"Adds a straight line to the line set, but does not submit it"
			"\n"
			"\n:param startPosition: The start position of the line"
			"\n:param startColor: The color of the line at start position"
			"\n:param endPosition: The end position of the line"
			"\n:param endColor: The color of the line at end position"
			"\n:param lineWidth: The width of the line in pixel" )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"AddCurvedLineCrt",
			AddCurvedLineCrt,
			1,
			"Adds a curved line (based on splines) to the line set using cartesian coordinates, but does not submit it"
			"\n"
			"\n:param startPosition: The start position of the line in 3d cartesian space"
			"\n:param startColor: The color of the line at start position"
			"\n:param endPosition: The end position of the line in 3d cartesian space"
			"\n:param endColor: The color of the line at end position"
			"\n:param middle: The center control point"
			"\n:param lineWidth: The width of the line in pixel"
			"\n:param segments: the number of line subdivisions, defaults to 20" )

		MAP_METHOD_AND_WRAP(
			"AddCurvedLineSph",
			AddCurvedLineSph,
			"Adds a curved line (based on splines) to the line set using spherical coordinates, but does not submit it"
			"\n"
			"\n:param startPosition: The start position of the line in 3d spherical coordinates (phi, theta, radius)"
			"\n:param startColor: The color of the line at start position"
			"\n:param endPosition: The end position of the line in 3d spherical coordinates (phi, theta, radius)"
			"\n:param endColor: The color of the line at end position"
			"\n:param center: The center of the sphere the spherical coordinates are based on"
			"\n:param middle: The center control point"
			"\n:param lineWidth: The width of the line in pixel" )

		MAP_METHOD_AND_WRAP(
			"AddSpheredLineCrt",
			AddSpheredLineCrt,
			"Adds a sphered line (a straight line on a sphere) to the line set using cartesian coordinates, but does not submit it"
			"\n"
			"\n:param startPosition: The start position of the line on the surface of the sphere in 3d cartesian space"
			"\n:param startColor: The color of the line at start position"
			"\n:param endPosition: The end position of the line on the surface of the sphere in 3d cartesian space"
			"\n:param endColor: The color of the line at end position"
			"\n:param center: The center of the sphere"
			"\n:param lineWidth: The width of the line in pixel" )

		MAP_METHOD_AND_WRAP(
			"AddSpheredLineSph",
			AddSpheredLineSph,
			"Adds a sphered line (a straight line on a sphere) to the line set using spherical coordinates, but does not submit it"
			"\n"
			"\n:param startPosition: The start position of the line on the surface of the sphere in 3d spherical coordinates (phi, theta, radius)"
			"\n:param startColor: The color of the line at start position"
			"\n:param endPosition: The end position of the line on the surface of the sphere in 3d spherical coordinates (phi, theta, radius)"
			"\n:param endColor: The color of the line at end position"
			"\n:param center: The center of the sphere"
			"\n:param lineWidth: The width of the line in pixel" )

		MAP_METHOD_AND_WRAP(
			"ChangeLineColor",
			ChangeLineColor,
			"Changes the start and end color of a line. Requires a call to SubmitChanges before being updated on the video card."
			"\n"
			"\n:param lineID: The line ID returned by a previous call to AddXXX()"
			"\n:param startColor: The new color of the line at start position"
			"\n:param endColor: The new color of the line at end position" )

		MAP_METHOD_AND_WRAP(
			"ChangeLineWidth",
			ChangeLineWidth,
			"Changes the width of a line. Requires a call to SubmitChanges before being updated on the video card."
			"\n"
			"\n:param lineID: The line ID returned by a previous call to AddXXX()"
			"\n:param width: The new width of the line" )

		MAP_METHOD_AND_WRAP(
			"ChangeLinePositionCrt",
			ChangeLinePositionCrt,
			"Changes only the start and endpositions of a line, no matter what type of line. Requires a call to SubmitChanges before being updated on the video card."
			"\n"
			"\n:param lineID: The line ID returned by a previous call to AddXXX()"
			"\n:param startPosition: The start position of the line on the surface of the sphere in 3d cartesian space"
			"\n:param endPosition: The end position of the line on the surface of the sphere in 3d cartesian space" )

		MAP_METHOD_AND_WRAP(
			"ChangeLinePositionSph",
			ChangeLinePositionSph,
			"Changes only the start and endpositions of a line using spherical coordinates, no matter what type of line. Requires a call to SubmitChanges before being updated on the video card."
			"\n"
			"\n:param lineID: The line ID returned by a previous call to AddXXX()"
			"\n:param startPosition: The start position of the line on the surface of the sphere in 3d spherical coordinates (phi, theta, radius)"
			"\n:param endPosition: The end position of the line on the surface of the sphere in 3d spherical coordinates (phi, theta, radius)"
			"\n:param center: The center of the sphere the spherical coordinates are based on in 3d cartesian space" )

		MAP_METHOD_AND_WRAP(
			"ChangeLineIntermediateCrt",
			ChangeLineIntermediateCrt,
			"Changes only the intermediate position of a line, no matter what type of line. Requires a call to SubmitChanges before being updated on the video card."
			"\n"
			"\n:param lineID: The line ID returned by a previous call to AddXXX()"
			"\n:param intermediatePosition: The new intermediate position of the line on the surface of the sphere in 3d cartesian space" )

		MAP_METHOD_AND_WRAP(
			"ChangeLineIntermediateSph",
			ChangeLineIntermediateSph,
			"Changes only the intermediate position of a line using spherical coordinates, no matter what type of line. Requires a call to SubmitChanges before being updated on the video card."
			"\n"
			"\n:param lineID: The line ID returned by a previous call to AddXXX()"
			"\n:param intermediatePosition: The new intermediate position of the line on the surface of the sphere in 3d spherical coordinates (phi, theta, radius)"
			"\n:param center: The center of the sphere the spherical coordinates are based on in 3d cartesian space" )

		MAP_METHOD_AND_WRAP(
			"ChangeLineMultiColor",
			ChangeLineMultiColor,
			"Changes the multicolor settings of a line, so it will have a seperate color until a border. Requires a call to SubmitChanges before being updated on the video card."
			"\n"
			"\n:param lineID: The line ID returned by a previous call to AddXXX()"
			"\n:param color: The color along the line until the border value is reached"
			"\n:param border: Border value along the line, goes from 0.0 to 1.0" )

		MAP_METHOD_AND_WRAP(
			"ChangeLineAnimation",
			ChangeLineAnimation,
			"Changes the animation settings of a line. Requires a call to SubmitChanges before being updated on the video card."
			"\n"
			"\n:param lineID: The line ID returned by a previous call to AddXXX()"
			"\n:param color: The color along the line until the border value is reached"
			"\n:param speed: The speed of the pattern/texture crawls along the line"
			"\n:param scale: The scale of the pattern/texture across the the line" )

		MAP_METHOD_AND_WRAP(
			"ChangeLineSegmentation",
			ChangeLineSegmentation,
			"Changes the number of segments a curved line is made of. Does not work with straight lines! Requires a call to SubmitChanges before being updated on the video card."
			"\n"
			"\n:param lineID: The line ID returned by a previous call to AddXXX()"
			"\n:param numOfSegments: number of segments" )

		MAP_METHOD_AND_WRAP(
			"RemoveLine",
			RemoveLine,
			"Removes a line from this line set! Requires a call to SubmitChanges before being updated on the video card."
			"\n"
			"\n:param lineID: The line ID returned by a previous call to AddXXX()" )

		MAP_METHOD_AND_WRAP(
			"ClearLines",
			ClearLines,
			"Clears all lines. Requires a call to SubmitChanges before being updated on the video card." )

		MAP_METHOD_AND_WRAP(
			"SubmitChanges",
			SubmitChanges,
			"Submits all changes of this line set to the video card. This is the critical one, try not calling this every frame!" )

	EXPOSURE_END()
}
