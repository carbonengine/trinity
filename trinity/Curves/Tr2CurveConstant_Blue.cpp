////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2CurveConstant.h"

BLUE_DEFINE( Tr2CurveConstant );

const Be::ClassInfo* Tr2CurveConstant::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CurveConstant, "" )
		MAP_INTERFACE( Tr2CurveConstant )
		MAP_INTERFACE( ITriScalarFunction )
		MAP_INTERFACE( ITriVectorFunction )
		MAP_INTERFACE( ITriQuaternionFunction )
		MAP_INTERFACE( ITriColorFunction )
		{
			Be::InterfaceEntry entry = { &GetITriFunctionIID(), BLUE_INTERFACEOFFSET( ITriScalarFunction ) };
			s_interfaces.push_back( entry ); 
		}

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"currentValue",
			m_value,
			"Curve value after the last update",
			Be::READ )

		MAP_ATTRIBUTE(
			"value",
			m_value,
			"",
			Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}