// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepRenderLineGraph.h"

BLUE_DEFINE( TriStepRenderLineGraph );

static const char* INIT_DOC_STRING = "Optional arguments are (sequence,float,float,bool)";

const Be::ClassInfo* TriStepRenderLineGraph::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepRenderLineGraph, "" )
		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRenderLineGraph )

		MAP_ATTRIBUTE(
			"lineGraphs",
			m_lineGraphs,
			"",
			Be::READ )
		MAP_ATTRIBUTE(
			"autoScale",
			m_autoScale,
			"If set, then scale is calculated automatically to fit the data in the line\n"
			"graphs. If not set, the scale is fixed to whatever is entered in the scale field.",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"showLegend",
			m_showLegend,
			"If show legend is set, a legend is added. Note that this uses debug rendering"
			"so it does not work on Mac",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"maxLegend",
			m_maxLegend,
			"Maximum scale allowed for auto scale. ",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"scale",
			m_scale,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"legendScale",
			m_legendScale,
			"",
			Be::READWRITE | Be::PERSIST )

#if BLUE_WITH_PYTHON
		MAP_ATTRIBUTE(
			"scaleChangeCallback",
			m_scaleChangeCallback,
			"Optional callback that is called when scale changes with auto scaling enabled",
			Be::READWRITE )
#endif

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			4,
			":param graphs: list of graphs\n"
			":param legendScale: scale of the legend\n"
			":param scale: graph scale\n"
			":param autoScale: use automatic scale to content\n" )

	EXPOSURE_CHAINTO( TriRenderStep )
}
