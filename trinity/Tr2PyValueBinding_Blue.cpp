#include "StdAfx.h"

#if BLUE_WITH_PYTHON

#include "Tr2PyValueBinding.h"

BLUE_DEFINE( Tr2PyValueBinding );

const Be::ClassInfo* Tr2PyValueBinding::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2PyValueBinding,"" )
        MAP_INTERFACE( Tr2PyValueBinding )
		MAP_INTERFACE( ITr2ValueBinding )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE
		( 
			"name", 
			m_name, 
			"Name of this value binding.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"isValid",
			m_isValid,
			"True if this binding is valid, i.e. source object, source attribute\n"
			"destination object and destination attribute are all set.",
			Be::READ
		)
		MAP_ATTRIBUTE
		( 
			"sourceObject", 
			m_sourceObject, 
			"Object providing the source value.", 
			Be::READWRITE | Be::NOTIFY
		)
		MAP_ATTRIBUTE
		( 
			"sourceAttribute", 
			m_sourceAttribute, 
			"Attribute of the source object. The value of this is copied to the"
			"named attribute of the destination object",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"destinationObject",
			m_destinationObject,
			"Destination object.",
			Be::READWRITE | Be::NOTIFY
		)
		MAP_ATTRIBUTE
		( 
			"destinationAttribute",
			m_destinationAttribute,
			"Attribute of the destination object. The value coming from the"
			"source object is copied to here",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

    EXPOSURE_END()
}

#endif
