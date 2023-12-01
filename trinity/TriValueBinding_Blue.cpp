#include "StdAfx.h"
#include "TriValueBinding.h"

BLUE_DEFINE( TriValueBinding );
BLUE_DEFINE_INTERFACE( ITriReroutable );
BLUE_DEFINE_INTERFACE( ITr2ValueBinding );

const Be::ClassInfo* TriValueBinding::ExposeToBlue()
{
    EXPOSURE_BEGIN( TriValueBinding,"" )
        MAP_INTERFACE( TriValueBinding )
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
			"destination object and destination attribute are all valid and if the\n"
			"source and destination are of compatible types.",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"isEnabled",
			m_isEnabled,
			"If false the binding is disabled and will not update the destination\n"
			"attribute with the source attribute.\n",
			Be::READWRITE
		)
		MAP_ATTRIBUTE
		( 
			"sourceObject", 
			m_sourceObject, 
			"Object providing the source value.", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)
		MAP_ATTRIBUTE
		( 
			"sourceAttribute", 
			m_sourceAttribute, 
			"Attribute of the source object. The value of this is copied to the"
			"named attribute of the destination object",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)
		MAP_ATTRIBUTE
		( 
			"destinationObject",
			m_destinationObject,
			"Destination object.",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)
		MAP_ATTRIBUTE
		( 
			"destinationAttribute",
			m_destinationAttribute,
			"Attribute of the destination object. The value coming from the"
			"source object is copied to here",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)
		MAP_ATTRIBUTE
		(
			"scale",
			m_scale,
			"Scale of source value before being applied to destination",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"offset",
			m_offset,
			"Offset of source value, added to it after the scaling, before being applied to destination",
			Be::READWRITE | Be::PERSIST
		)
#if BLUE_WITH_PYTHON
		MAP_ATTRIBUTE
		(
			"copyValueCallable",
			m_copyValueCallable,
			"Python callable to handle CopyValue.",
			Be::READWRITE | Be::NOTIFY
		)
#endif

    EXPOSURE_END()
}
