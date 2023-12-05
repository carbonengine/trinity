////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2FidelityFxCas.h"

BLUE_DEFINE( Tr2FidelityFxCas );

const Be::ClassInfo* Tr2FidelityFxCas::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2FidelityFxCas, "" )
		MAP_INTERFACE( Tr2FidelityFxCas )
		MAP_INTERFACE( ITr2Sharpening )

		MAP_ATTRIBUTE( "shader", m_shader, "The Fidelity CAS Shader", Be::READWRITE )

	EXPOSURE_END( )
}
