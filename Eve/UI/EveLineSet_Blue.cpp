#include "StdAfx.h"
#include "EveLineSet.h"

BLUE_DEFINE( EveLineSet );

const Be::ClassInfo* EveLineSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveLineSet, "" )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IEveTransform )
		MAP_INTERFACE( IEveSpaceObject2 )

		MAP_ATTRIBUTE( "name", m_name, "A name for this line set", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "Toggle visibility for this line set", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE
		(
			"effect",  
			m_effect, 
			"The effect to use to draw the particles", 
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"renderTransparent",
			m_isRenderedAsTransparent,
			"Toggles rendering transparently, compared to the default of additive.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(    
			"translationCurve",
			m_ballPosition,
			"Vector function slot for attaching a destiny ball to set the position of a ship",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(    
			"rotationCurve",
			m_ballRotation,
			"Quaternion function slot for attaching a destiny ball to set the rotation of a ship",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( 
			"AddLine", 
			AddLine, 
			"Adds a line to the line set, but does not submit it. Returns line ID.\n"
			":param startPosition: line start position\n"
			":param startColor: line start color\n"
			":param endPosition: line end position\n"
			":param endColor: line end color\n"
			)
		MAP_METHOD_AND_WRAP( 
			"RemoveLine", 
			RemoveLine, 
			"Removes a line, requires a call to SubmitChanges before being updated on the video card.\n"
			":param idx: line ID\n"
			)

		MAP_METHOD_AND_WRAP( 
			"ChangeLine", 
			ChangeLine, 
			"Changes the properties of a line. Requires a call to SubmitChanges before it will show up.\n"
			":param idx: line ID\n"
			":param startPosition: line start position\n"
			":param startColor: line start color\n"
			":param endPosition: line end position\n"
			":param endColor: line end color\n"
			)
		MAP_METHOD_AND_WRAP( 
			"ChangeLineColor", 
			ChangeLineColor, 
			"Changes just the colour of a line. Requires a call to SubmitChanges before it will show up.\n"
			":param idx: line ID\n"
			":param startColor: line start color\n"
			":param endColor: line end color\n"
			)
		MAP_METHOD_AND_WRAP( 
			"ChangeLinePosition", 
			ChangeLinePosition, 
			"Changes the start and endpositions of a line. Requires a call to SubmitChanges before it will show up.\n"
			":param idx: line ID\n"
			":param startPosition: line start position\n"
			":param endPosition: line end position\n"
			)
		MAP_METHOD_AND_WRAP( "ClearLines", ClearLines, "Clears all lines. Requires a call to SubmitChanges to complete." )
		MAP_METHOD_AND_WRAP( "SubmitChanges", SubmitChanges, "Submits changes, returning false if it fails for any reason." )

	EXPOSURE_END()
}
