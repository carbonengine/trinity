// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriEventKey.h"

BLUE_DEFINE( TriEventKey );

const Be::ClassInfo* TriEventKey::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriEventKey, "" )
		MAP_INTERFACE( TriEventKey )

		MAP_ATTRIBUTE(
			"time",
			m_time,
			"Time value for this key",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"value",
			m_value,
			"String associated with this key. This is passed to the event curve\n"
			"handler in the case where no callable is set.",
			Be::READWRITE | Be::PERSIST )

#if BLUE_WITH_PYTHON
		MAP_ATTRIBUTE(
			"callable",
			m_callable,
			"A Python callable, called when the event triggers.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"callableArgs",
			m_callableArgs,
			"Arguments passed to the Python callable, called when the event triggers.",
			Be::READWRITE )
#endif

	EXPOSURE_END()
}
