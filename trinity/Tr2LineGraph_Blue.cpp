#include "StdAfx.h"
#include "Tr2LineGraph.h"

BLUE_DEFINE( Tr2LineGraph );

const Be::ClassInfo* Tr2LineGraph::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2LineGraph, "" )

		MAP_INTERFACE( Tr2LineGraph )
		MAP_INTERFACE( ICcpStatisticsAccumulator )

		MAP_ATTRIBUTE
		(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"color",
			m_color,
			"",
			Be::READWRITE | Be::PERSIST
		)

		MAP_METHOD_AND_WRAP( 
			"AddMarker", 
			AddMarker, 
			"Adds a marker to the graph\n"
			":param name: marker name" 
		);
		MAP_METHOD_AND_WRAP( 
			"SetSize", 
			SetSize, 
			"Sets the size of the buffer of statistics\n" 
			":param size: buffer size"
		)
		MAP_METHOD_AND_WRAP( "GetStatsHistory", GetStatsHistory, "Gets a list of the statistics in the buffer" )

    EXPOSURE_END()
}
