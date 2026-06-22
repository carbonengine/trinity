// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2ExpressionTermInfo.h"


BLUE_DEFINE( Tr2ExpressionTermInfo );


const Be::ClassInfo* Tr2ExpressionTermInfo::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ExpressionTermInfo, "" )
		MAP_ATTRIBUTE( "type", m_type, "", Be::READWRITE )
		MAP_ATTRIBUTE( "category", m_category, "", Be::READWRITE )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE )
		MAP_ATTRIBUTE( "description", m_description, "", Be::READWRITE )

		MAP_METHOD_AND_WRAP( "GetArguments", GetArguments, "" )
	EXPOSURE_END()
}
