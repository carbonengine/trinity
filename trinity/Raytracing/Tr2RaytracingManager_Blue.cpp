// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2RaytracingManager.h"

BLUE_DEFINE( Tr2RaytracingManager );

const Be::ClassInfo* Tr2RaytracingManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2RaytracingManager, "" )
		MAP_INTERFACE( Tr2RaytracingManager )

		MAP_ATTRIBUTE( "shadowEffect", m_shadowEffect, "", Be::READ )
		MAP_ATTRIBUTE( "sunAngle", m_sunAngle, "penumbra for shadows", Be::READWRITE )
		MAP_ATTRIBUTE( "applyDenoiser", m_applyDenoiser, "apply denoiser or not", Be::READWRITE )
		MAP_ATTRIBUTE( "denoiser", m_denoiser, "", Be::READ )

	EXPOSURE_END()
}