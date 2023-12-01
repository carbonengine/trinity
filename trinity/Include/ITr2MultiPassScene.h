////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef ITr2MultiPassScene_H
#define ITr2MultiPassScene_H

class Tr2RenderContext;

// -------------------------------------------------------------
// Description:
//   ITr2MultiPassScene is an interface for multi-pass rendering
//   of scenes.
// SeeAlso:
//   ITr2Scene
// -------------------------------------------------------------
BLUE_INTERFACE( ITr2MultiPassScene ) : public IRoot
{
	enum RenderPassResult
	{
		PASS_RESULT_OK,
		PASS_RESULT_TERMINATE,
	};

	enum PassType
	{
		RP_BEGIN_RENDER,
		RP_PRE_PASS,
		RP_LIGHT_PASS,
		RP_GATHER_PASS,
		RP_FLARE_PASS,
		RP_END_RENDER,
		RP_BACKGROUND_RENDER,
		RP_MAIN_RENDER,
		RP_REFLECTION_RENDER ,
		RP_DEPTH_PASS,
		RP_SET_PERFRAME_DATA,
		RP_RENDER_UI,
		RP_COUNT,
	};

	// Renders a given pass
	virtual RenderPassResult RenderPass( PassType pass, Tr2RenderContext& renderContext ) = 0;
};

#endif // ITr2MultiPassScene_H
