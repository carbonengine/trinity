// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveVirtualCameraBehaviour.h"

// =============================================================================
// Float Behaviour Interface / Base
// =============================================================================

BLUE_DEFINE_ABSTRACT( EveVirtualCameraBehaviourFloatBase );

const Be::ClassInfo* EveVirtualCameraBehaviourFloatBase::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourFloatBase, "" )
		MAP_INTERFACE( INotify )
			MAP_ATTRIBUTE( "name", m_name, "The name.", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
				MAP_ATTRIBUTE( "active", m_active, "If not active the node is disabled.", Be::READWRITE | Be::PERSIST )
					EXPOSURE_END()
}

// =============================================================================
// Set
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourFloatSet );

const Be::ClassInfo* EveVirtualCameraBehaviourFloatSet::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourFloatSet, "Lerp from one position to another over time." )
		MAP_INTERFACE( INotify )
			MAP_ATTRIBUTE( "value", m_value, "The value to set.", Be::READWRITE | Be::PERSIST )
				EXPOSURE_CHAINTO( EveVirtualCameraBehaviourFloatBase )
}

// =============================================================================
// Add
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourFloatAdd );

const Be::ClassInfo* EveVirtualCameraBehaviourFloatAdd::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourFloatAdd, "Lerp from one position to another over time." )
		MAP_INTERFACE( INotify )
			MAP_ATTRIBUTE( "value", m_value, "The value to set.", Be::READWRITE | Be::PERSIST )
				MAP_ATTRIBUTE( "scaleCurve", m_scaleCurve, "A curve to dictate how to scale the value over the lifetime of the caemra.\nIf not used the value is absolute.\nShould typically range from 0-1 in value and time.", Be::READWRITE | Be::PERSIST )
					EXPOSURE_CHAINTO( EveVirtualCameraBehaviourFloatBase )
}

// =============================================================================
// Noise
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourFloatNoise );

const Be::ClassInfo* EveVirtualCameraBehaviourFloatNoise::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourFloatNoise, "Shakes around the position / point of interest. Can be scaled by view when applied to the point of interest so it acts as a rotations shake." )
		MAP_INTERFACE( INotify )
			MAP_ATTRIBUTE( "perlineScale", m_frequency, "Scales the perlin noise used. Higher values means higher frequency.", Be::READWRITE | Be::PERSIST )
				MAP_ATTRIBUTE( "octaves", m_octaves, "Determines the detail of the noise. Higher values give more detailed noise.\n:jessica-numeric-range: (1, 16)\n", Be::READWRITE | Be::PERSIST )
					MAP_ATTRIBUTE( "magnitude", m_magnitude, "Scales the overall noise effect.", Be::READWRITE | Be::PERSIST )
						MAP_ATTRIBUTE( "magnitudeCurve", m_magnitudeCurve, "If false, the offset is in meters,\nif true the offset is scaled by the anchor object collision sphere radius.", Be::READWRITE | Be::PERSIST )
							EXPOSURE_CHAINTO( EveVirtualCameraBehaviourFloatBase )
}

// =============================================================================
// Noise
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourFloatDamping );

const Be::ClassInfo* EveVirtualCameraBehaviourFloatDamping::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourFloatDamping, "Shakes around the position / point of interest. Can be scaled by view when applied to the point of interest so it acts as a rotations shake." )
		MAP_INTERFACE( INotify )
			MAP_ATTRIBUTE( "dampingFactor", m_dampingRatio, "Scales the perlin noise used. Higher values means higher frequency.", Be::READWRITE | Be::PERSIST )
				EXPOSURE_CHAINTO( EveVirtualCameraBehaviourFloatBase )
}

// =============================================================================
// Vector3 Behaviour Interface / Base
// =============================================================================

BLUE_DEFINE_ABSTRACT( EveVirtualCameraBehaviourVector3Base );

const Be::ClassInfo* EveVirtualCameraBehaviourVector3Base::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourVector3Base, "" )
		MAP_INTERFACE( INotify )
			MAP_ATTRIBUTE( "name", m_name, "The name.", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
				MAP_ATTRIBUTE( "active", m_active, "If not active the node is disabled.", Be::READWRITE | Be::PERSIST )
					EXPOSURE_END()
}

// =============================================================================
// Move Between / Lerp
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourVector3MoveBetween );

