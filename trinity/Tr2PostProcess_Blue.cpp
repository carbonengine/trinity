// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2PostProcess.h"

BLUE_DEFINE( Tr2PostProcess );

const Be::ClassInfo* Tr2PostProcess::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PostProcess, "" )

		MAP_INTERFACE( Tr2PostProcess )

		MAP_ATTRIBUTE(
			"stages",
			m_stages,
			"Post processing stages",
			Be::READ | Be::PERSIST )

	EXPOSURE_END()
}
