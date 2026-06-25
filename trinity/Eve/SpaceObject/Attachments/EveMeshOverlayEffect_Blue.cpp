// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveMeshOverlayEffect.h"

#include "Shader/Tr2Effect.h"
#include "Curves/TriCurveSet.h"

BLUE_DEFINE( EveMeshOverlayEffect );

const Be::ClassInfo* EveMeshOverlayEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveMeshOverlayEffect, "" )
		MAP_INTERFACE( EveMeshOverlayEffect )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( ITr2ControllerOwner )
		MAP_INTERFACE( ITr2CurveSetOwner )

		MAP_ATTRIBUTE( "display", m_display, "Toggle visibility", Be::READWRITE )
		MAP_ATTRIBUTE( "update", m_update, "Toggle update", Be::READWRITE )
		MAP_ATTRIBUTE( "curveSet", m_curveSet, "Curve Set affecting the overlay effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "opaqueEffects", m_opaqueEffects, "List of opaque effects", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "decalEffects", m_decalEffects, "List of decal effects, rendered before transparency", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "transparentEffects", m_transparentEffects, "List of transparent effects.", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "additiveEffects", m_additiveEffects, "List of additive effect, rendered after transparencies", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "distortionEffects", m_distortionEffects, "List of distortion effects", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "controllers", m_controllers, "Controllers for awesomeness", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}
