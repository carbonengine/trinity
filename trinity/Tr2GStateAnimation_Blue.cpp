#include "StdAfx.h"

#if GSTATE_ENABLED

#include "Tr2GStateAnimation.h"
#include "Resources/TriGeometryRes.h"
#include "Resources/TriGrannyRes.h"
#include "Resources/Tr2GrannyStateRes.h"

BLUE_DEFINE( Tr2GStateAnimation );

const Be::ClassInfo* Tr2GStateAnimation::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GStateAnimation, "" )
		MAP_INTERFACE( Tr2GStateAnimation )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2AnimationUpdater )

		MAP_PROPERTY( "resPath", GetResPath, SetResPath, "The resource path to the Granny file containing the animations to be played." )
		MAP_ATTRIBUTE( "resPath_", m_resPath, "", Be::PERSISTONLY )
		MAP_PROPERTY( "gStateResPath", GetGStateResPath, SetGStateResPath, "The resource path to a base layer GState file containing animations to be played." )
		MAP_ATTRIBUTE( "gStateResPath_", m_gStateResPath, "", Be::PERSISTONLY )
		MAP_ATTRIBUTE( "grannyRes", m_grannyRes, "", Be::READ )
		MAP_PROPERTY( "model", GetModel, SetModel, "" )
		MAP_ATTRIBUTE( "model_", m_model, "", Be::PERSISTONLY )
		MAP_ATTRIBUTE( "parameters", m_gStateParameterList, "", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"debugRenderSkeleton", m_debugRenderSkeleton, "If set, and a debug renderer is set, then the skeleton is rendered with lines connecting\n"
														  "the joints of the skeleton.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"debugRenderJointNames", m_debugRenderJointNames, "If set, and a debug renderer is set, then the names of the joints in the skeleton\n"
															  "are displayed at their projected locations.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"animationEnabled", m_animationEnabled, "Enable/disable animation update", Be::READWRITE )

		MAP_ATTRIBUTE(
			"eventListener",
			m_eventListener,
			"An event listener that's triggered by granny text track events.",
			Be::READWRITE )



		MAP_METHOD_AND_WRAP(
			"TogglePauseAnimations",
			TogglePauseAnimations,
			"TogglePauseAnimations( pause )\n\n"
			"Pauses/unpauses animation playback."
			":param pause: boolean True for pause, boolean False to unpause" )

		MAP_METHOD_AND_WRAP(
			"LoadModelFromGstate",
			LoadModelFromGstate,
			"LoadModelFromGstate()\n\n"
			"Loads model resource file." )

		MAP_METHOD_AND_WRAP(
			"GetGStateAnimFileRefPaths",
			GetGStateAnimFileRefPaths,
			"GetGStateAnimFileRefPaths( )\n\n"
			"Returns list of GState anim file ref paths" )

		MAP_METHOD_AND_WRAP(
			"GetTopLevelNodeNames",
			GetTopLevelNodeNames,
			"GetTopLevelNodeNames()\n\n"
			"Returns a list of names of top-level nodes in the state machine." )

		MAP_METHOD_AND_WRAP(
			"GetTopLevelParameterNodeNames",
			GetTopLevelParameterNodeNames,
			"GetTopLevelParameterNodeNames()\n\n"
			"Returns a list of names of top-level parameter nodes in the state machine." )

		MAP_METHOD_AND_WRAP(
			"GetTopLevelStateNodeNames",
			GetTopLevelStateNodeNames,
			"GetTopLevelStateNodeNames()\n\n"
			"Returns a list of names of top-level state nodes in the state machine." )

		MAP_METHOD_AND_WRAP(
			"GetParameters",
			GetParameters,
			"GetParameters( param_node )\n\n"
			"Returns a list of names of parameters on a parameter node.\n"
			":param nodeName: parameter node name" )

		MAP_METHOD_AND_WRAP(
			"GetParameterRange",
			GetParameterRange,
			"GetParameterRange( param_node, param_name )\n\n"
			"Returns a list containing minimum and maximum values for a parameter.\n"
			":param nodeName: parameter node name\n"
			":param paramName: parameter name" )

		MAP_METHOD_AND_WRAP(
			"GetActiveMachineElementName",
			GetActiveMachineElementName,
			"GetActiveMachineElementName()\n\n"
			"Returns the name of the active state or transition object." )

		MAP_METHOD_AND_WRAP(
			"GetStartStateIdx",
			GetStartStateIdx,
			"GetStartStateIdx()\n\n"
			"Returns the index of the starting state." )

		MAP_METHOD_AND_WRAP(
			"SetStartStateIdx",
			SetStartStateIdx,
			"SetStartStateIdx( index )\n\n"
			"Sets the starting state by index.\n"
			":param idx: index for starting state" )

		MAP_METHOD_AND_WRAP(
			"SetStartStateByName",
			SetStartStateByName,
			"SetStartStateByName( name )\n\n"
			"Sets the starting state by name.\n"
			":param name: name for starting state" )

		MAP_METHOD_AND_WRAP(
			"RequestChangeToState",
			RequestChangeToState,
			"RequestChangeToState( state_name )\n\n"
			"Requests a change to a particular state, through any intermediate transitions. Returns boolean that indicates whether the change will take place.\n"
			":param state_name: name for destination state" )

		MAP_METHOD_AND_WRAP(
			"ForceChangeToState",
			ForceChangeToState,
			"ForceChangeToState( state_name )\n\n"
			"Forces a change to a particular state, through any intermediate transitions. Returns boolean that indicates whether the change will take place.\n"
			":param state_name: name for destination state" )

		MAP_METHOD_AND_WRAP(
			"StartTransitionByName",
			StartTransitionByName,
			"StartTransitionByName( transition_name )\n\n"
			"Requests taking a particular transition. Returns boolean that indicates whether the change will take place.\n"
			":param transition_name: name for transition to start" )

		MAP_METHOD_AND_WRAP(
			"GetParameter",
			GetParameter,
			"GetParameter( parameter_node_name, param_index )\n\n"
			"Queries the value of a given parameter in a parameter node.  Returns float.\n"
			":param parameter_node_name: name for parameter node to query\n"
			":param param_index: index for parameter to query." )

		MAP_METHOD_AND_WRAP(
			"SetParameter",
			SetParameter,
			"SetParameter( parameter_node_name, param_index, value )\n\n"
			"Instantly sets a parameter by index on a named parameter node, to a float value.\n"
			":param parameter_node_name: name for parameter node to query\n"
			":param param_index: index for parameter to query.\n"
			":param value: value to set." )

		MAP_METHOD_AND_WRAP(
			"RequestParameter",
			RequestParameter,
			"RequestParameter( parameter_node_name, param_index, value )\n\n"
			"Sets a parameter by index on a named parameter node, to a float value, allowing a smooth transition.\n"
			":param parameter_node_name: name for parameter node to query\n"
			":param param_index: index for parameter to query.\n"
			":param value: value to set.\n" )

		MAP_METHOD_AND_WRAP(
			"RequestParameterByName",
			RequestParameterByName,
			"RequestParameter( parameter_node_name, param_name, value )\n\n"
			"Sets a parameter by index on a named parameter node, to a float value, allowing a smooth transition.\n"
			":param parameter_node_name: name for parameter node to request.\n"
			":param param_name: name for parameter to request.\n"
			":param value: value to set." )

		MAP_METHOD_AND_WRAP(
			"GetParameterByName",
			GetParameterByName,
			"GetParameterByName( parameter_node_name, param_name )\n\n"
			"Returns the value of a parameter specified by name on a param node\n"
			":param parameter_node_name: name for parameter node\n"
			":param param_name: name for parameter\n" )

		MAP_METHOD_AND_WRAP(
			"GetParameterIndexByName",
			GetParameterIndexByName,
			"GetParameterIndexByName( parameter_node_name, param_name )\n\n"
			"Returns the integer index of a parameter name on a param node\n"
			":param parameter_node_name: name for parameter node\n"
			":param param_name: name for parameter" )

		MAP_METHOD_AND_WRAP(
			"IsFullyLoaded",
			IsFullyLoaded,
			"IsFullyLoaded()\n\n"
			"Returns boolean indicating whether the gState instance and all dependent data is fully loaded." )

		MAP_METHOD_AND_WRAP(
			"InstantiateCharacter",
			InstantiateCharacter,
			"InstantiateCharacter()\n\n"
			"Used to bind animation once it is fully loaded." )

		MAP_METHOD_AND_WRAP(
			"ResetParamsToDefault",
			ResetParamsToDefault,
			"ResetParamsToDefault()\n\n"
			"Resets all parameters to initial values." )

		MAP_METHOD_AND_WRAP(
			"GetAnimationTime",
			GetAnimationTime,
			"GetAnimationTime()\n\n"
			"Returns float indicating current GState animation time" )

		MAP_METHOD_AND_WRAP(
			"StartScrub",
			StartScrub,
			"StartScrub()\n\n"
			"Enables scrub mode and sets scrub offset to 0" )

		MAP_METHOD_AND_WRAP(
			"ClearScrub",
			ClearScrub,
			"ClearScrub()\n\n"
			"Exits scrub mode" )

		MAP_METHOD_AND_WRAP(
			"PlayFromScrub",
			PlayFromScrub,
			"PlayFromScrub()\n\n"
			"When scrubbing mode enabled, plays from scrub time." )

		MAP_METHOD_AND_WRAP(
			"StopPlayFromScrub",
			StopPlayFromScrub,
			"StopPlayFromScrub()\n\n"
			"When scrubbing mode enabled, stops playing from scrub time." )

		MAP_METHOD_AND_WRAP(
			"GetScrubOffset",
			GetScrubOffset,
			"GetScrubOffset()\n\n"
			"When scrubbing mode enabled, returns the scrub offset as a float" )

		MAP_METHOD_AND_WRAP(
			"SetScrubOffset",
			SetScrubOffset,
			"SetScrubOffset(float scrub_offset)\n\n"
			"When scrubbing mode enabled, sets scrub offset to the float value passed in (in ms)" )

	EXPOSURE_END()
}

#endif