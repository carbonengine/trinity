#include "StdAfx.h"
#include "TriEventCurve.h"
#include "TriConstants.h"

BLUE_DEFINE( TriEventCurve );

const Be::ClassInfo* TriEventCurve::ExposeToBlue()
{
    EXPOSURE_BEGIN( TriEventCurve, ":jessica-deprecated: True" )
        MAP_INTERFACE( TriEventCurve )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITriCurveLength )

		MAP_ATTRIBUTE
		(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"time",
			m_time,
			"Current time of the curve",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"length",
			m_length,
			"Length of the curve",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"localTime",
			m_localTime,
			"Local time - reset to 0 when curve cycles.",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"value",
			m_value,
			"The string value of the last key triggered.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(  
			"keys",          
			m_keys,          
			"These are the keys of the curve", 
			Be::PERSISTONLY
		)
		MAP_ATTRIBUTE_WITH_CHOOSER
		(   
			"extrapolation", 
			m_extrapolation, 
			"This controls how time values outside the range of the curve are handled", 
			Be::READWRITE | Be::PERSIST | Be::ENUM, 
			TriExtrapolation
		)
		MAP_ATTRIBUTE
		(
			"eventListener",
			m_eventListener,
			"",
			Be::READWRITE
		)

		MAP_METHOD_AND_WRAP
		( 
			"AddKey", 
			AddKey, 
			"Adds a key with a string value. This string value is passed to\n"
			"the handler when the key is triggered.\n"
			":param time: key time\n"
			":param evtName: event name"
		)

#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP
		( 
			"AddCallableKey", 
			AddCallableKey, 
			"Adds a key with a callable. This callable is called when the key\n"
			"is triggered.\n"
			":param time: key time\n"
			":param cb: callback callable\n"
			":param args: arguments for the cb"
		)
#endif

		MAP_METHOD_AND_WRAP( 
			"RemoveKey", 
			RemoveKey, 
			"Removes a key\n"
			":param idx: key index" );
		MAP_METHOD_AND_WRAP( "GetKeyCount", GetKeyCount, "Returns number of keys" );
		MAP_METHOD_AND_WRAP( 
			"GetKeyValue", 
			GetKeyValue, 
			"Returns key value\n"
			":param idx: key index" );
		MAP_METHOD_AND_WRAP( 
			"GetKeyTime", 
			GetKeyTime, 
			"Returns key time\n"
			":param idx: key index" );
		MAP_METHOD_AND_WRAP( 
			"SetKeyValue", 
			SetKeyValue, 
			"Changes key value\n"
			":param idx: key index\n"
			":param value: new key value" );
		MAP_METHOD_AND_WRAP( 
			"SetKeyTime", 
			SetKeyTime, 
			"Changes key time.\n"
			":param idx: key index\n"
			":param time: new key time" );
#if BLUE_WITH_PYTHON
        MAP_METHOD_AND_WRAP( 
			"GetCallableKeyValue", 
			GetCallableKeyValue, 
			"Returns callable associated with the key\n"
			":param idx: key index" );
        MAP_METHOD_AND_WRAP( 
			"GetCallableKeyArgs", 
			GetCallableKeyArgs, 
			"Returns callable arguments for the key\n"
			":param idx: key index" );
#endif

    EXPOSURE_END()
}
