////////////////////////////////////////////////////////////
//
//    Created:   January 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "Tr2Denoiser.h"
#include "Tr2RenderTarget.h"
#include "Shader/Tr2Effect.h"
#include "Tr2Renderer.h"


Tr2Denoiser::Tr2Denoiser() :
	m_radius( 5 ),
	m_stepSize( 1 ),
	m_depthWeight( 100 ),
	m_normalWeight( 1.5f ),
	m_planeWeight( 0 ),
	m_bypass( false ),
	m_parametersDirty( true )
{
	m_noiseEstimate.CreateInstance();
	m_intermediate.CreateInstance();
	m_result.CreateInstance();

	m_estimateNoise.CreateInstance();
	m_estimateNoise->SetEffectPathName( "res:/graphics/effect/managed/space/system/EstimateNoise.fx" );

	m_denoiseEstimate.CreateInstance();
	m_denoiseEstimate->SetParameter( BlueSharedString( "Source" ), m_intermediate );
	m_denoiseEstimate->SetParameter( BlueSharedString( "Source" ), m_intermediate );
	m_denoiseEstimate->SetEffectPathName( "res:/graphics/effect/managed/space/system/DenoiseEstimate.fx" );

	m_denoiseHoriz.CreateInstance();
	m_denoiseHoriz->SetParameter( BlueSharedString( "NoiseEstimate" ), m_noiseEstimate );
	m_denoiseHoriz->SetEffectPathName( "res:/graphics/effect/managed/space/system/Denoise1D.fx" );

	m_denoiseVert.CreateInstance();
	m_denoiseVert->SetParameter( BlueSharedString( "NoiseEstimate" ), m_noiseEstimate );
	m_denoiseVert->SetParameter( BlueSharedString( "Source" ), m_intermediate );
	m_denoiseVert->SetEffectPathName( "res:/graphics/effect/managed/space/system/Denoise1D.fx" );
}

namespace
{
const BlueSharedString NORMALS_INPUT = BlueSharedString( "NORMALS_INPUT" );
const BlueSharedString NORMALS_INPUT_NORMALS = BlueSharedString( "NORMALS_INPUT_NORMALS" );
const BlueSharedString NORMALS_INPUT_NONE = BlueSharedString( "NORMALS_INPUT_NONE" );

const BlueSharedString SOURCE = BlueSharedString( "Source" );
const BlueSharedString DEPTH_BUFFER = BlueSharedString( "DepthBuffer" );
const BlueSharedString RADIUS = BlueSharedString( "Radius" );
const BlueSharedString SOURCE_DIMENSIONS = BlueSharedString( "SourceDimensions" );
const BlueSharedString DEPTH_WEIGHT = BlueSharedString( "DepthWeight" );
const BlueSharedString NORMAL_WEIGHT = BlueSharedString( "NormalWeight" );
const BlueSharedString PLANE_WEIGHT = BlueSharedString( "PlaneWeight" );
const BlueSharedString PASS_THROUGH = BlueSharedString( "PassThrough" );
const BlueSharedString AXIS = BlueSharedString( "Axis" );
const BlueSharedString PROJECTION_INV = BlueSharedString( "ProjectionInv" );
const BlueSharedString CLIP_INFO = BlueSharedString( "ClipInfo" );
const BlueSharedString NORMAL_BUFFER = BlueSharedString( "NormalBuffer" );

}

