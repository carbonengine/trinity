// Copyright © 2016 CCP ehf.

#include "StdAfx.h"
#include "EveTacticalOverlay.h"

BLUE_DEFINE( EveTacticalOverlayTrackObject );
const Be::ClassInfo* EveTacticalOverlayTrackObject::ExposeToBlue(){
	EXPOSURE_BEGIN( EveTacticalOverlayTrackObject, "" )
		MAP_INTERFACE( EveTacticalOverlayTrackObject )

			MAP_ATTRIBUTE( "translationCurve", m_positionCurve, "", Be::READWRITE | Be::PERSIST )
				MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
					MAP_ATTRIBUTE( "radius", m_radius, "", Be::READWRITE | Be::PERSIST )
						MAP_ATTRIBUTE( "isAggressive", m_aggressive, "", Be::READWRITE | Be::PERSIST )
							MAP_ATTRIBUTE( "showVelocity", m_showVelocity, "", Be::READWRITE | Be::PERSIST )
								EXPOSURE_END()
}

BLUE_DEFINE( EveTacticalOverlay );
const Be::ClassInfo* EveTacticalOverlay::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveTacticalOverlay, "" )
		MAP_INTERFACE( EveTacticalOverlay )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "trackObjects", m_trackObjects, "", Be::READ )
		MAP_ATTRIBUTE( "interestObject", m_interestObject, "", Be::READWRITE )

		MAP_ATTRIBUTE( "anchorEffect", m_anchorEffect, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "connectorEffect", m_connectorEffect, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "velocityEffect", m_velocityEffect, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "activeRange", m_ranges.x, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rangeFadeLength", m_ranges.y, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rangeMultiplier", m_ranges.z, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sourceRadius", m_ranges.w, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "interestRange", m_interestRange, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "outsideInterestIntensity", m_outsideInterestIntensity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minRadiusForRange", m_minRadiusForRange, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "segmentsLow", m_connectorSegmentsLow, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "segmentsMedium", m_connectorSegmentsMedium, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "segmentsHigh", m_connectorSegmentsHigh, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "totalSegmentsLast", m_totalSegmentsLast, "", Be::READ )
		MAP_ATTRIBUTE( "requestedSegmentsLast", m_requestedSegmentsLast, "", Be::READ )
		MAP_ATTRIBUTE( "targetMaxSegments", m_targetSegmentCount, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "arcSegmentMultiplier", m_arcSegmentMultiplier, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "segmentCountMultiplier", m_segmentCountMultiplier, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "translationCurve", m_positionCurve, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldPosition", m_rootPosition, "", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}