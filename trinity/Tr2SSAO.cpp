// Copyright © 2022 CCP ehf.

#include "StdAfx.h"

#include "ffx_cacao_defines.h"
#include "ffx_cacao.cpp"
#include "CACAOCommon/Common.h"

#include "Tr2SSAO.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Effect.h"
#include "Resources/TriTextureRes.h"

using namespace Tr2RenderContextEnum;


Tr2SSAO::Tr2SSAO( IRoot* lockobj )
{

	m_cortaoEnabled = true;
	m_cortaoBentNormal = true;

	m_cortaoStrength = 1.0f;
	m_cortaoRadius = 1.0E10f;
	m_cortaoMaxBlockerSearchRadius = 0.25f;
	m_cortaoMipBias = -4.0f;
	m_cortaoBlur = true;

	m_cortaoInitialized = false;

	m_detail.settings = FFX_CACAO_PRESETS[0].settings;
	m_detail.settings.radius = 6;
	m_detail.settings.shadowMultiplier = 1.0f;
	m_detail.settings.shadowPower = 2.6f;
	m_detail.settings.shadowClamp = 0.98f;
	m_detail.settings.sharpness = 0.5f;
	m_detail.settings.detailShadowStrength = 5;
	m_detail.effect.CreateInstance();
	m_detail.effect->SetEffectPathName( "res:/graphics/effect/managed/space/System/SSAO/SSAO.fx" );
}

inline static uint32_t DispatchSize( uint32_t tileSize, uint32_t totalSize )
{
	return ( totalSize + tileSize - 1 ) / tileSize;
}

FFX_CACAO_Settings GetSettings( bool useDownsampledSSAO, bool generateNormals, SSAOQuality quality )
{
	unsigned settingsIndex = static_cast<int>( quality ) + useDownsampledSSAO * ( static_cast<int>( SSAOQuality::LOWEST ) + 1 );
	FFX_CACAO_Settings settings = FFX_CACAO_PRESETS[settingsIndex].settings;

	settings.generateNormals = generateNormals;

	// Set fade out to something large to disable it
	settings.fadeOutFrom = 10;
	settings.fadeOutTo = 1;

	// Set to maximum to compensate for our large scene sizes
	settings.radius = 5;
	settings.sharpness = 0.5f;
	settings.detailShadowStrength = 5;
	settings.shadowPower = 2;

	return settings;
}

void Tr2SSAO::Enable( bool enable )
{
	m_detail.enabled = enable;
}

void Tr2SSAO::SetQuality( SSAOQuality quality, bool downsampled )
{
	m_detail.quality = quality;
	m_detail.downsampled = downsampled;
}


HRESULT Tr2SSAO::ApplyConstBuffer( unsigned pass, Tr2RenderContext& renderContext )
{
	return renderContext.SetConstants( m_constBuffers[std::min( pass, SSAO_PASS_COUNT )], COMPUTE_SHADER, Tr2Renderer::GetPerFramePSStartRegister() );
}

Tr2GpuResourcePool::Texture Tr2SSAO::Filter( const Tr2TextureAL& depthBuffer, const Tr2TextureAL& normalBuffer, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, bool temporal )
{

	if( !m_detail.enabled )
	{
		return {};
	}

	GPU_REGION( renderContext, "SSAO" );
	if( m_cortaoEnabled )
	{
		//Lazily initialize CORTAO, as EVE currently doesn't use it, so it doesn't have the shaders/lookup table for it.
		if( !m_cortaoInitialized )
		{
			m_cortaoEffect.CreateInstance();
			m_cortaoEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/System/CORTAO/CORTAO.fx" );

			m_cortaoBlurEffect.CreateInstance();
			m_cortaoBlurEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/System/CORTAO/Blur.fx" );


			m_cortaoLookupTable.CreateInstance();

			//Available lookup tables, the lowest resolution one is sufficient for our sample counts.
			BeResMan->GetResource( "res:/texture/ssao/24x24x16x16.dds", "", m_cortaoLookupTable );
			//BeResMan->GetResource( "res:/texture/ssao/32x32x24x16.dds", "", m_cortaoLookupTable );
			//BeResMan->GetResource( "res:/texture/ssao/64x64x32x16.dds", "", m_cortaoLookupTable );

			m_cortaoInitialized = true;
		}

		GPU_REGION( renderContext, "CORTAO" );
		return ComputeCORTAO( depthBuffer, normalBuffer, gpuResourcePool, renderContext, temporal );
	}
	else
	{
		GPU_REGION( renderContext, "CACAO" );
		return PerformPass( m_detail, depthBuffer, normalBuffer, false, gpuResourcePool, renderContext );
	}
}

