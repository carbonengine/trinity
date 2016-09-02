#include "StdAfx.h"
#include "Tr2Blitter.h"
#include "Shader/Tr2Effect.h"

#include "TriDevice.h"	//TODO gTriDev/ScreenVertexDecl.

static const char* BLIT_EFFECT_PATH = "res:/Graphics/Effect/Managed/space/system/Blit.fx";
static const char* BLIT_FILTERED_EFFECT_PATH = "res:/Graphics/Effect/Managed/space/system/BlitFiltered.fx";
static const char* BLITCUBE_EFFECT_PATH = "res:/Graphics/Effect/Managed/space/system/BlitCube.fx";

using namespace Tr2RenderContextEnum;

Tr2Blitter::Tr2Blitter()
	: m_screenVertexDecl( -1 )
	, m_cubeFaceVar( "cubeFace", 0.f )
	, m_mipLevelVar( "mipLevel", 0.f )
{
	// register so effect parameters pick it up -- but otherwise we only care about
	// the per thread version
	if( auto var = GlobalStore().RegisterPlaceholderTextureVariable( "BlitSource" ) )
	{
		var->SetMultithreaded( true );
	}

	// create all shaders needed in the blitter
	m_blitEffect.CreateInstance();
	m_blitEffect->SetEffectPathName( BLIT_EFFECT_PATH );
	m_blitFilteredEffect.CreateInstance();
	m_blitFilteredEffect->SetEffectPathName( BLIT_FILTERED_EFFECT_PATH );
		
	m_blitCubeEffect.CreateInstance();
	m_blitCubeEffect->SetEffectPathName( BLITCUBE_EFFECT_PATH );

	Tr2Blitter::PrepareResources();
}

Tr2Blitter::~Tr2Blitter()
{
	GlobalStore().UnregisterVariable( "BlitSource" );
	GlobalStore().UnregisterVariable( "cubeFace" );
	GlobalStore().UnregisterVariable( "mipLevel" );
}

bool Tr2Blitter::Draw( ITr2ShaderMaterial* effect, 
                       const Vector2& tlTexCoord, const Vector2& brTexCoord, 
                       const Vector2& tlVertexCoord, const Vector2& brVertexCoord )
{
    return DrawHelper( effect->GetShaderStateInterface(), effect, nullptr, false, tlTexCoord, brTexCoord, 
                       tlVertexCoord, brVertexCoord );
}

bool Tr2Blitter::Draw( Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord, Filtering filter )
{
	switch( filter )
	{
	case FILTER_POINT:
		return DrawHelper( m_blitEffect->GetShaderStateInterface(), m_blitEffect, &texture, false, tlTexCoord, brTexCoord );
	case FILTER_LINEAR:
		return DrawHelper( m_blitFilteredEffect->GetShaderStateInterface(), m_blitFilteredEffect, &texture, false, tlTexCoord, brTexCoord );
	}
	return false;
}

bool Tr2Blitter::Draw( Tr2TextureAL& texture, 
                       const Vector2& tlTexCoord, const Vector2& brTexCoord,
                       const Vector2& tlVertexCoord, const Vector2& brVertexCoord )
{
    return DrawHelper( m_blitEffect->GetShaderStateInterface(), m_blitEffect, &texture, false, 
                       tlTexCoord, brTexCoord, tlVertexCoord, brVertexCoord );
}

bool Tr2Blitter::Draw( ITr2ShaderMaterial* effect, Tr2TextureAL& texture )
{
	return DrawHelper( effect->GetShaderStateInterface(), effect, &texture, false );
}

bool Tr2Blitter::Draw( ITr2ShaderMaterial* effect, Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord )
{
	return DrawHelper( effect->GetShaderStateInterface(), effect, &texture, false, tlTexCoord, brTexCoord );
}

bool Tr2Blitter::Draw( ITr2ShaderMaterial* effect, Tr2TextureAL& texture, 
                       const Vector2& tlTexCoord, const Vector2& brTexCoord, 
                       const Vector2& tlVertexCoord, const Vector2& brVertexCoord )
{
    return DrawHelper( effect->GetShaderStateInterface(), effect, &texture, false, tlTexCoord, brTexCoord, 
                       tlVertexCoord, brVertexCoord );
}
// --------------------------------------------------------------------------------------
// Description:
//   Wrapper around DrawHelper.
// Arguments:
//   material - the Tr2ShaderMaterial to use when drawing.
// Return Value:
//   true if success, false if there is an error.
// See Also: Tr2Renderer::DrawFullScreenWithShader()
// --------------------------------------------------------------------------------------
bool Tr2Blitter::Draw( ITr2ShaderMaterial* material)
{
	ITr2ShaderState * shader = material->GetShaderStateInterface();

	return DrawHelper( shader, material, NULL, false );
}

