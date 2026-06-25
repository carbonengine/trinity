// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2ShaderBuffer.h"

BLUE_DEFINE( Tr2ShaderBuffer );

const Be::ClassInfo* Tr2ShaderBuffer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ShaderBuffer, "" )
		MAP_INTERFACE( Tr2ShaderBuffer )

		MAP_ATTRIBUTE(
			"size",
			m_size,
			"",
			Be::READ )

		MAP_METHOD_AND_WRAP(
			"SetData",
			SetDataFromScript,
			":param data: data string\n"
			":param size: size of the data" )

	EXPOSURE_END()
}