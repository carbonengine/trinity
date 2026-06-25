// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2RenderNodeEffect.h"

namespace
{

const Be::VarChooser TriRMChooser[] = {
	// Name		   Value		    Docstring
	{ "RM_OPAQUE", BeCast( Tr2EffectStateManager::RM_OPAQUE ), "Opaque rendering" },
	{ "RM_DECAL", BeCast( Tr2EffectStateManager::RM_DECAL ), "Decal rendering" },
	{ "RM_DECAL_NO_DEPTH", BeCast( Tr2EffectStateManager::RM_DECAL_NO_DEPTH ), "Decal rendering (Normals Only)" },
	{ "RM_ALPHA", BeCast( Tr2EffectStateManager::RM_ALPHA ), "Alpha-blended rendering" },
	{ "RM_ALPHA_ADDITIVE", BeCast( Tr2EffectStateManager::RM_ALPHA_ADDITIVE ), "Additive rendering" },
	{ "RM_DEPTH_ONLY", BeCast( Tr2EffectStateManager::RM_DEPTH_ONLY ), "Depth-only rendering" },
	{ "RM_PICKING", BeCast( Tr2EffectStateManager::RM_PICKING ), "Rendering for picking" },
	{ "RM_FULLSCREEN", BeCast( Tr2EffectStateManager::RM_FULLSCREEN ), "Full-screen effects (2D) rendering" },
	{ "RM_SPRITE2D", BeCast( Tr2EffectStateManager::RM_SPRITE2D ), "2D sprite rendering" },
	{ 0 }
};

}


BLUE_DEFINE( Tr2RenderNodeEffect );

const Be::ClassInfo* Tr2RenderNodeEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2RenderNodeEffect, "" )
		MAP_INTERFACE( ITr2RenderNode )

		MAP_ATTRIBUTE(
			"effect",
			m_effect,
			"",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"viewport",
			m_viewport,
			"",
			Be::READWRITE )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"renderingMode",
			m_renderingMode,
			"The rendering mode for this effect",
			Be::READWRITE | Be::ENUM,
			TriRMChooser )
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"AddSource",
			AddSource,
			1,
			"Adds an input node as a source for this effect.\n\n"
			":param name: Effect parameter name to bind the source texture to.\n"
			":param source: The input node to add.\n"
			":param outputName: Optional name of the output for the node (DepthMap, etc.). Pass an empty string for the default output." )

		MAP_ATTRIBUTE(
			"inputNodes",
			m_inputNodes,
			"Input nodes added as sources to this effect. Do not modify this attribute directly; use AddSource() instead.",
			Be::READ | Be::PERSIST )
	EXPOSURE_END()
}
