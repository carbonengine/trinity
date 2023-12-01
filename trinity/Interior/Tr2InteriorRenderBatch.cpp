#include "StdAfx.h"

#include "Tr2InteriorRenderBatch.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2Renderer.h"


// -----------------------------------------------------------------------------------------------------
// Description
//   Draws the cubemap using a screenspace quad.
// -----------------------------------------------------------------------------------------------------
void Tr2InteriorBackgroundCubemapBatch::SubmitGeometry( Tr2RenderContext& renderContext )
{
	using namespace Tr2RenderContextEnum;
	
	Tr2Renderer::DrawCameraSpaceScreenQuad( renderContext, this->GetShaderStateInterface() , this->GetShaderMaterialInterface() );
}

