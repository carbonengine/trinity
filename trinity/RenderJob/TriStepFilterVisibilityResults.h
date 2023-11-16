////////////////////////////////////////////////////////////
//
//    Created:   August 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef TriStepFilterVisibilityResults_h
#define TriStepFilterVisibilityResults_h


#include "TriRenderStep.h"

BLUE_DECLARE( Tr2VisibilityResults );
BLUE_DECLARE_IVECTOR( IRoot );

// -------------------------------------------------------------
// Description:
//   TriStepFilterVisibilityResults is a render step that
//   filters a given visibility results
//   set to produce a new one removing specified events and/or
//   object instances.
// SeeAlso:
//   TriRenderStep, Tr2VisibilityResults
// -------------------------------------------------------------
BLUE_CLASS( TriStepFilterVisibilityResults ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepFilterVisibilityResults( IRoot* lockobj = 0 );
	~TriStepFilterVisibilityResults(void);

	void py__init__( 
		Tr2VisibilityResults* input, 
		Tr2VisibilityResults* output,
		Be::Optional<unsigned> eventFilter,
		Be::Optional<unsigned> filter );

	// RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	// Type of per-object filter
	enum FilterType
	{
		// Output events that have objects specified in the object list only
		ONLY_OBJECTS_IN_LIST,
		// Output events that have objects not specified in the object list only
		EXCLUDE_OBJECTS_IN_LIST,
	};

	void SetInputResults( Tr2VisibilityResults* results )
	{
		m_inputResults = results;
	}
	void SetOutputResults( Tr2VisibilityResults* results )
	{
		m_outputResults = results;
	}

	void SetEventFilter( unsigned int eventFilter );
	void SetFilterType( FilterType filterType );

private:
	// Bitfield for filter on event type
	uint32_t m_eventFilter;
	// Type of per-object filter
	FilterType m_filterType;
	// List of objects to include/exclude in the result
	PIRootVector m_objects;
	// Input visibility results list to process
	Tr2VisibilityResultsPtr m_inputResults;
	// Output visibility results
	Tr2VisibilityResultsPtr m_outputResults;
};

TYPEDEF_BLUECLASS( TriStepFilterVisibilityResults );

#endif // TriStepFilterVisibilityResults_h