bool Tr2Blitter::Draw( ITr2ShaderMaterial* effect, const Vector2& tlTexCoord, const Vector2& brTexCoord )
{
	ITr2ShaderState * shader = effect->GetShaderStateInterface();

	return DrawHelper( shader, effect, NULL, false, tlTexCoord, brTexCoord );
}

bool Tr2Blitter::DrawInCameraSpace(ITr2ShaderState* shader, ITr2ShaderMaterial* material )
{
	return DrawHelper( shader, material, NULL, true );
}

bool Tr2Blitter::DrawCube( Tr2TextureAL& texture, Tr2RenderContextEnum::CubemapFace face, int mipLevel )
{
	// set the face we want to render
	m_cubeFaceVar = (float)face;
	m_mipLevelVar = (float)mipLevel;

	return DrawHelper( m_blitCubeEffect->GetShaderStateInterface(), m_blitCubeEffect, &texture, false );
}

bool Tr2Blitter::DrawHelper( ITr2ShaderState* shader, ITr2ShaderMaterial* material, 
                             Tr2TextureAL* halTexture,
							 bool isCameraSpace,
							 const Vector2& tlTexCoord, const Vector2& brTexCoord,
                             const Vector2& tlVertexCoord, const Vector2& brVertexCoord )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	CCP_ASSERT( material );
	if( !material )
	{
		// Final build should not crash even if we get here with a null effect
		return false;
	}

	if( m_screenVertexDecl == -1 || !m_vertexBuffer.IsValid() )
	{
		return false;
	}

	if( !shader )
	{
		return false;
	}

	Tr2ScreenVertex* quad = NULL;
	CR_RETURN_VAL( m_vertexBuffer.Lock( quad, LOCK_WRITEONLY, renderContext ), false );

	if( isCameraSpace )
	{
        CCP_ASSERT( tlVertexCoord[0] == 0.0f && tlVertexCoord[1] == 0.0f );
        CCP_ASSERT( brVertexCoord[0] == 1.0f && brVertexCoord[1] == 1.0f );

		SetupScreenQuadInCameraSpace( quad );
	}
	else
	{
		SetupScreenQuad( quad, tlTexCoord, brTexCoord, tlVertexCoord, brVertexCoord );
	}

	m_vertexBuffer.Unlock( renderContext );

	renderContext.m_esm.ApplyVertexDeclaration( m_screenVertexDecl );
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( Tr2ScreenVertex ) );

	if( halTexture )
	{
		//m_blitSourceVar = halTexture;
		renderContext.GetVariableStore().GetVariable( "BlitSource" )->SetValue( halTexture );
	}

	unsigned int passCount = shader->GetPassCount();

	for( unsigned int passIx = 0; passIx < passCount; ++passIx )
	{
		shader->ApplyAllStateForPass( passIx, renderContext );
		material->ApplyMaterialDataForPass( passIx, renderContext );
		{
			renderContext.SetTopology( TOP_TRIANGLE_STRIP );
			renderContext.DrawPrimitive( 0, 2 );
		}
	}

	renderContext.GetVariableStore().GetVariable( "BlitSource" )->Clear();	

	return true;
}

bool Tr2Blitter::OnPrepareResources()
{
	if( m_screenVertexDecl == -1 )
	{
		m_screenVertexDecl = gTriDev->CreateScreenVertexDecl();
	}

	if( !m_vertexBuffer.IsValid() )
	{
		unsigned int vbSize = sizeof( Tr2ScreenVertex ) * 4;
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_vertexBuffer.Create( vbSize, USAGE_CPU_WRITE | USAGE_LOCK_FREQUENTLY, nullptr, renderContext );
	}

	return true;
}

void Tr2Blitter::ReleaseResources( TriStorage s )
{
	m_screenVertexDecl = -1;
	m_vertexBuffer.Destroy();
}
