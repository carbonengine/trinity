// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriValueBinding.h"

BLUE_DEFINE( TriValueBinding );
BLUE_DEFINE_INTERFACE( ITriReroutable );
BLUE_DEFINE_INTERFACE( ITr2ValueBinding );

const Be::ClassInfo* TriValueBinding::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriValueBinding, "" )
		MAP_INTERFACE( TriValueBinding )
		MAP_INTERFACE( ITr2ValueBinding )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"Name of this value binding.",
			Be::READWRITE | Be::PERSIST )
		MAP_PROPERTY_READONLY(
			"isValid",
			IsValid,
			"True if this binding is valid, i.e. source object, source attribute\n"
			"destination object and destination attribute are all valid and if the\n"
			"source and destination are of compatible types." )
		MAP_ATTRIBUTE(
			"isWeak",
			m_isWeak,
			"True if this binding was set up to have weak references to the source and\n"
			"destination objects. Can only be set up for specific bindings in C++.",
			Be::READ )
		MAP_ATTRIBUTE(
			"isEnabled",
			m_isEnabled,
			"If false the binding is disabled and will not update the destination\n"
			"attribute with the source attribute.\n",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"sourceObject",
			m_sourceObject,
			"Object providing the source value.",
			Be::PERSISTONLY )
		MAP_PROPERTY(
			"sourceObject",
			GetSourceObject,
			SetSourceObject,
			"Object providing the source value." )
		MAP_ATTRIBUTE(
			"sourceAttribute",
			m_sourceAttribute,
			"Attribute of the source object. The value of this is copied to the"
			"named attribute of the destination object",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"destinationObject",
			m_destinationObject,
			"Destination object.",
			Be::PERSISTONLY )
		MAP_PROPERTY(
			"destinationObject",
			GetDestinationObject,
			SetDestinationObject,
			"Destination object." )
		MAP_ATTRIBUTE(
			"destinationAttribute",
			m_destinationAttribute,
			"Attribute of the destination object. The value coming from the"
			"source object is copied to here",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"scale",
			m_scale,
			"Scale of source value before being applied to destination",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"offset",
			m_offset,
			"Offset of source value, added to it after the scaling, before being applied to destination",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"timeOffset",
			m_timeOffset,
			"Time offset, in seconds, applied to the source curve of this binding.\n"
			"The curve is sampled at (curve set time - timeOffset), so a positive\n"
			"value makes this binding lag behind the curve set, a negative one makes\n"
			"it run ahead. This lets several bindings drive different targets from\n"
			"the same curve at different times, without duplicating the curve. The\n"
			"curve itself is not moved, so it can be shared freely. A looped time\n"
			"range on the curve set wraps the offset time into the range.\n"
			"Only applies when the source is the currentValue of a scalar, vector,\n"
			"quaternion or color curve - any other source logs a warning and is\n"
			"copied as it stands - and only when a curve set applies the binding.\n"
			"Bindings run without a curve set time (lens flares, child links, socket\n"
			"parameters, dynamic bindings) copy the curve's current value and\n"
			"silently ignore the offset.",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
#if BLUE_WITH_PYTHON
		MAP_ATTRIBUTE(
			"copyValueCallable",
			m_copyValueCallable,
			"Python callable to handle CopyValue.",
			Be::READWRITE | Be::NOTIFY )
#endif

	EXPOSURE_END()
}
