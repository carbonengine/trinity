// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveSpaceObjectChild.h"

BLUE_DEFINE_ABSTRACT( EveSpaceObjectChild );

const Be::ClassInfo* EveSpaceObjectChild::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSpaceObjectChild, "" )
		MAP_INTERFACE( EveSpaceObjectChild )

		MAP_ATTRIBUTE( "name", m_name, "Name of the space object child", Be::READWRITE | Be::PERSIST )
		MAP_PROPERTY_READONLY( "partTag", GetPartTag, "Part tag for multi-part space objects" )
		MAP_METHOD_AND_WRAP( "GetParent", GetParent, "Returns the parent space object child in the hierarchy" )
		MAP_METHOD_AND_WRAP( "GetOwner", GetOwner, "Returns the owner space object" )

	EXPOSURE_END()
}
