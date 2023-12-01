#include "StdAfx.h"
#include "TriStepFilterVisibilityResults.h"
#include "Tr2VisibilityResults.h"

const Be::VarChooser TriVisibilityEventTypeChooser[] =
{
	// Name												Value															Docstring
	{ "TRIVISIBILITY_QUERY_BEGIN",						BeCast( Tr2VisibilityEvent::QUERY_BEGIN ),						"Begin of visibility query" }, 
	{ "TRIVISIBILITY_QUERY_END",						BeCast( Tr2VisibilityEvent::QUERY_END ),						"End of visibility query" }, 
	{ "TRIVISIBILITY_PORTAL_ENTER",						BeCast( Tr2VisibilityEvent::PORTAL_ENTER ),						"Enter a portal or mirror" }, 
	{ "TRIVISIBILITY_PORTAL_EXIT",						BeCast( Tr2VisibilityEvent::PORTAL_EXIT ),						"Exit a portal or mirror" }, 
	{ "TRIVISIBILITY_PORTAL_PRE_EXIT",					BeCast( Tr2VisibilityEvent::PORTAL_PRE_EXIT ),					"Before exiting a portal or mirror" }, 
	{ "TRIVISIBILITY_CELL_IMMEDIATE_REPORT",			BeCast( Tr2VisibilityEvent::CELL_IMMEDIATE_REPORT ),			"Entering a cell" }, 
	{ "TRIVISIBILITY_VIEW_PARAMETERS_CHANGED",			BeCast( Tr2VisibilityEvent::VIEW_PARAMETERS_CHANGED ),			"View parameters changed" }, 
	{ "TRIVISIBILITY_INSTANCE_VISIBLE",					BeCast( Tr2VisibilityEvent::INSTANCE_VISIBLE ),					"Object or light instance visible" }, 
	{ "TRIVISIBILITY_REMOVAL_SUGGESTED",				BeCast( Tr2VisibilityEvent::REMOVAL_SUGGESTED ),				"" }, 
	{ "TRIVISIBILITY_INSTANCE_IMMEDIATE_REPORT",		BeCast( Tr2VisibilityEvent::INSTANCE_IMMEDIATE_REPORT ),		"" }, 
	{ "TRIVISIBILITY_REGION_OF_INFLUENCE_ACTIVE",		BeCast( Tr2VisibilityEvent::REGION_OF_INFLUENCE_ACTIVE ),		"Entering a light region of influence" }, 
	{ "TRIVISIBILITY_REGION_OF_INFLUENCE_INACTIVE",		BeCast( Tr2VisibilityEvent::REGION_OF_INFLUENCE_INACTIVE ),		"Exiting a light region of influence" }, 
	{ "TRIVISIBILITY_STENCIL_MASK",						BeCast( Tr2VisibilityEvent::STENCIL_MASK ),						"" }, 
	{ "TRIVISIBILITY_TEXT_MESSAGE",						BeCast( Tr2VisibilityEvent::TEXT_MESSAGE ),						"" }, 
	{ "TRIVISIBILITY_DRAW_LINE_2D",						BeCast( Tr2VisibilityEvent::DRAW_LINE_2D ),						"" }, 
	{ "TRIVISIBILITY_DRAW_LINE_3D",						BeCast( Tr2VisibilityEvent::DRAW_LINE_3D ),						"" }, 
	{ "TRIVISIBILITY_DRAW_BUFFER",						BeCast( Tr2VisibilityEvent::DRAW_BUFFER ),						"" }, 
	{ "TRIVISIBILITY_OCCLUSION_QUERY_BEGIN",			BeCast( Tr2VisibilityEvent::OCCLUSION_QUERY_BEGIN ),			"" }, 
	{ "TRIVISIBILITY_OCCLUSION_QUERY_END",				BeCast( Tr2VisibilityEvent::OCCLUSION_QUERY_END ),				"" }, 
	{ "TRIVISIBILITY_OCCLUSION_QUERY_GET_RESULT",		BeCast( Tr2VisibilityEvent::OCCLUSION_QUERY_GET_RESULT ),		"" }, 
	{ "TRIVISIBILITY_OCCLUSION_QUERY_DRAW_DEPTH_TEST",	BeCast( Tr2VisibilityEvent::OCCLUSION_QUERY_DRAW_DEPTH_TEST ),	"" }, 
	{ "TRIVISIBILITY_INSTANCE_DRAW_DEPTH",				BeCast( Tr2VisibilityEvent::INSTANCE_DRAW_DEPTH ),				"" }, 
	{ "TRIVISIBILITY_FLUSH_DEPTH",						BeCast( Tr2VisibilityEvent::FLUSH_DEPTH ),						"" }, 
	{ "TRIVISIBILITY_DEPTH_PASS_BEGIN",					BeCast( Tr2VisibilityEvent::DEPTH_PASS_BEGIN ),					"" }, 
	{ "TRIVISIBILITY_DEPTH_PASS_END",					BeCast( Tr2VisibilityEvent::DEPTH_PASS_END ),					"" }, 
	{ "TRIVISIBILITY_COLOR_PASS_BEGIN",					BeCast( Tr2VisibilityEvent::COLOR_PASS_BEGIN ),					"" }, 
	{ "TRIVISIBILITY_COLOR_PASS_END",					BeCast( Tr2VisibilityEvent::COLOR_PASS_END ),					"" }, 
	{ "TRIVISIBILITY_TILE_BEGIN",						BeCast( Tr2VisibilityEvent::TILE_BEGIN ),						"" }, 
	{ "TRIVISIBILITY_TILE_END",							BeCast( Tr2VisibilityEvent::TILE_END ),							"" }, 
	{ "TRIVISIBILITY_FLUSH_GPU_COMMAND_BUFFER",			BeCast( Tr2VisibilityEvent::FLUSH_GPU_COMMAND_BUFFER ),			"" }, 
	{0}
};

