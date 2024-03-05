#pragma once
#ifndef TriStepRenderLineGraph_h
#define TriStepRenderLineGraph_h


#include "TriRenderStep.h"
#include "Tr2LineGraph.h"

BLUE_DECLARE( TriStepRenderLineGraph );

class TriStepRenderLineGraph:
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	TriStepRenderLineGraph( IRoot* lockobj = NULL );
	~TriStepRenderLineGraph();

	void py__init__( 
		const std::vector<Tr2LineGraph*>& graphs, 
		Be::Optional<float> legendScale,
		Be::Optional<float> scale,
		Be::Optional<bool> autoScale );

	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

protected:
	PTr2LineGraphVector m_lineGraphs;
	float m_legendScale;

	// Scale for values in the graph. If auto scale is set, this is
	// calculated on every Execute call - otherwise it is used as is.
	float m_scale;

	// Maximum scale for the auto scale
	float m_maxLegend;

	// If auto scale is set, the scale is calculated on every Execute call.
	bool m_autoScale;

	// If show legend is set, a legend is added. Note that this uses debug rendering
	// so it does not work on Mac.
	bool m_showLegend;

	// Optional callback into Python when scale changes with auto scaling enabled.
	BlueScriptCallback m_scaleChangeCallback;
};

TYPEDEF_BLUECLASS( TriStepRenderLineGraph );
#endif //TriStepRenderLineGraph_h