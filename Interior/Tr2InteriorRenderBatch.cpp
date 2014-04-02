#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorRenderBatch.h"
#include "Resources/TriGeometryRes.h"


const std::string Tr2InteriorClippingBatch::s_batchTypeName = "Tr2InteriorClippingBatch";
const std::string Tr2InteriorStencilMaskBatch::s_batchTypeName = "Tr2InteriorStencilMaskBatch";
const std::string Tr2InteriorBackgroundCubemapBatch::s_batchTypeName = "Tr2InteriorBackgroundCubemapBatch";

// -----------------------------------------------------------------------------------------------------
// Description
//   Sets a user clip plane and per-frame data, including a float to indicate cull mode inversion.
//   The per-frame data is needed to ensure correct normal mapping across mirrors.                                     
// -----------------------------------------------------------------------------------------------------
void Tr2InteriorClippingBatch::SubmitGeometry( Tr2RenderContext& renderContext )
{
	TriClippingBatch::SubmitGeometry( renderContext );
	if( !m_PSBuffer.IsValid() )
	{
		CR_RETURN( m_PSBuffer.Create( 
			uint32_t( sizeof( m_psData ) ), 
			Tr2RenderContextEnum::USAGE_CPU_WRITE | Tr2RenderContextEnum::USAGE_LOCK_FREQUENTLY, 
			nullptr, 
			renderContext.GetPrimaryRenderContext() ) );
	}
	FillAndSetConstants( m_PSBuffer, m_psData, Tr2RenderContextEnum::PIXEL_SHADER, Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
}

// -----------------------------------------------------------------------------------------------------
// Description
//   Sets the depth/stencil state and draws a mask for a mirror.  If the stencil mode is increment, then
//   it draws the geometry again to force the depth buffer to 1 in the mask region (manual depth buffer
//   clear).  Sets appropriate depth/stencil state following the mask draw.
// -----------------------------------------------------------------------------------------------------
void Tr2InteriorStencilMaskBatch::SubmitGeometry( Tr2RenderContext& renderContext )
{
	using namespace Tr2RenderContextEnum;
	
	if( m_disableStencil )
	{
		renderContext.SetRenderState( RS_STENCILENABLE, FALSE );
		return;
	}

	if( m_setStencilTestOnly )
	{
		D3DPERF_EVENT( L"Set stencil test" );

		renderContext.SetRenderState( RS_STENCILENABLE, TRUE );
		renderContext.SetRenderState( RS_STENCILFUNC, CMP_EQUAL );
		renderContext.SetRenderState( RS_STENCILREF, (uint32_t)m_stencilTest );
		renderContext.SetRenderState( RS_STENCILMASK, 0xff );

		renderContext.SetRenderState( RS_STENCILFAIL, D3DSTENCILOP_KEEP );
		renderContext.SetRenderState( RS_STENCILZFAIL, D3DSTENCILOP_KEEP );
		renderContext.SetRenderState( RS_STENCILPASS, D3DSTENCILOP_KEEP );

		return;
	}

	// Setup initial stencil state
	{
		D3DPERF_EVENT( L"Stencil mask - setup stencil state" );
		
		if( !m_colorWrite )
		{
			renderContext.SetRenderState( RS_COLORWRITEENABLE, 0 );
		}

		renderContext.SetRenderState( RS_ALPHATESTENABLE, FALSE );

		renderContext.SetRenderState( RS_STENCILENABLE, TRUE );
		renderContext.SetRenderState( RS_STENCILFUNC, CMP_EQUAL );
		renderContext.SetRenderState( RS_STENCILREF, (uint32_t)m_stencilTest );
		renderContext.SetRenderState( RS_STENCILMASK, 0xff );

		renderContext.SetRenderState( RS_STENCILFAIL, D3DSTENCILOP_KEEP );
		renderContext.SetRenderState( RS_STENCILZFAIL, D3DSTENCILOP_KEEP );
		renderContext.SetRenderState( RS_STENCILPASS, m_stencilPassState );
	}

	uint32_t oldZWriteEnable = TRUE;
	renderContext.GetRenderState( RS_ZWRITEENABLE, &oldZWriteEnable );
	// Render stencil mask geometry
	{
		D3DPERF_EVENT( L"Stencil mask - render mask geometry" );

		renderContext.SetRenderState( RS_ZWRITEENABLE, TRUE );
		if( m_geometryResource )
		{
			m_geometryResource->RenderAreas( m_meshIndex, m_areaIndex, m_areaCount, renderContext );
		}
	}

	// Clear the depth buffer for the stenciled region
	if( m_doDepthClear )
	{
		D3DPERF_EVENT( L"Stencil mask - clear depth in stenciled region" );

		renderContext.SetRenderState( RS_ZFUNC, CMP_ALWAYS );

		renderContext.SetRenderState( RS_STENCILFUNC, CMP_EQUAL );
		renderContext.SetRenderState( RS_STENCILREF, (uint32_t)m_stencilWrite );
		renderContext.SetRenderState( RS_STENCILMASK, 0xff );

		renderContext.SetRenderState( RS_STENCILFAIL, D3DSTENCILOP_KEEP );
		renderContext.SetRenderState( RS_STENCILZFAIL, D3DSTENCILOP_KEEP );
		renderContext.SetRenderState( RS_STENCILPASS, D3DSTENCILOP_KEEP );

		Tr2Viewport viewport;
		renderContext.GetViewport( viewport );
		viewport.m_minZ = 0.9999f;
		viewport.m_maxZ = 1.0f;
		renderContext.SetViewport( viewport );

		if( m_geometryResource )
		{
			m_geometryResource->RenderAreas( m_meshIndex, m_areaIndex, m_areaCount, renderContext );
		}
	}

	renderContext.SetRenderState( RS_ZWRITEENABLE, oldZWriteEnable );

	// Restore stencil state
	{
		D3DPERF_EVENT( L"Stencil mask - restore stencil state" );

		if( !m_colorWrite )
		{
			renderContext.SetRenderState( RS_COLORWRITEENABLE, 0x0000000F );
		}
		renderContext.SetRenderState( RS_ZFUNC, CMP_LESSEQUAL );

		renderContext.SetRenderState( RS_STENCILFUNC, CMP_EQUAL );
		renderContext.SetRenderState( RS_STENCILREF, (uint32_t)m_stencilWrite );
		renderContext.SetRenderState( RS_STENCILMASK, 0xff );

		renderContext.SetRenderState( RS_STENCILFAIL, D3DSTENCILOP_KEEP );
		renderContext.SetRenderState( RS_STENCILZFAIL, D3DSTENCILOP_KEEP );
		renderContext.SetRenderState( RS_STENCILPASS, D3DSTENCILOP_KEEP );

		Tr2Viewport viewport;
		renderContext.GetViewport( viewport );
		viewport.m_minZ = 0.0f;
		viewport.m_maxZ = 1.0f;
		renderContext.SetViewport( viewport );
	}
}

// -----------------------------------------------------------------------------------------------------
// Description
//   Draws the cubemap using a screenspace quad.
// -----------------------------------------------------------------------------------------------------
void Tr2InteriorBackgroundCubemapBatch::SubmitGeometry( Tr2RenderContext& renderContext )
{
	using namespace Tr2RenderContextEnum;
	
	// TODO, Replace this with new shaders 
	uint32_t oldCull;
	renderContext.GetRenderState( RS_CULLMODE, &oldCull );
	renderContext.SetRenderState( RS_CULLMODE, D3DCULL_NONE );
	Tr2Renderer::DrawCameraSpaceScreenQuad( this->GetShaderStateInterface() , this->GetShaderMaterialInterface() );
	renderContext.SetRenderState( RS_ZENABLE, TRUE );
	renderContext.SetRenderState( RS_ZWRITEENABLE, TRUE );
	renderContext.SetRenderState( RS_CULLMODE, oldCull );

}

#endif