Tr2GpuResourcePool::Texture Tr2SSAO::PerformPass( const Layer& layer, const Tr2TextureAL& depthBuffer, const Tr2TextureAL& normalBuffer, bool reuseNormals, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext )
{
	m_detail.effect->SetOption( BlueSharedString( "SSAO_INPUT_2D_TEXTURE_TYPE" ), BlueSharedString( depthBuffer.GetMsaaDesc().samples > 1 ? "TEXTURE_2DMS" : "TEXTURE_2D" ) );

	FFX_CACAO_BufferSizeInfo size{};
	FFX_CACAO_UpdateBufferSizeInfo( depthBuffer.GetWidth(), depthBuffer.GetHeight(), layer.downsampled, &size );
	if( !size.importanceMapWidth || !size.importanceMapHeight )
	{
		return {};
	}

	for( unsigned i = 0; i <= SSAO_PASS_COUNT; i++ )
	{
		if( !m_constBuffers[i].IsValid() )
		{
			CR_RETURN_VAL( m_constBuffers[i].Create( sizeof( FFX_CACAO_Constants ), renderContext.GetPrimaryRenderContext() ), {} );
		}
	}


	// From CACAO sample project
	FFX_CACAO_Matrix4x4 proj{}, normalsWorldToView{};
	{
		XMFLOAT4X4 p;
		XMMATRIX xProj = Tr2Renderer::GetReversedDepthProjectionTransform();
		XMStoreFloat4x4( &p, xProj );
		proj.elements[0][0] = p._11;
		proj.elements[0][1] = p._12;
		proj.elements[0][2] = p._13;
		proj.elements[0][3] = p._14;
		proj.elements[1][0] = p._21;
		proj.elements[1][1] = p._22;
		proj.elements[1][2] = p._23;
		proj.elements[1][3] = p._24;
		proj.elements[2][0] = p._31;
		proj.elements[2][1] = p._32;
		proj.elements[2][2] = p._33;
		proj.elements[2][3] = p._34;
		proj.elements[3][0] = p._41;
		proj.elements[3][1] = p._42;
		proj.elements[3][2] = p._43;
		proj.elements[3][3] = p._44;
		XMMATRIX xNormalsWorldToView = XMMATRIX( 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1 ) * Tr2Renderer::GetInverseViewTransform(); // should be transpose(inverse(view)), but XMM is row-major and HLSL is column-major
		XMStoreFloat4x4( &p, xNormalsWorldToView );
		normalsWorldToView.elements[0][0] = p._11;
		normalsWorldToView.elements[0][1] = p._12;
		normalsWorldToView.elements[0][2] = p._13;
		normalsWorldToView.elements[0][3] = p._14;
		normalsWorldToView.elements[1][0] = p._21;
		normalsWorldToView.elements[1][1] = p._22;
		normalsWorldToView.elements[1][2] = p._23;
		normalsWorldToView.elements[1][3] = p._24;
		normalsWorldToView.elements[2][0] = p._31;
		normalsWorldToView.elements[2][1] = p._32;
		normalsWorldToView.elements[2][2] = p._33;
		normalsWorldToView.elements[2][3] = p._34;
		normalsWorldToView.elements[3][0] = p._41;
		normalsWorldToView.elements[3][1] = p._42;
		normalsWorldToView.elements[3][2] = p._43;
		normalsWorldToView.elements[3][3] = p._44;
	}

	proj.elements[3][2] /= layer.zoomLevel;

	bool hasNormals = normalBuffer.IsValid();

	unsigned settingsIndex = static_cast<int>( layer.quality ) + layer.downsampled * ( static_cast<int>( SSAOQuality::HIGHEST ) + 1 );
	FFX_CACAO_Settings settings = GetSettings( layer.downsampled, !hasNormals, layer.quality );

	settings.radius = layer.settings.radius;
	settings.shadowMultiplier = layer.settings.shadowMultiplier;
	settings.shadowPower = layer.settings.shadowPower;
	settings.shadowClamp = layer.settings.shadowClamp;
	settings.sharpness = layer.settings.sharpness;
	settings.detailShadowStrength = layer.settings.detailShadowStrength;

	auto GetTempTexture = [&gpuResourcePool]( auto name, auto dim, auto usage ) {
#if __APPLE__
		return gpuResourcePool.GetPersistentTexture( name, dim, usage, []( auto& tex, auto& rc ) {} );
#else
		return gpuResourcePool.GetTempTexture( name, dim, usage );
#endif
	};

	auto deinterleavedDepthTarget = GetTempTexture(
		"ssao_deinterleaved_depth",
		Tr2BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R32_FLOAT, size.deinterleavedDepthBufferWidth, size.deinterleavedDepthBufferHeight, 1, settings.qualityLevel >= FFX_CACAO_QUALITY_MEDIUM ? 4 : 1, SSAO_PASS_COUNT ),
		Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE );
	auto deinterleavedNormalTarget = GetTempTexture(
		"ssao_deinterleaved_normal",
		Tr2BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R8G8B8A8_SNORM, size.deinterleavedDepthBufferWidth, size.deinterleavedDepthBufferHeight, 1, 1, SSAO_PASS_COUNT ),
		Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE );
	auto ssaoWorkerTargetA = GetTempTexture(
		"ssao_worker_a",
		Tr2BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R8G8_UNORM, size.ssaoBufferWidth, size.ssaoBufferHeight, 1, 1, SSAO_PASS_COUNT ),
		Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE );
	auto ssaoWorkerTargetB = GetTempTexture(
		"ssao_worker_b",
		Tr2BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R8G8_UNORM, size.ssaoBufferWidth, size.ssaoBufferHeight, 1, 1, SSAO_PASS_COUNT ),
		Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE );
	Tr2GpuResourcePool::Texture importanceTargetA;
	Tr2GpuResourcePool::Texture importanceTargetB;
	if( settings.qualityLevel == FFX_CACAO_QUALITY_HIGHEST )
	{
		importanceTargetA = GetTempTexture(
			"ssao_importance_a",
			Tr2BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R8_UNORM, size.importanceMapWidth, size.importanceMapHeight, 1, 1, 1 ),
			Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE );
		importanceTargetB = GetTempTexture(
			"ssao_importance_b",
			Tr2BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R8_UNORM, size.importanceMapWidth, size.importanceMapHeight, 1, 1, 1 ),
			Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE );
	}
	auto outputTarget = GetTempTexture(
		"ssao_output",
		Tr2BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R8_UNORM, depthBuffer.GetWidth(), depthBuffer.GetHeight(), 1, 1, 1 ),
		Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE );


	Tr2GpuResourcePool::Buffer loadCounterBuffer;
	if( m_detail.quality == SSAOQuality::HIGHEST )
	{
		loadCounterBuffer = gpuResourcePool.GetTempBuffer(
			"ssao_load_counter",
			Tr2BufferDescriptionAL( PIXEL_FORMAT_R32_UINT, 1, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE ) );
	}

	layer.effect->SetParameter( BlueSharedString( "g_DepthIn" ), depthBuffer );
	layer.effect->SetParameter( BlueSharedString( "g_PrepareNormalsFromNormalsInput" ), normalBuffer );
	layer.effect->SetParameter( BlueSharedString( "g_PrepareDepthsOut" ), deinterleavedDepthTarget );
	layer.effect->SetParameter( BlueSharedString( "g_PrepareNormals_NormalOut" ), deinterleavedNormalTarget );

	// SSAO pass
	layer.effect->SetParameter( BlueSharedString( "g_ViewspaceDepthSource" ), deinterleavedDepthTarget );
	layer.effect->SetParameter( BlueSharedString( "g_DeinterleavedNormals" ), deinterleavedNormalTarget );
	layer.effect->SetParameter( BlueSharedString( "g_LoadCounter" ), loadCounterBuffer );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceMap" ), importanceTargetA );
	layer.effect->SetParameter( BlueSharedString( "g_FinalSSAO" ), ssaoWorkerTargetA );

	// Importance pass
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceFinalSSAO" ), ssaoWorkerTargetA );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceOut" ), importanceTargetA );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceAIn" ), importanceTargetA );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceAOut" ), importanceTargetB );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceBIn" ), importanceTargetB );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceBOut" ), importanceTargetA );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceBLoadCounter" ), loadCounterBuffer );

	// Edge sensitive blur pass
	layer.effect->SetParameter( BlueSharedString( "g_EdgeSensitiveBlur_Input" ), ssaoWorkerTargetB );
	layer.effect->SetParameter( BlueSharedString( "g_EdgeSensitiveBlur_Output" ), ssaoWorkerTargetA );

	// Bilateral upsample pass
	layer.effect->SetParameter( BlueSharedString( "g_BilateralUpscaleInput" ), settings.blurPassCount ? ssaoWorkerTargetA : ssaoWorkerTargetB );
	layer.effect->SetParameter( BlueSharedString( "g_BilateralUpscaleDepth" ), depthBuffer );
	layer.effect->SetParameter( BlueSharedString( "g_BilateralUpscaleDownscaledDepth" ), deinterleavedDepthTarget );
	layer.effect->SetParameter( BlueSharedString( "g_BilateralUpscaleOutput" ), outputTarget );

	// Apply pass
	layer.effect->SetParameter( BlueSharedString( "g_ApplyFinalSSAO" ), settings.blurPassCount ? ssaoWorkerTargetA : ssaoWorkerTargetB );
	layer.effect->SetParameter( BlueSharedString( "g_ApplyOutput" ), outputTarget );

	{
		GPU_REGION( renderContext, "Upload const buffers" );

		FFX_CACAO_Constants consts{};
		FFX_CACAO_UpdateConstants( &consts, &settings, &size, &proj, &normalsWorldToView );
		FFX_CACAO_Constants constsPP[SSAO_PASS_COUNT + 1]{};

		for( unsigned i = 0; i <= SSAO_PASS_COUNT; i++ )
		{
			constsPP[i] = consts;

			if( i < SSAO_PASS_COUNT )
			{
				FFX_CACAO_UpdatePerPassConstants( constsPP + i, &settings, &size, i );
			}

			void* data;
			CR_RETURN_VAL( m_constBuffers[i].Lock( &data, renderContext ), {} );
			memmove( data, constsPP + i, sizeof( consts ) );
			CR_RETURN_VAL( m_constBuffers[i].Unlock( renderContext ), {} );
		}
	}

	if( settings.qualityLevel == FFX_CACAO_QUALITY_HIGHEST )
	{
		GPU_REGION( renderContext, "Clear load counter" );
		layer.effect->SetParameter( BlueSharedString( "g_ClearLoadCounter_LoadCounter" ), loadCounterBuffer );
		Tr2Renderer::RunComputeShader( layer.effect, BlueSharedString( "ClearLoadCounter" ), 1, 1, 1, renderContext );
		layer.effect->SetParameter( BlueSharedString( "g_ClearLoadCounter_LoadCounter" ), Tr2BufferAL{} );
	}

	{
		GPU_REGION( renderContext, "Prepare" );

		CR_RETURN_VAL( ApplyConstBuffer( SSAO_PASS_COUNT, renderContext ), {} );

		if( settings.qualityLevel == FFX_CACAO_QUALITY_LOWEST )
		{
			std::string shader = layer.downsampled ? "PrepareDownsampledDepthsHalf" : "PrepareNativeDepthsHalf";

			unsigned dimX = DispatchSize( FFX_CACAO_PREPARE_DEPTHS_HALF_WIDTH, size.deinterleavedDepthBufferWidth );
			unsigned dimY = DispatchSize( FFX_CACAO_PREPARE_DEPTHS_HALF_HEIGHT, size.deinterleavedDepthBufferHeight );
			Tr2Renderer::RunComputeShader( layer.effect, BlueSharedString( shader.c_str() ), dimX, dimY, 1, renderContext );
		}
		else
		{
			std::string shader = layer.downsampled ? "PrepareDownsampledDepths" : "PrepareNativeDepths";

			unsigned dimX = DispatchSize( FFX_CACAO_PREPARE_DEPTHS_WIDTH, size.deinterleavedDepthBufferWidth );
			unsigned dimY = DispatchSize( FFX_CACAO_PREPARE_DEPTHS_HEIGHT, size.deinterleavedDepthBufferHeight );
			Tr2Renderer::RunComputeShader( layer.effect, BlueSharedString( shader.c_str() ), dimX, dimY, 1, renderContext );
		}

		if( !reuseNormals )
		{
			if( settings.generateNormals )
			{
				std::string shader = layer.downsampled ? "PrepareDownsampledNormals" : "PrepareNativeNormals";

				unsigned dimX = DispatchSize( FFX_CACAO_PREPARE_NORMALS_WIDTH, size.deinterleavedDepthBufferWidth );
				unsigned dimY = DispatchSize( FFX_CACAO_PREPARE_NORMALS_HEIGHT, size.deinterleavedDepthBufferHeight );
				Tr2Renderer::RunComputeShader( layer.effect, BlueSharedString( shader.c_str() ), dimX, dimY, 1, renderContext );
			}
			else
			{
				std::string shader = layer.downsampled ? "PrepareDownsampledNormalsFromInputNormals" : "PrepareNativeNormalsFromInputNormals";

				unsigned dimX = DispatchSize( PREPARE_NORMALS_FROM_INPUT_NORMALS_WIDTH, size.deinterleavedDepthBufferWidth );
				unsigned dimY = DispatchSize( PREPARE_NORMALS_FROM_INPUT_NORMALS_HEIGHT, size.deinterleavedDepthBufferHeight );
				Tr2Renderer::RunComputeShader( layer.effect, BlueSharedString( shader.c_str() ), dimX, dimY, 1, renderContext );
			}
		}
	}

	if( deinterleavedDepthTarget->GetMipCount() > 1 )
	{
		GPU_REGION( renderContext, "Prepare mips" );
		deinterleavedDepthTarget->GenerateMipMaps( renderContext );
	}

	if( settings.qualityLevel == FFX_CACAO_QUALITY_HIGHEST )
	{
		{
			GPU_REGION( renderContext, "Interactive base pass" );

			layer.effect->SetParameter( BlueSharedString( "g_SSAOOutput" ), ssaoWorkerTargetA );

			unsigned dimX = DispatchSize( FFX_CACAO_GENERATE_WIDTH, size.ssaoBufferWidth );
			unsigned dimY = DispatchSize( FFX_CACAO_GENERATE_HEIGHT, size.ssaoBufferHeight );

			for( unsigned i = 0; i < SSAO_PASS_COUNT; i++ )
			{
				CR_RETURN_VAL( ApplyConstBuffer( i, renderContext ), {} );
				Tr2Renderer::RunComputeShader( layer.effect, BlueSharedString( "GenerateQ3Base" ), dimX, dimY, 1, renderContext );
			}
		}

		{
			GPU_REGION( renderContext, "Importance map" );

			CR_RETURN_VAL( ApplyConstBuffer( SSAO_PASS_COUNT, renderContext ), {} );

			unsigned dimX = DispatchSize( IMPORTANCE_MAP_WIDTH, size.importanceMapWidth );
			unsigned dimY = DispatchSize( IMPORTANCE_MAP_HEIGHT, size.importanceMapHeight );
			Tr2Renderer::RunComputeShader( layer.effect, BlueSharedString( "PostprocessImportanceMap" ), dimX, dimY, 1, renderContext );
		}
	}

	{
		GPU_REGION( renderContext, "Main pass" );

		layer.effect->SetParameter( BlueSharedString( "g_SSAOOutput" ), ssaoWorkerTargetB );

		unsigned dimX, dimY, dimZ;
		if( settings.qualityLevel <= FFX_CACAO_QUALITY_MEDIUM )
		{
			dimX = DispatchSize( FFX_CACAO_GENERATE_SPARSE_WIDTH, size.ssaoBufferWidth );
			dimX = ( dimX + 4 ) / 5;
			dimY = DispatchSize( FFX_CACAO_GENERATE_SPARSE_HEIGHT, size.ssaoBufferHeight );
			dimZ = 5;
		}
		else
		{
			dimX = DispatchSize( FFX_CACAO_GENERATE_WIDTH, size.ssaoBufferWidth );
			dimY = DispatchSize( FFX_CACAO_GENERATE_HEIGHT, size.ssaoBufferHeight );
			dimZ = 1;
		}

		unsigned quality = std::max( 0, settings.qualityLevel - 1 );
		std::string shader = "GenerateQ" + std::to_string( quality );

		for( unsigned i = 0; i < SSAO_PASS_COUNT; i++ )
		{
			CR_RETURN_VAL( ApplyConstBuffer( i, renderContext ), {} );
			Tr2Renderer::RunComputeShader( layer.effect, BlueSharedString( shader.c_str() ), dimX, dimY, dimZ, renderContext );
		}
	}
	loadCounterBuffer = {};

	if( settings.blurPassCount )
	{
		GPU_REGION( renderContext, "Edge sensitive blur" );

		unsigned dimX = DispatchSize( 4 * FFX_CACAO_BLUR_WIDTH - 2 * settings.blurPassCount, size.ssaoBufferWidth );
		unsigned dimY = DispatchSize( 3 * FFX_CACAO_BLUR_HEIGHT - 2 * settings.blurPassCount, size.ssaoBufferHeight );

		for( unsigned i = 0; i < SSAO_PASS_COUNT; i++ )
		{
			if( settings.qualityLevel == FFX_CACAO_QUALITY_LOWEST && ( i == 1 || i == 2 ) )
			{
				continue;
			}

			CR_RETURN_VAL( ApplyConstBuffer( i, renderContext ), {} );

			std::string passName = "EdgeSensitiveBlur" + std::to_string( settings.blurPassCount );
			Tr2Renderer::RunComputeShader( layer.effect, BlueSharedString( passName.c_str() ), dimX, dimY, 1, renderContext );
		}
	}

	if( layer.downsampled )
	{
		GPU_REGION( renderContext, "Bilateral upsample" );

		CR_RETURN_VAL( ApplyConstBuffer( SSAO_PASS_COUNT, renderContext ), {} );

		unsigned dimX = DispatchSize( 2 * FFX_CACAO_BILATERAL_UPSCALE_WIDTH, size.inputOutputBufferWidth );
		unsigned dimY = DispatchSize( 2 * FFX_CACAO_BILATERAL_UPSCALE_HEIGHT, size.inputOutputBufferHeight );

		std::string shader;
		switch( settings.qualityLevel )
		{
		case FFX_CACAO_QUALITY_LOWEST:
			shader = "UpscaleBilateral5x5Half";
			break;
		case FFX_CACAO_QUALITY_LOW:
		case FFX_CACAO_QUALITY_MEDIUM:
			shader = "UpscaleBilateral5x5NonSmart";
			break;
		default:
			shader = "UpscaleBilateral5x5Smart";
		}

		Tr2Renderer::RunComputeShader( layer.effect, BlueSharedString( shader.c_str() ), dimX, dimY, 1, renderContext );
	}
	else
	{
		GPU_REGION( renderContext, "Apply" );

		CR_RETURN_VAL( ApplyConstBuffer( SSAO_PASS_COUNT, renderContext ), {} );

		unsigned dimX = DispatchSize( FFX_CACAO_APPLY_WIDTH, size.inputOutputBufferWidth );
		unsigned dimY = DispatchSize( FFX_CACAO_APPLY_HEIGHT, size.inputOutputBufferHeight );

		std::string shader;
		switch( settings.qualityLevel )
		{
		case FFX_CACAO_QUALITY_LOWEST:
			shader = "NonSmartHalfApply";
			break;
		case FFX_CACAO_QUALITY_LOW:
			shader = "NonSmartApply";
			break;
		default:
			shader = "Apply";
		}

		Tr2Renderer::RunComputeShader( layer.effect, BlueSharedString( shader.c_str() ), dimX, dimY, 1, renderContext );
	}


	layer.effect->SetParameter( BlueSharedString( "g_DepthIn" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_PrepareNormalsFromNormalsInput" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_PrepareDepthsOut" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_PrepareNormals_NormalOut" ), Tr2TextureAL{} );

	// SSAO pass
	layer.effect->SetParameter( BlueSharedString( "g_ViewspaceDepthSource" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_DeinterleavedNormals" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_LoadCounter" ), loadCounterBuffer );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceMap" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_FinalSSAO" ), Tr2TextureAL{} );

	// Importance pass
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceFinalSSAO" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceOut" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceAIn" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceAOut" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceBIn" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceBOut" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_ImportanceBLoadCounter" ), loadCounterBuffer );

	// Edge sensitive blur pass
	layer.effect->SetParameter( BlueSharedString( "g_EdgeSensitiveBlur_Input" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_EdgeSensitiveBlur_Output" ), Tr2TextureAL{} );

	// Bilateral upsample pass
	layer.effect->SetParameter( BlueSharedString( "g_BilateralUpscaleInput" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_BilateralUpscaleDepth" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_BilateralUpscaleDownscaledDepth" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_BilateralUpscaleOutput" ), Tr2TextureAL{} );

	// Apply pass
	layer.effect->SetParameter( BlueSharedString( "g_ApplyFinalSSAO" ), Tr2TextureAL{} );
	layer.effect->SetParameter( BlueSharedString( "g_ApplyOutput" ), Tr2TextureAL{} );
	return outputTarget;
}

uint32_t Tr2SSAO::Hash( uint32_t n )
{
	return n * ( n ^ ( n >> 15u ) );
}

Tr2GpuResourcePool::Texture Tr2SSAO::ComputeCORTAO( const Tr2TextureAL& depthBuffer, const Tr2TextureAL& normalBuffer, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, bool temporal )
{
	uint32_t width = depthBuffer.GetWidth();
	uint32_t height = depthBuffer.GetHeight();

	Tr2GpuResourcePool::Texture packedBuffer;
	{
		uint32_t mipLevels = 1;
		uint32_t res = max( width, height );
		for( ; mipLevels < 8; mipLevels++ )
		{
			res /= 2;
			if( res < 32 )
			{
				break;
			}
		}
		Tr2BitmapDimensions desc( width, height, mipLevels, PixelFormat::PIXEL_FORMAT_R32_FLOAT );
		packedBuffer = gpuResourcePool.GetTempTexture( "cortao_packed", desc, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE );
	}


	if( !m_cortaoConstantBuffer.IsValid() )
	{
		CR_RETURN_VAL( m_cortaoConstantBuffer.Create( uint32_t( sizeof( CortaoPerObjectData ) ), renderContext.GetPrimaryRenderContext() ), {} );
	}

	CortaoPerObjectData* data;
	CR_RETURN_VAL( m_cortaoConstantBuffer.Lock( (void**)&data, renderContext ), {} );
	{
		Matrix viewMatrix = Tr2Renderer::GetViewTransform();

		Matrix projectionMatrix = Tr2Renderer::GetProjectionTransform();
		Matrix inverseProjectionMatrix = Inverse( projectionMatrix );

		float nearPlane = Tr2Renderer::GetBackClip();
		float farPlane = Tr2Renderer::GetFrontClip();

		data->resolution = Vector4( (float)width, (float)height, 1.0f / (float)width, 1.0f / (float)height );

		data->projectionParams = Vector4(
			+0.5f * projectionMatrix._11,
			-0.5f * projectionMatrix._22,
			-0.5f + projectionMatrix._31 * +0.5f,
			-0.5f + projectionMatrix._32 * -0.5f );

		data->unprojectParams = Vector4(
			+2.0f * inverseProjectionMatrix._11,
			-2.0f * inverseProjectionMatrix._22,
			-inverseProjectionMatrix._11 + inverseProjectionMatrix._41,
			+inverseProjectionMatrix._22 - inverseProjectionMatrix._42 );

		data->depthParams = Vector2( ( nearPlane - farPlane ) / ( nearPlane * farPlane ), 1.0f / nearPlane );

		data->radius = m_cortaoRadius;

		data->normalBias = inverseProjectionMatrix._11 * sqrtf( 2.0f ) / width;


		float maxApparentCircleRadius = m_cortaoMaxBlockerSearchRadius * 2.0f * fminf( inverseProjectionMatrix._11, inverseProjectionMatrix._22 );
		data->maxApparentCircleRadiusCoefficient = maxApparentCircleRadius / sqrtf( maxApparentCircleRadius * maxApparentCircleRadius + 1.0f );

		float circleBias = log2f( projectionMatrix._11 * width * 0.5f );
		data->mipBias = m_cortaoMipBias + circleBias;
		data->radiusMultiplier = m_cortaoStrength;
		data->lookupOccluderRadiusScale = 1.0f;

		if( temporal )
		{
			for( int i = 0; i < 4; i++ )
			{
				m_cortaoRandSeeds[i] = Hash( m_cortaoRandSeeds[i] + rand() );
			}
		}

		data->randomVectorSeedX = m_cortaoRandSeeds[0];
		data->randomVectorSeedY = m_cortaoRandSeeds[1];
		data->randomAngleOffset = ( 6.283185307179586476925286766559f / (float)0xFFFFFFFFu ) * (float)m_cortaoRandSeeds[2];

		data->inverseMaxSlopeWeight = 1.0f / 5.0f;




		//Remove the translation from the view matrix
		Matrix normalMatrix = viewMatrix;
		normalMatrix._41 = 0.0f;
		normalMatrix._42 = 0.0f;
		normalMatrix._43 = 0.0f;

		//bias matrix to convert from [0, 1] to [-1, +1]
		Matrix biasMatrix = ScalingMatrix( 2.0f, 2.0f, 2.0f ) * TranslationMatrix( -1.0f, -1.0f, -1.0f );

		data->normalMatrix = Transpose( biasMatrix * normalMatrix );

		data->mipCount = packedBuffer->GetMipCount();
	}

	CR_RETURN_VAL( m_cortaoConstantBuffer.Unlock( renderContext ), {} );

	CR_RETURN_VAL( renderContext.SetConstants( m_cortaoConstantBuffer, Tr2RenderContextEnum::COMPUTE_SHADER, Tr2Renderer::GetPerObjectVSStartRegister() ), {} );


	PixelFormat outputFormat = m_cortaoBentNormal ? PixelFormat::PIXEL_FORMAT_R8G8B8A8_SNORM : PixelFormat::PIXEL_FORMAT_R8_UNORM;
	auto outputTarget = gpuResourcePool.GetTempTexture( "cortao_output", width, height, outputFormat, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE );


	const char* const SAMPLE_COUNTS[] = {
		"SAMPLE_COUNT_16",
		"SAMPLE_COUNT_12",
		"SAMPLE_COUNT_8",
		"SAMPLE_COUNT_6",
		"SAMPLE_COUNT_4"
	};
	m_cortaoEffect->SetOption( BlueSharedString( "SAMPLE_COUNT" ), BlueSharedString( SAMPLE_COUNTS[static_cast<int>( m_detail.quality )] ) );
	m_cortaoEffect->SetOption( BlueSharedString( "BENT_NORMAL" ), BlueSharedString( m_cortaoBentNormal ? "BENT_NORMAL_ENABLED" : "BENT_NORMAL_DISABLED" ) );

	m_cortaoEffect->SetParameter( BlueSharedString( "NormalBuffer" ), normalBuffer );
	m_cortaoEffect->SetParameter( BlueSharedString( "DepthBuffer" ), depthBuffer );

	//Set all mip levels to ensure no warnings produced by unset resources.
	for( uint32_t i = 0; i < 8; i++ )
	{
		std::string parameterName = std::string( "PackedOutputBuffer" ) + std::to_string( i );
		m_cortaoEffect->SetParameter( BlueSharedString( parameterName.c_str() ), packedBuffer, std::min( i, packedBuffer->GetMipCount() - 1 ) );
	}


	m_cortaoEffect->SetParameter( BlueSharedString( "PackedBuffer" ), packedBuffer );
	m_cortaoEffect->SetParameter( BlueSharedString( "LookupTable" ), m_cortaoLookupTable );
	m_cortaoEffect->SetParameter( BlueSharedString( "OutputBuffer" ), outputTarget );


	{
		GPU_REGION( renderContext, "Pack" );
		uint32_t packWorkGroupSize = 8;
		Tr2Renderer::RunComputeShader( m_cortaoEffect, BlueSharedString( "Pack" ), ( width + packWorkGroupSize - 1 ) / packWorkGroupSize, ( height + packWorkGroupSize - 1 ) / packWorkGroupSize, 1, renderContext );
	}
	{
		GPU_REGION( renderContext, "Main pass" );
		uint32_t mainPassWorkGroupSize = 16;
		Tr2Renderer::RunComputeShader( m_cortaoEffect, BlueSharedString( "MainPass" ), ( width + mainPassWorkGroupSize - 1 ) / mainPassWorkGroupSize, ( height + mainPassWorkGroupSize - 1 ) / mainPassWorkGroupSize, 1, renderContext );
	}

	if( m_cortaoBlur )
	{
		Tr2BitmapDimensions desc( width, height, 1, outputFormat );
		auto blurBuffer = gpuResourcePool.GetTempTexture( "cortao_blur", desc, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE );

		uint32_t blurWorkGroupSize = 8;
		{
			GPU_REGION( renderContext, "Blur" );

			m_cortaoBlurEffect->SetParameter( BlueSharedString( "PackedDataBuffer" ), packedBuffer );

			m_cortaoBlurEffect->SetOption( BlueSharedString( "BENT_NORMAL" ), BlueSharedString( m_cortaoBentNormal ? "BENT_NORMAL_ENABLED" : "BENT_NORMAL_DISABLED" ) );

			{
				GPU_REGION( renderContext, "Pass 1" );
				m_cortaoBlurEffect->SetOption( BlueSharedString( "MODE" ), BlueSharedString( temporal ? "MODE_TEMPORAL" : "MODE_PASS1" ) );
				m_cortaoBlurEffect->SetParameter( BlueSharedString( "SSAOInputBuffer" ), outputTarget );
				m_cortaoBlurEffect->SetParameter( BlueSharedString( "SSAOOutputBuffer" ), blurBuffer );
				Tr2Renderer::RunComputeShader( m_cortaoBlurEffect, BlueSharedString( "Blur" ), ( width + blurWorkGroupSize - 1 ) / blurWorkGroupSize, ( height + blurWorkGroupSize - 1 ) / blurWorkGroupSize, 1, renderContext );
			}
			{
				GPU_REGION( renderContext, "Pass 2" );
				m_cortaoBlurEffect->SetOption( BlueSharedString( "MODE" ), BlueSharedString( temporal ? "MODE_TEMPORAL" : "MODE_PASS2" ) );
				m_cortaoBlurEffect->SetParameter( BlueSharedString( "SSAOInputBuffer" ), blurBuffer );
				m_cortaoBlurEffect->SetParameter( BlueSharedString( "SSAOOutputBuffer" ), outputTarget );
				Tr2Renderer::RunComputeShader( m_cortaoBlurEffect, BlueSharedString( "Blur" ), ( width + blurWorkGroupSize - 1 ) / blurWorkGroupSize, ( height + blurWorkGroupSize - 1 ) / blurWorkGroupSize, 1, renderContext );
			}
		}
	}
	return outputTarget;
}