BLUE_REGISTER_ENUM( "TRIVISIBILITY_EVENT_TYPE", Tr2VisibilityEvent::EventType, TriVisibilityEventTypeChooser );

const Be::VarChooser TriVisibilityResultsFilterTypeChooser[] =
{
	// Name												Value																Docstring
	{ "TRIVISIBILITY_FILTER_ONLY_OBJECTS_IN_LIST",		BeCast( TriStepFilterVisibilityResults::ONLY_OBJECTS_IN_LIST ),		"Only allow objects/lights in the objects list" }, 
	{ "TRIVISIBILITY_FILTER_EXCLUDE_OBJECTS_IN_LIST",	BeCast( TriStepFilterVisibilityResults::EXCLUDE_OBJECTS_IN_LIST ),	"Exclude objects/lights in the objects list" }, 
	{0}
};

BLUE_REGISTER_ENUM( "TRIVISIBILITY_FILTER_TYPE", TriStepFilterVisibilityResults::FilterType, TriVisibilityResultsFilterTypeChooser );


BLUE_DEFINE( TriStepFilterVisibilityResults );

const Be::ClassInfo* TriStepFilterVisibilityResults::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepFilterVisibilityResults, "Render step for filtering visibility results list" )

		MAP_INTERFACE( TriStepFilterVisibilityResults )

		MAP_ATTRIBUTE_WITH_CHOOSER
		(
			"eventFilter", 
			m_eventFilter,
			"Which events are copied to new visibility results",
			Be::READWRITE | Be::PERSIST,
			TriVisibilityEventTypeChooser
		)

		MAP_ATTRIBUTE_WITH_CHOOSER
		(
			"filterType", 
			m_filterType,
			"Type of per-object filtering to perform on visibility results",
			Be::READWRITE | Be::PERSIST | Be::ENUM,
			TriVisibilityResultsFilterTypeChooser
		)

		MAP_ATTRIBUTE
		(
			"objects", 
			m_objects,
			"List of objects to include/exclude from visibility results",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"inputResults", 
			m_inputResults,
			"Input visibility results to be filtered",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"outputResults", 
			m_outputResults,
			"Output visibility results",
			Be::READWRITE | Be::PERSIST
		)

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		( 
			"__init__", 
			py__init__, 
			4,
			"Create a visibility results filter to exclude objects/events from visibility results\n"
			":param source: source visibility results\n"
			":param dest: destination visibility results\n"
			":param eventFilter: Which events are copied to new visibility results (default is All)\n"
			":param filterType: Type of per-object filtering to perform on visibility results\n"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}