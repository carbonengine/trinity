// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2DeferredLighting.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2EffectStateManager.h"

Tr2DeferredLighting::Tr2DeferredLighting( const char* effectFilePath )
{
	m_effect.CreateInstance();
	m_effect->SetEffectPathName( effectFilePath );
}

Tr2DeferredLighting::~Tr2DeferredLighting() = default;

void Tr2DeferredLighting::Render( const Tr2TextureAL& colorMap, const Tr2TextureAL& depthMap, const Tr2TextureAL& albedoMap, const Tr2TextureAL& specularMap, const Tr2TextureAL& normalMap, const Tr2TextureAL& parametersMap, Tr2RenderContext& renderContext )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	GPU_REGION( renderContext, "Lighting Pass" );

	static const BlueSharedString albedoName( "GBufferAlbedo" );
	static const BlueSharedString normalName( "GBufferNormal" );
	static const BlueSharedString parametersName( "GBufferParameters" );
	static const BlueSharedString specularName( "GBufferSpecular" );
	m_effect->SetParameter( albedoName, albedoMap );
	m_effect->SetParameter( normalName, normalMap );
	m_effect->SetParameter( parametersName, parametersMap );
	m_effect->SetParameter( specularName, specularMap );

	renderContext.m_esm.PushRenderTarget( colorMap );
	renderContext.m_esm.PushDepthStencilBuffer( depthMap );
	renderContext.SetReadOnlyDepth( true );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	Tr2Renderer::DrawFullScreenWithShader( renderContext, m_effect );

	renderContext.SetReadOnlyDepth( false );
	renderContext.m_esm.PopDepthStencilBuffer();
	renderContext.m_esm.PopRenderTarget();

	m_effect->SetParameter( albedoName, Tr2TextureAL() );
	m_effect->SetParameter( normalName, Tr2TextureAL() );
	m_effect->SetParameter( parametersName, Tr2TextureAL() );
	m_effect->SetParameter( specularName, Tr2TextureAL() );
}

Tr2Effect* Tr2DeferredLighting::GetEffect() const
{
	return m_effect;
}