const Be::ClassInfo* EveVirtualCameraBehaviourVector3MoveBetween::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourVector3MoveBetween, "Lerp from one position to another over time." )
		MAP_INTERFACE( INotify )
			MAP_ATTRIBUTE( "start", m_start, "Start position.", Be::READWRITE | Be::PERSIST )
				MAP_ATTRIBUTE( "end", m_end, "End position.", Be::READWRITE | Be::PERSIST )
					MAP_ATTRIBUTE( "interpolationCurve", m_interpolationCurve, "Optional curve to define how to lerp from start to end.\nShould typically range from 0-1 in value and time.", Be::READWRITE | Be::PERSIST )
						MAP_ATTRIBUTE( "proportional", m_proportional, "If false, the offset is in meters,\nif true the offset is scaled by the anchor object collision sphere radius.", Be::READWRITE | Be::PERSIST )
							MAP_ATTRIBUTE( "world", m_world, "If true the offset is in world coordinates,\notherwise it's in local coordinates (i.e. relative to the anchors).", Be::READWRITE | Be::PERSIST )
								EXPOSURE_CHAINTO( EveVirtualCameraBehaviourVector3Base )
}

// =============================================================================
// Offset
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourVector3Offset );

const Be::ClassInfo* EveVirtualCameraBehaviourVector3Offset::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourVector3Offset, "Lerp from one position to another over time." )
		MAP_INTERFACE( INotify )
			MAP_ATTRIBUTE( "offset", m_offset, "Offset from the anchor center (or origin if no anchor objects are supplied).", Be::READWRITE | Be::PERSIST )
				MAP_ATTRIBUTE( "proportional", m_proportional, "If false, the offset is in meters,\nif true the offset is scaled by the anchor object collision sphere radius.", Be::READWRITE | Be::PERSIST )
					MAP_ATTRIBUTE( "world", m_world, "If true the offset is in world coordinates,\notherwise it's in local coordinates (i.e. relative to the anchors).", Be::READWRITE | Be::PERSIST )
						EXPOSURE_CHAINTO( EveVirtualCameraBehaviourVector3Base )
}

// =============================================================================
// Orbit
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourVector3Orbit );

const Be::ClassInfo* EveVirtualCameraBehaviourVector3Orbit::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourVector3Orbit, "Lerp from one position to another over time." )
		MAP_INTERFACE( INotify )
			MAP_ATTRIBUTE( "start", m_start, "Starting orbit position (in degrees) where 0 is in front\nof the anchor objects and 180 is behind.", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
				MAP_ATTRIBUTE( "end", m_end, "End orbit position (in degrees) where 0 is in front\nof the anchor objects and 180 is behind.", Be::READWRITE | Be::PERSIST )
					MAP_ATTRIBUTE( "distance", m_distance, "The range to orbit the object.\nEither in metres, km or scaled by the bounding\nsphere of the anchor obejcts. See proportional.", Be::READWRITE | Be::PERSIST )
						MAP_ATTRIBUTE( "distanceScalarCurve", m_distanceScalarCurve, "A curve to scale the range with.\nShould typically range from 0-1 in value and time.", Be::READWRITE | Be::PERSIST )
							MAP_ATTRIBUTE( "orbitCurve", m_orbitCurve, "A curve to dictate how to lerp from start to end.\nWhen not specified interpolation is simply linear.\nShould typically range from 0-1 in value and time.", Be::READWRITE | Be::PERSIST )
								MAP_ATTRIBUTE( "proportional", m_proportional, "If false, the offset is in meters,\nif true the offset is scaled by the anchor object collision sphere radius.", Be::READWRITE | Be::PERSIST )
									MAP_ATTRIBUTE( "world", m_world, "If true the orbit angle is an angle from the world forward direction (0, 0, 1),\notherwise it's an angle from the forward direction of the anchors.", Be::READWRITE | Be::PERSIST )
										EXPOSURE_CHAINTO( EveVirtualCameraBehaviourVector3Base )
}

// =============================================================================
// Move Foward / Dolly
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourVector3MoveForward );

const Be::ClassInfo* EveVirtualCameraBehaviourVector3MoveForward::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourVector3MoveForward, "Move the camera forward (a.k.a. dolly)." )
		MAP_INTERFACE( INotify )
			MAP_ATTRIBUTE( "value", m_value, "The amount to move in by.", Be::READWRITE | Be::PERSIST )
				MAP_ATTRIBUTE( "scaleCurve", m_scaleCurve, "A curve to dictate how to scale the value over the lifetime of the caemra.\nIf not used the value is absolute.\nShould typically range from 0-1 in value and time.", Be::READWRITE | Be::PERSIST )
					MAP_ATTRIBUTE( "proportional", m_proportional, "If false, the value is in meters,\nif true the value is scaled by the anchor object collision sphere radius.", Be::READWRITE | Be::PERSIST )
						EXPOSURE_CHAINTO( EveVirtualCameraBehaviourVector3Base )
}

