#include "StdAfx.h"
#include "TriStepSetStandardRenderStates.h"
#include "TriConstants.h"

BLUE_DEFINE( TriStepSetStdRndStates );

const Be::VarChooser TriRMChooser[] =
{
	// Name		   Value		    Docstring
	{ "RM_OPAQUE",			BeCast( Tr2EffectStateManager::RM_OPAQUE ),			"Opaque rendering" }, 
	{ "RM_DECAL",			BeCast( Tr2EffectStateManager::RM_DECAL ),			"Decal rendering" },
	{ "RM_DECAL_NO_DEPTH",	BeCast( Tr2EffectStateManager::RM_DECAL_NO_DEPTH ),	"Decal rendering (Normals Only)" },
	{ "RM_ALPHA",			BeCast( Tr2EffectStateManager::RM_ALPHA ),			"Alpha-blended rendering" },
	{ "RM_ALPHA_ADDITIVE",	BeCast( Tr2EffectStateManager::RM_ALPHA_ADDITIVE ),	"Additive rendering" },
	{ "RM_DEPTH_ONLY",		BeCast( Tr2EffectStateManager::RM_DEPTH_ONLY ),		"Depth-only rendering" },
	{ "RM_PICKING",			BeCast( Tr2EffectStateManager::RM_PICKING ),		"Rendering for picking" },
	{ "RM_FULLSCREEN",		BeCast( Tr2EffectStateManager::RM_FULLSCREEN ),		"Full-screen effects (2D) rendering" },
	{ "RM_SPRITE2D",		BeCast( Tr2EffectStateManager::RM_SPRITE2D ),		"2D sprite rendering" },
	{ 0 }
};

BLUE_REGISTER_ENUM( "RENDERING_MODE", Tr2EffectStateManager::RenderingMode, TriRMChooser );

const Be::ClassInfo* TriStepSetStdRndStates::ExposeToBlue()
{
    EXPOSURE_BEGIN( TriStepSetStdRndStates, "" )
        MAP_INTERFACE( TriStepSetStdRndStates )
		MAP_INTERFACE( TriRenderStep )

		MAP_ATTRIBUTE_WITH_CHOOSER( "renderingMode", m_renderingMode, "", Be::READWRITE | Be::PERSIST | Be::ENUM, TriRMChooser )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__", 
			py__init__, 
			1,
			"Creates a render step that sets a set of standard render states\n"
			":param renderMode: a value from the RM_ enumeration"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}
