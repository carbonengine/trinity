// Copyright © 2020 CCP ehf.

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
	m_estimateNoise.CreateInstance();
	m_estimateNoise->SetEffectPathName( "res:/graphics/effect/managed/space/system/EstimateNoise.fx" );

	m_denoiseEstimate.CreateInstance();
	m_denoiseEstimate->SetEffectPathName( "res:/graphics/effect/managed/space/system/DenoiseEstimate.fx" );

	m_denoiseHoriz.CreateInstance();
	m_denoiseHoriz->SetEffectPathName( "res:/graphics/effect/managed/space/system/Denoise1D.fx" );

	m_denoiseVert.CreateInstance();
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
const BlueSharedString NOISE_ESTIMATE = BlueSharedString( "NoiseEstimate" );

}

Tr2GpuResourcePool::Texture Tr2Denoiser::Apply( Tr2GpuResourcePool::Texture source, const Tr2TextureAL& depth, const Tr2TextureAL& normals, const Matrix& projection, float upscaling, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext )
{
	return Apply( std::move( source ), depth, normals, projection, 0, upscaling, gpuResourcePool, renderContext );
}

Tr2GpuResourcePool::Texture Tr2Denoiser::Apply( Tr2GpuResourcePool::Texture src, const Tr2TextureAL& depth, const Tr2TextureAL& normals, const Matrix& projection, uint32_t index, float upscaling, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext )
{
	if( !src.IsValid() || !depth.IsValid() )
	{
		return {};
	}
	bool hasNormals = normals.IsValid();

	m_denoiseVert->SetOption( NORMALS_INPUT, hasNormals ? NORMALS_INPUT_NORMALS : NORMALS_INPUT_NONE );
	m_denoiseHoriz->SetOption( NORMALS_INPUT, hasNormals ? NORMALS_INPUT_NORMALS : NORMALS_INPUT_NONE );

	const auto width = src->GetWidth();
	const auto height = src->GetHeight();

	auto sourceSize = Vector4( float( width ), float( height ), 1.f / float( width ), 1.f / float( height ) );
	m_denoiseEstimate->SetParameter( SOURCE_DIMENSIONS, sourceSize );

	auto SetParams = [&]( Tr2Effect* effect, const Vector2& direction ) {
		if( m_parametersDirty )
		{
			effect->SetParameter( DEPTH_WEIGHT, m_depthWeight );
			effect->SetParameter( NORMAL_WEIGHT, m_normalWeight );
			effect->SetParameter( PLANE_WEIGHT, m_planeWeight );
			effect->SetParameter( PASS_THROUGH, m_bypass ? 1.f : 0.f );

			effect->SetParameter( AXIS, direction * float( m_stepSize ) );
		}
		effect->SetParameter( RADIUS, std::max( uint32_t( m_radius / upscaling + 0.5f ), 2u ) );

		effect->SetParameter( PROJECTION_INV, projection );
		effect->SetParameter( CLIP_INFO, Vector2( projection._43, projection._33 ) );

		effect->SetParameter( DEPTH_BUFFER, depth );
		effect->SetParameter( NORMAL_BUFFER, normals );
		effect->SetParameter( SOURCE_DIMENSIONS, sourceSize );
	};

	renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.PopDepthStencilBuffer(); } );

	renderContext.m_esm.PushRenderTarget();
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.PopRenderTarget(); } );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	auto noiseEstimate = gpuResourcePool.GetTempTexture( "Tr2Denoiser Noise Estimate", width, height, ImageIO::PIXEL_FORMAT_R8_UNORM, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::RENDER_TARGET );
	{
		GPU_REGION( renderContext, "Estimate noise" );
		renderContext.m_esm.SetRenderTarget( 0, noiseEstimate );
		renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
		m_estimateNoise->SetParameter( SOURCE, src );
		Tr2Renderer::DrawScreenQuad( renderContext, m_estimateNoise );
		m_estimateNoise->SetParameter( SOURCE, Tr2TextureAL{} );
	}

	auto denoiseEstimate = gpuResourcePool.GetTempTexture( "Tr2Denoiser Denoise Mask", width, height, ImageIO::PIXEL_FORMAT_R8_UNORM, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::RENDER_TARGET );
	{
		GPU_REGION( renderContext, "Estimate denoising" );
		m_denoiseEstimate->SetParameter( SOURCE, noiseEstimate );
		renderContext.m_esm.SetRenderTarget( 0, denoiseEstimate );
		renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
		Tr2Renderer::DrawScreenQuad( renderContext, m_denoiseEstimate );
	}
	noiseEstimate = {};

	auto tempTexture = gpuResourcePool.GetTempTexture( "Tr2Denoiser Temp", width, height, ImageIO::PIXEL_FORMAT_R8_UNORM, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::RENDER_TARGET );
	{
		GPU_REGION( renderContext, "Denoise horizontal" );
		SetParams( m_denoiseHoriz, Vector2( 1, 0 ) );
		m_denoiseHoriz->SetParameter( SOURCE, src );
		m_denoiseHoriz->SetParameter( NOISE_ESTIMATE, denoiseEstimate );

		renderContext.m_esm.SetRenderTarget( 0, tempTexture );
		renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
		Tr2Renderer::DrawScreenQuad( renderContext, m_denoiseHoriz );

		m_denoiseHoriz->SetParameter( SOURCE, Tr2TextureAL{} );
		m_denoiseHoriz->SetParameter( NOISE_ESTIMATE, Tr2TextureAL{} );
	}
	src = {};

	auto result = gpuResourcePool.GetTempTexture( "Tr2Denoiser Result", width, height, ImageIO::PIXEL_FORMAT_R8_UNORM, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::RENDER_TARGET );
	{
		GPU_REGION( renderContext, "Denoise vertical" );
		SetParams( m_denoiseVert, Vector2( 0, 1 ) );
		m_denoiseVert->SetParameter( SOURCE, tempTexture );
		m_denoiseVert->SetParameter( NOISE_ESTIMATE, denoiseEstimate );

		renderContext.m_esm.SetRenderTarget( 0, result, true, index );
		renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
		Tr2Renderer::DrawScreenQuad( renderContext, m_denoiseVert );

		m_denoiseVert->SetParameter( SOURCE, Tr2TextureAL{} );
		m_denoiseVert->SetParameter( NOISE_ESTIMATE, Tr2TextureAL{} );
	}

	m_denoiseHoriz->SetParameter( DEPTH_BUFFER, Tr2TextureAL() );
	m_denoiseHoriz->SetParameter( NORMAL_BUFFER, Tr2TextureAL() );
	m_denoiseVert->SetParameter( DEPTH_BUFFER, Tr2TextureAL() );
	m_denoiseVert->SetParameter( NORMAL_BUFFER, Tr2TextureAL() );

	m_parametersDirty = false;
	return result;
}

bool Tr2Denoiser::OnModified( Be::Var* )
{
	m_parametersDirty = true;
	return true;
}

void Tr2Denoiser::SetRadius( uint32_t value )
{
	m_radius = value;
}