// =============================================================================
// Move Right / Truck
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourVector3MoveRight );

const Be::ClassInfo* EveVirtualCameraBehaviourVector3MoveRight::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourVector3MoveRight, "Move the camera right (a.k.a. truck)." )
		MAP_INTERFACE( INotify )
			EXPOSURE_CHAINTO( EveVirtualCameraBehaviourVector3MoveForward )
}

// =============================================================================
// Move Up / Pedestal
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourVector3MoveUp );

const Be::ClassInfo* EveVirtualCameraBehaviourVector3MoveUp::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourVector3MoveUp, "Move the camera up (a.k.a. pedestal)." )
		MAP_INTERFACE( INotify )
			EXPOSURE_CHAINTO( EveVirtualCameraBehaviourVector3MoveForward )
}

// =============================================================================
// Shake
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourVector3Shake );

const Be::ClassInfo* EveVirtualCameraBehaviourVector3Shake::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourVector3Shake, "Shakes around the position / point of interest. Can be scaled by view when applied to the point of interest so it acts as a rotations shake." )
		MAP_INTERFACE( INotify )
			MAP_ATTRIBUTE( "perlineScale", m_frequency, "Scales the perlin noise used. Higher values means higher frequency.", Be::READWRITE | Be::PERSIST )
				MAP_ATTRIBUTE( "octaves", m_octaves, "Determines the detail of the noise. Higher values give more detailed noise.\n:jessica-numeric-range: (1, 16)\n", Be::READWRITE | Be::PERSIST )
					MAP_ATTRIBUTE( "magnitude", m_magnitude, "Scales the overall noise effect.", Be::READWRITE | Be::PERSIST )
						MAP_ATTRIBUTE( "magnitudeCurve", m_magnitudeCurve, "If false, the offset is in meters,\nif true the offset is scaled by the anchor object collision sphere radius.", Be::READWRITE | Be::PERSIST )
							MAP_ATTRIBUTE( "scaleByView", m_scaleByView, "Typically on. Scales the overall shake relative to the distance from the camera to the point of interest so the shake feels consistent no matter the \"zoom\" level.", Be::READWRITE | Be::PERSIST )
								EXPOSURE_CHAINTO( EveVirtualCameraBehaviourVector3Base )
}

// =============================================================================
// Damping
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourVector3Damping );

const Be::ClassInfo* EveVirtualCameraBehaviourVector3Damping::ExposeToBlue(){
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourVector3Damping, "Adds a damping factor to the velocity of behaviours that come before it in the stack, smoothing out sudden changes.\n" )
		MAP_INTERFACE( INotify )
			MAP_ATTRIBUTE( "m_dampingRatio", m_dampingRatio, "Scalar from 0-1 to scale down the velocity of the camera.\nA value of 1 has no effect.\n:jessica-numeric-range: (0.0, 1.0)\n", Be::READWRITE | Be::PERSIST )
				EXPOSURE_CHAINTO( EveVirtualCameraBehaviourVector3Base )
}

// =============================================================================
// Inertia
// =============================================================================

BLUE_DEFINE( EveVirtualCameraBehaviourVector3Inertia );

const Be::ClassInfo* EveVirtualCameraBehaviourVector3Inertia::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveVirtualCameraBehaviourVector3Inertia, "Adds a damping factor to the behaviours that come before it in the stack, adding weight to the camera and smoothing out velocity spikes.\nAdd to the end of the stack to give the camera inertia." )
		MAP_INTERFACE( INotify )
		MAP_ATTRIBUTE(
			"inertiaFactor",
			m_inertiaFactor,
			"Adds a sense of inertia. Values > 1 are \"underdamped\" and give a spring like inertia.\n"
			"A value of 1 is \"critically damped\" which has no real effect.\n"
			"A value < 1 would result in explosive behaviour and is undesirable\n"
			":jessica-numeric-range: (1.0, 1000.0)\n",
			Be::READWRITE | Be::PERSIST )
	EXPOSURE_CHAINTO( EveVirtualCameraBehaviourVector3Base )
}
