///////////////////////////////////////////////////////////////////////////////
//
// Created:		1/17/2019 
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPGodRaysEffect.h"
#include "Tr2Renderer.h"


Tr2PPGodRaysEffect::Tr2PPGodRaysEffect( IRoot* lockobj ) :
	m_godRayColor( 1.0, 1.0, 1.0, 1.0 ),
	m_intensityVector(0.0, 0.0, 1.0, 1.0),
	m_intensity( 0.0f ),
	m_noiseTexturePath( "res:/Texture/Global/noise.dds" )
{
	Vector4 grFactors = Vector4( 1000.0, 0.2, 128.0, 2.0 );
	
	m_downSampleEffect.CreateInstance();
	m_downSampleEffect->StartUpdate();
	m_downSampleEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/DownsampleDepth.fx" );
	m_downSampleEffect->EndUpdate();
	
	m_effect.CreateInstance();
	m_effect->StartUpdate();
	m_effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Godrays.fx" );
	m_effect->SetParameter( BlueSharedString( "Color" ), m_godRayColor );
	m_effect->SetParameter( BlueSharedString( "Intensity" ), m_intensityVector );
	m_effect->SetParameter( BlueSharedString( "grFactors" ), grFactors );
	m_effect->AddResourceTexture2D( BlueSharedString( "NoiseTexMap" ), m_noiseTexturePath.c_str() );
	m_effect->EndUpdate();
}


Tr2PPGodRaysEffect::~Tr2PPGodRaysEffect()
{
}


bool Tr2PPGodRaysEffect::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_noiseTexturePath ) )
	{
		TriTextureParameter* resource = dynamic_cast< TriTextureParameter* >( m_effect->GetResourceByName( "NoiseTexMap" ) );
		resource->SetResourcePath( m_noiseTexturePath.c_str() );
	}
	else if( IsMatch( value, m_intensity ) )
	{
		m_intensityVector.x = m_intensity;
		m_effect->StartUpdate();
		m_effect->SetParameter( BlueSharedString( "Intensity" ), m_intensityVector );
		m_effect->EndUpdate();
	}
	else if( IsMatch( value, m_godRayColor) )
	{
		m_effect->StartUpdate();
		m_effect->SetParameter( BlueSharedString( "Color" ), m_godRayColor );
		m_effect->EndUpdate();
	}
	return true;
}


void Tr2PPGodRaysEffect::Render( 
	Tr2RenderContext& renderContext, 
	Tr2RenderTarget* downSampleRT, 
	Tr2RenderTarget* godRayRT, 
	Tr2RenderTarget* backBufferRT, 
	Tr2ShaderBuffer* m_psData )
{
	if( m_intensity > 0 )
	{
		// Downsample 
		m_psData->ApplyBuffer( renderContext );
		Tr2Renderer::PushRenderTarget( *downSampleRT, renderContext );

		HRESULT hr = renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0x00000000, 1.0, 0 );
		if( !SUCCEEDED( hr ) )
		{
			CCP_LOGERR( "Godray depth clear failed" );
		}
		Tr2Renderer::DrawScreenQuad( m_downSampleEffect );
		Tr2Renderer::PopRenderTarget( renderContext );

		// God rays
		Tr2Renderer::PushRenderTarget( *godRayRT, renderContext );
		hr = renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0x00000000, 1.0, 0 );

		if( !SUCCEEDED( hr ) )
		{
			CCP_LOGERR( "Godray clear failed" );
		}
		m_effect->StartUpdate();
		m_effect->SetParameter( BlueSharedString( "DepthMap" ), downSampleRT );
		m_effect->EndUpdate();

		Tr2Renderer::DrawScreenQuad( m_effect );
		Tr2Renderer::PopRenderTarget( renderContext );
		
		// Blit
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
		Tr2Renderer::PushRenderTarget( *backBufferRT, renderContext );
		Tr2Renderer::DrawTexture( *godRayRT, Vector2( 0, 0 ), Vector2( 1, 1 ) );
		Tr2Renderer::PopRenderTarget( renderContext );
	}
}
