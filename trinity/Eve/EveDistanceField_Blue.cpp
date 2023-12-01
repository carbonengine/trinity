#include "StdAfx.h"
#include "EveDistanceField.h"

BLUE_DEFINE( EveDistanceField );

const Be::ClassInfo* EveDistanceField::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistanceField, "" )
        MAP_INTERFACE( EveDistanceField )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "cameraView", m_cameraView, "na", Be::READWRITE )
		MAP_ATTRIBUTE( "objects", m_objects, "na", Be::READ | Be::NOTIFY  )
		MAP_ATTRIBUTE( "curveSet", m_curveSet, "na", Be::READWRITE )
		MAP_ATTRIBUTE( "distance", m_distance, "na", Be::READWRITE )
		
		MAP_ATTRIBUTE( "timeAdjustmentSecondsOut", 
						m_timeAdjustmentSecondsOut, 
						"Adjust how long it takes to settle on a new value when zooming out", 
						Be::READWRITE )
		MAP_ATTRIBUTE( "timeAdjustmentSecondsIn", 
						m_timeAdjustmentSecondsIn, 
						"Adjust how long it takes to settle on a new value when zooming in", 
						Be::READWRITE )

		MAP_ATTRIBUTE( "dimensions", m_dimensions, "", Be::READ );
		MAP_ATTRIBUTE( "midpoint", m_middle, "", Be::READ );
		MAP_ATTRIBUTE( "distanceThreshold", m_distanceThreshold, "", Be::READWRITE );
		MAP_ATTRIBUTE( "maxXZRatio", m_maxXZRatio, "", Be::READWRITE );
		MAP_ATTRIBUTE( "minYRatio", m_minYRatio, "", Be::READWRITE );
		
		MAP_ATTRIBUTE( "minDistance", m_minDistance, "The threshold distance from an object in the field where you are 100% inside the environment", Be::READWRITE | Be::NOTIFY );
		MAP_ATTRIBUTE( "maxDistance", m_maxDistance, "The threshold distance from an object in the field where you are outside the environment", Be::READWRITE | Be::NOTIFY );
		
		MAP_METHOD_AND_WRAP( 
			"SetupDynamicDistanceField", 
			SetupDynamicDistanceField, 
			"Sets up the distance  field as a dynamic distance field\n"
			":param threshold: distance threshold\n"
			":param timeOut: time adjustment seconds out\n"
			":param timeIn: time adjustment seconds in\n"
			)
		MAP_METHOD_AND_WRAP( 
			"SetupStaticDistanceField", 
			SetupStaticDistanceField, 
			"Sets up the distance field as a static distance field\n"
			":param dimensions: field size\n"
			":param position: field position\n"
			":param threshold: distance threshold\n"
			":param timeOut: time adjustment seconds out\n"
			":param timeIn: time adjustment seconds in\n"
			)

	EXPOSURE_END()
}