#include "StdAfx.h"
#include "TriCurveSet.h"

BLUE_DEFINE( TriCurveSet );

const Be::ClassInfo* TriCurveSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( TriCurveSet,"" )
        MAP_INTERFACE( TriCurveSet )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2Updateable )

		MAP_ATTRIBUTE
		(
			"name",
			m_name,
			"Name of this curve set",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"curves", 
			m_curves, 
			"List of curves. Bind these curves to target objects with a binding.", 
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"bindings", 
			m_bindings, 
			"List of bindings. Note that a single curve can have multiple bindings.", 
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"scale",
			m_scale,
			"Scaling factor applied to time when curves are playing.\n"
			"By default this set to 1.0 so the curves play at regular speed.\n"
			"Values higher than 1.0 make the curves play faster, lower than 1.0\n"
			"make them play slower.\n"
			"Negative values make the curves play backwards.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"playOnLoad",
			m_playOnLoad,
			"If set, the curve set plays automatically on load. Otherwise Play must be"
			"called explicitly to start it.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"isPlaying",
			m_isPlaying,
			"True if the curve set is playing",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"scaledTime",
			m_scaledTime,
			"This is the time value passed into the curves when sampling their values.\n"
			"When the curve is playing, this value is updated every tick according\n"
			"to the time passed, adjusted by the 'scale' factor.\n"
			"Note that this value can be changed at any time, making the curves\n"
			"jump to that point in time. To have absolute control over this time\n"
			"value, set the scale factor to 0.\n",
			Be::READWRITE
		)
		MAP_ATTRIBUTE
		(
			"useSimTimeRebase",
			m_useSimTimeRebase,
			"Should internal time be rebased when sim time is rebased. Will only work if loaded with this attribute set.",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"useRealTime",
			m_useRealTime,
			"If set, curves are updated based on real time, rather than sim time (the default). This means\n"
			"they will not be affected by time dilation and should generally only be used for UI animations.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_METHOD_AND_WRAP
		(
			"Update",
			UpdateWithCurrentTime,
			"Updates the curve set with the current frame time."
		)

		MAP_METHOD_AND_WRAP( 
			"Play", 
			Play, 
			"Play this curve set from the start\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/play.png"
			)
		MAP_METHOD_AND_WRAP( "PlayFrom", PlayFrom, "Play this curve set from the given time (in seconds)" )
		MAP_METHOD_AND_WRAP( 
			"Stop", 
			Stop, 
			"Stop the playback of this curveset\n" 
			":jessica-favorite:\n"
			":jessica-icon: timeline/stop.png"
			)
		MAP_METHOD_AND_WRAP( "StopOnNextFrame", StopOnNextFrame, "Stop the playback of this curveset on the next frame" )
		MAP_METHOD_AND_WRAP( "StopAfter", StopAfter, "Stop the playback of this curveset after the given number of seconds" )
		MAP_METHOD_AND_WRAP( "GetMaxCurveDuration", GetMaxCurveDuration, "Gets the duration of the longest non-cycling curve" )

		MAP_METHOD_AND_WRAP
		(
			"StopAfterWithCallback",
			StopAfterWithCallback,
			"Stop the playback of this curveset after the given number of seconds, issuing a callback to Python when the playback stops"
		)

    EXPOSURE_END()
}