ALResult Tr2Denoiser::Apply( ITr2TextureProvider& source, ITr2TextureProvider& depth, ITr2TextureProvider* normals, const Matrix& projection, Tr2RenderContext& renderContext )
{
	auto src = source.GetTexture();
	auto depthMap = depth.GetTexture();
	if( !src || !src->IsValid() || !depthMap || !depthMap->IsValid() )
	{
		return E_INVALIDARG;
	}
	bool hasNormals = false;
	if( normals )
	{
		auto normalTex = normals->GetTexture();
		if( normalTex && normalTex->IsValid() )
		{
			hasNormals = true;
		}
	}

	m_denoiseVert->SetOption( NORMALS_INPUT, hasNormals ? NORMALS_INPUT_NORMALS : NORMALS_INPUT_NONE );
	m_denoiseHoriz->SetOption( NORMALS_INPUT, hasNormals ? NORMALS_INPUT_NORMALS : NORMALS_INPUT_NONE );

	if( !m_noiseEstimate->IsValid() || m_noiseEstimate->GetWidth() != src->GetWidth() || m_noiseEstimate->GetHeight() != src->GetHeight() )
	{
		m_noiseEstimate->Create( src->GetWidth(), src->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM );
	}
	if( !m_intermediate->IsValid() || m_intermediate->GetWidth() != src->GetWidth() || m_intermediate->GetHeight() != src->GetHeight() )
	{
		m_intermediate->Create( src->GetWidth(), src->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM );
	}
	if( !m_result->IsValid() || m_result->GetWidth() != src->GetWidth() || m_result->GetHeight() != src->GetHeight() )
	{
		m_result->Create( src->GetWidth(), src->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM );
	}

	m_estimateNoise->SetParameter( SOURCE, &source );

	auto sourceSize = Vector4( float( src->GetWidth() ), float( src->GetHeight() ), 1.f / float( src->GetWidth() ), 1.f / float( src->GetHeight() ) );
	m_denoiseEstimate->SetParameter( SOURCE_DIMENSIONS, sourceSize );

	auto SetParams = [&]( Tr2Effect* effect, const Vector2& direction ) {
		if( m_parametersDirty )
		{
			effect->SetParameter( DEPTH_WEIGHT, m_depthWeight );
			effect->SetParameter( NORMAL_WEIGHT, m_normalWeight );
			effect->SetParameter( PLANE_WEIGHT, m_planeWeight );
			effect->SetParameter( PASS_THROUGH, m_bypass ? 1.f : 0.f );

			effect->SetParameter( RADIUS, m_radius );
			effect->SetParameter( AXIS, direction * float( m_stepSize ) );
		}

		effect->SetParameter( PROJECTION_INV, projection );
		effect->SetParameter( CLIP_INFO, Vector2( projection._43, projection._33 ) );

		effect->SetParameter( DEPTH_BUFFER, &depth );
		effect->SetParameter( NORMAL_BUFFER, normals );
		effect->SetParameter( SOURCE_DIMENSIONS, sourceSize );
	};

	renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.PopDepthStencilBuffer(); } );

	renderContext.m_esm.PushRenderTarget();
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.PopRenderTarget(); } );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	renderContext.m_esm.SetRenderTarget( 0, *m_intermediate );
	renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
	Tr2Renderer::DrawScreenQuad( renderContext, m_estimateNoise );

	renderContext.m_esm.SetRenderTarget( 0, *m_noiseEstimate );
	renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
	Tr2Renderer::DrawScreenQuad( renderContext, m_denoiseEstimate );

	m_denoiseHoriz->SetParameter( SOURCE, &source );
	SetParams( m_denoiseHoriz, Vector2( 1, 0 ) );
	renderContext.m_esm.SetRenderTarget( 0, *m_intermediate );
	renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
	Tr2Renderer::DrawScreenQuad( renderContext, m_denoiseHoriz );

	SetParams( m_denoiseVert, Vector2( 0, 1 ) );
	renderContext.m_esm.SetRenderTarget( 0, *m_result );
	renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
	Tr2Renderer::DrawScreenQuad( renderContext, m_denoiseVert );

	m_parametersDirty = false;
	return S_OK;
}

ITr2TextureProvider* Tr2Denoiser::GetTexture() const
{
	return m_result;
}

bool Tr2Denoiser::OnModified( Be::Var* )
{
	m_parametersDirty = true;
	return true;
}