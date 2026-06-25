// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "Tr2TimelineController.h"


BLUE_DEFINE( Tr2TimelineController );


const Be::ClassInfo* Tr2TimelineController::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TimelineController, "" )
		MAP_INTERFACE( Tr2TimelineController )
		MAP_INTERFACE( ITr2Controller )
		MAP_INTERFACE( ITr2ActionController )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"variables",
			m_variables,
			"List of controller variables",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"eventHandlers",
			m_eventHandlers,
			"List of event handlers that are fired when HandleEvent is called",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"isPlaying",
			m_isActive,
			"True if the timeline is playing or paused (i.e. Start has been called)",
			Be::READ )
		MAP_ATTRIBUTE(
			"isPaused",
			m_isPaused,
			"When it is True the timeline does not avance time while playing",
			Be::READ )
		MAP_PROPERTY(
			"time",
			GetTime,
			SetTime,
			"Current play time from the start of the timeline in seconds" )
		MAP_ATTRIBUTE(
			"timeScale",
			m_timeScale,
			"Scale for time when playing to speed up or slow down the playback",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "actions", m_actions, "", Be::PERSISTONLY )
		MAP_ATTRIBUTE( "entries", m_entries, "", Be::PERSISTONLY )

		MAP_METHOD_AND_WRAP(
			"Start",
			Start,
			"Starts the controller\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/play.png" )
		MAP_METHOD_AND_WRAP(
			"Stop",
			Stop,
			"Stops the controller\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/stop.png" )
		MAP_METHOD_AND_WRAP(
			"Pause",
			Pause,
			"Pauses the playback of the timeline until Resume is called" )
		MAP_METHOD_AND_WRAP(
			"Resume",
			Resume,
			"Resumes the playback of the timeline after a call to Pause" )
		MAP_METHOD_AND_WRAP(
			"HandleEvent",
			HandleEvent,
			"Handle the specified event\n"
			":param name: event name" )
		MAP_METHOD_AND_WRAP(
			"GetOwner",
			GetOwner,
			"Returns controller owner" )
		MAP_METHOD_AND_WRAP(
			"RegisterCallback",
			RegisterCallback,
			"Registers a callback under a specific name\n"
			":param callbackName: the name of the callback\n"
			":param callback: A python function that accepts no arguments" )
		MAP_METHOD_AND_WRAP(
			"ClearCallbacks",
			ClearCallbacks,
			"Clears all callbacks" )
		MAP_METHOD_AND_WRAP(
			"ReLink",
			ReLink,
			"Re-links the controller with the assigned owner. Used by tools only." )

		MAP_METHOD_AND_WRAP(
			"GetActionCount",
			GetActionCount,
			"Returns the number of actions in the timeline" )
		MAP_METHOD_AND_WRAP(
			"GetAction",
			GetAction,
			"Returns the action by index. Raises ValueError if the index is out of bounds.\n\n"
			":param idx: index of the action" )
		MAP_METHOD_AND_WRAP(
			"GetActionStartTime",
			GetActionStartTime,
			"Returns the start time for the action. Raises ValueError if the action index is out of bounds.\n\n"
			":param idx: index of the action" )
		MAP_METHOD_AND_WRAP(
			"GetActionEndTime",
			GetActionEndTime,
			"Returns the end time for the action. Raises ValueError if the action index is out of bounds.\n\n"
			":param idx: index of the action" )
		MAP_METHOD_AND_WRAP(
			"GetActionTrackID",
			GetActionTrackID,
			"Returns the track ID for the action. Raises ValueError if the action index is out of bounds.\n\n"
			":param idx: index of the action" )
		MAP_METHOD_AND_WRAP(
			"SetActionStartTime",
			SetActionStartTime,
			"Sets the start time for the action. Raises ValueError if the action index is out of bounds.\n\n"
			":param idx: index of the action\n"
			":param time: start time for the action" )
		MAP_METHOD_AND_WRAP(
			"SetActionEndTime",
			SetActionEndTime,
			"Sets the end time for the action. Raises ValueError if the action index is out of bounds.\n\n"
			":param idx: index of the action\n"
			":param time: end time for the action" )
		MAP_METHOD_AND_WRAP(
			"SetActionTrackID",
			SetActionTrackID,
			"Sets the track ID for the action. Raises ValueError if the action index is out of bounds.\n\n"
			":param idx: index of the action\n"
			":param trackID: track ID for the action" )
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"AddAction",
			AddAction,
			1,
			"Adds a new action to the timeline.\n\n"
			":param action: new action (trinity.ITr2ControllerAction) to add to the timeline\n"
			":param startTime: start time for the action\n"
			":param endTime: end time for the action\n"
			":param trackID: track ID for the action (optional, defaults to zero)" )
		MAP_METHOD_AND_WRAP(
			"RemoveAction",
			RemoveAction,
			"Removes the action by index. Raises ValueError if the index is out of bounds.\n\n"
			":param idx: index of the action" )

		MAP_METHOD_AND_WRAP(
			"IsTrackEnabled",
			IsTrackEnabled,
			"Returns True if the track with the given track ID has not been disabled with the call to EnableTrack.\n"
			"Tracks are enabled by default, but can be disabled with the call to EnableTrack. Actions with the\n"
			"disabled track ID and not played (i.e. Start/Stop events are not sent to these actions). Track\n"
			"enablement is not persisted with the timeline and is for tools only.\n\n"
			":param trackID: ID of the track" )
		MAP_METHOD_AND_WRAP(
			"EnableTrack",
			EnableTrack,
			"Enables/disables track with the given ID. See IsTrackEnabled.\n\n"
			":param trackID: ID of the track\n"
			":param enable: True of False to enable or disable the track" )

	EXPOSURE_CHAINTO( EveThrottleable )
}
