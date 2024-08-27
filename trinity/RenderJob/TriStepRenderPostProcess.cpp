#include "StdAfx.h"
#include "TriStepRenderPostProcess.h"
#include "PostProcess/Tr2PostProcess2.h"
#include "Shader/Parameter/TriTextureParameter.h"
#include "Include/TriMath.h"

// FidelityFX headers
#define A_CPU
#include "ffx_a.h"
#include "ffx_cas.h"

namespace
{
const uint32_t HISTOGRAM_TILE_SIZE_X = 16;
const uint32_t HISTOGRAM_TILE_SIZE_Y = 16;
const uint32_t NUM_TILES_PER_THREAD_GROUP = 256;

ITr2TextureProvider* PLACEHOLDER = nullptr;

void DrawInto( Tr2TextureAL& dest, Tr2LoadAction::Type loadAction, Tr2TextureAL& src, Tr2RenderContext& renderContext )
{
	renderContext.RenderPassHint( { loadAction, Tr2StoreAction::STORE }, {} );
	renderContext.m_esm.PushRenderTarget( dest );
	Tr2Renderer::DrawTexture( renderContext, src );
	renderContext.m_esm.PopRenderTarget();
}

void DrawPartiallyInto( Tr2TextureAL& dest, Tr2LoadAction::Type loadAction, Tr2TextureAL& src, Tr2RenderContext& renderContext, const Vector2 tlTextureCoords = Vector2( 0, 0 ), const Vector2 brTextureCoords = Vector2( 1, 1 ) )
{
	renderContext.RenderPassHint( { loadAction, Tr2StoreAction::STORE }, {} );
	renderContext.m_esm.PushRenderTarget( dest );
	Tr2Renderer::DrawTexture( renderContext, src, tlTextureCoords, brTextureCoords, Tr2Blitter::FILTER_LINEAR );
	renderContext.m_esm.PopRenderTarget();
}

void DrawInto( Tr2TextureAL& dest, Tr2LoadAction::Type loadAction, Tr2Effect* effect, Tr2RenderContext& renderContext )
{
	renderContext.RenderPassHint( { loadAction, Tr2StoreAction::STORE }, {} );
	renderContext.m_esm.PushRenderTarget( dest );
	Tr2Renderer::DrawScreenQuad( renderContext, effect );
	renderContext.m_esm.PopRenderTarget();
}

}

namespace PostProcessBlur
{

BlurContext CreateBlurContext( float outputSize )
{
	BlurContext context;

	context.outputSize = outputSize;

	context.type = BlurType::BT_Big;
	context.channel = BlurChannel::BC_rgba;
	context.process = BlurProcess::BP_None;
	context.finalize = BlurFinalize::BF_None;
	return context;
}

BlurContext CreateBlurContext( float outputSize, BlurType type )
{
	BlurContext context;

	context.type = type;
	context.outputSize = outputSize;

	context.channel = BlurChannel::BC_rgba;
	context.process = BlurProcess::BP_None;
	context.finalize = BlurFinalize::BF_None;
	return context;
}

BlurContext CreateBlurContext( BlurType type, BlurChannel channel, float outputSize )
{
	BlurContext context;
	context.type = type;
	context.channel = channel;
	context.outputSize = outputSize;

	context.process = BlurProcess::BP_None;
	context.finalize = BlurFinalize::BF_None;
	return context;
}

BlurContext CreateBlurContext( BlurType type, BlurChannel channel, BlurProcess process, float outputSize )
{
	BlurContext context;
	context.type = type;
	context.channel = channel;
	context.outputSize = outputSize;
	context.process = process;

	context.finalize = BlurFinalize::BF_None;
	return context;
}

BlurContext CreateBlurContext( BlurType type, BlurChannel channel, BlurProcess process, BlurFinalize finalize, float outputSize )
{
	BlurContext context;
	context.type = type;
	context.channel = channel;
	context.outputSize = outputSize;
	context.process = process;
	context.finalize = finalize;

	return context;
}



BlueSharedString GetBlurChannelOptionValue( BlurChannel channel )
{
	switch( channel )
	{
	case BlurChannel::BC_rgba:
		return BlueSharedString( "BLUR_CHANNEL_RGBA" );
	case BlurChannel::BC_r:
		return BlueSharedString( "BLUR_CHANNEL_R" );
	case BlurChannel::BC_g:
		return BlueSharedString( "BLUR_CHANNEL_G" );
	case BlurChannel::BC_b:
		return BlueSharedString( "BLUR_CHANNEL_B" );
	case BlurChannel::BC_a:
		return BlueSharedString( "BLUR_CHANNEL_A" );
	default:
		return BlueSharedString( "BLUR_CHANNEL_RGBA" );
	}
}

BlueSharedString GetBlurTypeOptionValue( BlurType type )
{
	if( type == BlurType::BT_Big )
	{
		return BlueSharedString( "BLUR_TYPE_BIG" );
	}
	return BlueSharedString( "BLUR_TYPE_SMALL" );
}

BlueSharedString GetProcessTypeOptionValue( BlurProcess process )
{
	if( process == BlurProcess::BP_Maximum )
	{
		return BlueSharedString( "BLUR_PROCESS_TYPE_MAXIMUM" );
	}
	if( process == BlurProcess::BP_Minimum )
	{
		return BlueSharedString( "BLUR_PROCESS_TYPE_MINIMUM" );
	}
	return BlueSharedString( "BLUR_PROCESS_TYPE_NONE" );
}

BlueSharedString GetFinalizeTypeOptionValue( BlurFinalize finalize )
{
	if( finalize == BlurFinalize::BF_MaxOfAllChannels )
	{
		return BlueSharedString( "BLUR_FINALIZE_TYPE_MAX_OF_ALL_CHANNELS" );
	}
	return BlueSharedString( "BLUR_FINALIZE_TYPE_NONE" );
}

}

namespace AMDSharpening
{
	Vector4 AsVector( uintfloat4 v )
	{
		return Vector4( v.f[0], v.f[1], v.f[2], v.f[3] );
	}
}

TriStepRenderPostProcess::TriStepRenderPostProcess( IRoot* lockobj ) :
	m_quality( HIGH ),
	m_tilesX( 0 ),
	m_tilesY( 0 ),
	m_localHistogramCount( 0 ),
	m_mergeHistogramXDim( 0 ),
	m_desaturateEnabled( false ),
	m_fadeEnabled( false ),
	m_lutEnabled( false ),
	m_vignetteEnabled( false ),
	m_sceneDirty( false ),
	m_lastFrameTime( std::numeric_limits<long long>().max() ),
	m_upscalingContextID( Tr2UpscalingAL::INVALID_CONTEXT_ID )
{
	m_renderInfo.CreateInstance();
	m_tonemappingEffect.CreateInstance();
	m_tonemappingEffect->StartUpdate();
	m_tonemappingEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/ToneMapping.fx" );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteDetailScroll" ), Vector4( 0.0, 0.0, 0.0, 0.0 ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "GrainColorAmount" ), 0.600000023842f );
	m_tonemappingEffect->SetOption( BlueSharedString( "TONE_MAPPING_TOGGLE" ), BlueSharedString( "TONE_MAPPING_ENABLED" ) );
	m_tonemappingEffect->SetOption( BlueSharedString( "DESATURATE_TOGGLE" ), BlueSharedString( "DESATURATE_DISABLED" ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteColor" ), Vector4( 1.0, 1.0, 1.0, 1.0 ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteSineRange" ), Vector4( 0.0, 1.0, 0.0, 0.0 ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "GrainIntensity" ), 0.00300000002608f );
	m_tonemappingEffect->SetOption( BlueSharedString( "COLORED_GRAIN_TOGGLE" ), BlueSharedString( "COLORED_GRAIN_DISABLED" ) );
	m_tonemappingEffect->SetOption( BlueSharedString( "LUT_TOGGLE" ), BlueSharedString( "LUT_DISABLED" ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "FadeAmount" ), 0.0f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "GrimeWeight" ), 0.0f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureAdjust" ), 1.0f );
	m_tonemappingEffect->SetOption( BlueSharedString( "DYNAMIC_EXPOSURE_TOGGLE" ), BlueSharedString( "DYNAMIC_EXPOSURE_DISABLED" ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "GrainSize" ), 2.0f );
	m_tonemappingEffect->SetOption( BlueSharedString( "VIGNETTE_TOGGLE" ), BlueSharedString( "VIGNETTE_DISABLED" ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "GrainLuminanceExponent" ), 0.20000000298f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "FadeColor" ), Vector4( 0.0, 0.0, 0.0, 0.0 ) );
	m_tonemappingEffect->SetOption( BlueSharedString( "FILM_GRAIN_TOGGLE" ), BlueSharedString( "FILM_GRAIN_DISABLED" ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureMiddleValue" ), 0.5f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteDetailSize" ), Vector4( 16.0, 16.0, 16.0, 16.0 ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "LUTInfluence" ), 0.0f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteSineFrequency" ), 1.0f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureInfluence" ), 1.0f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "BloomBrightness" ), 0.20000000298f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteIntensity" ), Vector4( 0.0, 0.0, 0.0, 0.0 ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "SaturationFactor" ), 1.0f );
	m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "Grime" ), "res:/texture/global/black.dds" );
	m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "TexLUT" ), "res:/dx9/scene/postprocess/LUTdefault.dds" );
	m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "VignetteDetail" ), "res:/texture/global/white.dds" );
	m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "VignetteShape" ), "res:/texture/global/black.dds" );
	m_tonemappingEffect->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
	m_tonemappingEffect->SetParameter( BlueSharedString( "BlitOriginal" ), PLACEHOLDER );

	m_tonemappingEffect->SetParameter( BlueSharedString( "ShoulderStrength" ), 0.125f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "LinearStrength" ), 0.25f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "LinearAngle" ), 0.1f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "ToeStrength" ), 0.15f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "ToeNumerator" ), 0.021f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "ToeDenominator" ), 0.3f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "WhiteScale" ), 2.5f );
	m_tonemappingEffect->SetParameter( BlueSharedString( "SplitScreenRatio" ), 1.0f ); // TODO: review. 1.0 == new settings. we may want to remove this once we've found settings that we're happy with
	m_tonemappingEffect->SetParameter( BlueSharedString( "AutoSwipe" ), Vector4( 0.0, 1000.0, 0.0, 0.0 ) );

	m_tonemappingEffect->EndUpdate();

	m_reactiveMaskEffect.CreateInstance();
	m_reactiveMaskEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/ReactiveMask.fx" );
}

TriStepRenderPostProcess::~TriStepRenderPostProcess( void )
{
	if( m_upscalingContextID != Tr2UpscalingAL::INVALID_CONTEXT_ID )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		auto context = renderContext.GetUpscalingContext(m_upscalingContextID);
		if( context )
		{
			context->SetHudLessTexture( nullptr );
		}
	}
	if( m_scene )
	{
		m_scene->SetVelocityMap( nullptr );
	}

	m_scene = nullptr;
}

void TriStepRenderPostProcess::py__init__( EveSpaceScene* scene, Tr2RenderTarget* source, Tr2RenderTarget* opaque_source )
{
	if( scene == nullptr )
	{
		return;
	}

	m_scene = scene;
	m_sceneDirty = true;

	if( !source || !source->IsValid() )
	{
		CCP_LOGERR( "TriStepRenderPostProcess: source is invalid" );
	}

	SetRenderTarget( source );
	m_opaqueColorBuffer = opaque_source;
	SetupVelocityMap();
	m_lastFrameTime = BeOS->GetCurrentFrameTime();
}

void SetDirtyIfNotNull( Tr2PPEffect* effect )
{
	if( nullptr != effect )
	{
		effect->SetDirty( true );
	}
}

void TriStepRenderPostProcess::SetupVelocityMap()
{
	auto sourceBuffer = m_renderInfo->GetSourceBuffer();

	if( sourceBuffer && sourceBuffer->IsValid() )
	{
		m_velocityBuffer.CreateInstance();
		m_velocityBuffer->SetName( "VelocityMap" );
		m_velocityBuffer->Create( sourceBuffer->GetWidth(), sourceBuffer->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_FLOAT, sourceBuffer->GetMsaaType(), 0 );
	}
}

bool TriStepRenderPostProcess::OnModified( Be::Var* value )
{
	m_sceneDirty = true;
	return true;
}

TriStepResult TriStepRenderPostProcess::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_renderInfo->Setup( renderContext ) )
	{
		return RS_FAILED;
	}

	auto sourceBuffer = m_renderInfo->GetSourceBuffer();

	if( !sourceBuffer || !sourceBuffer->IsValid() )
	{
		return RS_OK;
	}

	if( m_scene == nullptr )
	{
		return RS_OK;
	}

	m_renderInfo->SetRenderSize( sourceBuffer->GetWidth(), sourceBuffer->GetHeight() );

	GPU_REGION( renderContext, "Post-processing" );

	Tr2PostProcess2Ptr postProcess = m_scene->GetPostProcess();

	Tr2PPGodRaysEffect* godrays = nullptr;
	Tr2PPBloomEffect* bloom = nullptr;
	Tr2PPSignalLossEffect* signalLoss = nullptr;
	Tr2PPDynamicExposureEffectPtr dynamicExposure = nullptr;
	Tr2PPFilmGrainEffectPtr filmGrain = nullptr;
	Tr2PPDesaturateEffectPtr desaturate = nullptr;
	Tr2PPFadeEffectPtr fade = nullptr;
	Tr2PPLutEffectPtr lut = nullptr;
	Tr2PPVignetteEffectPtr vignette = nullptr;
	Tr2PPFogEffectPtr fog = nullptr;
	Tr2PPTaaEffectPtr taa = nullptr;
	Tr2PPDepthOfFieldEffectPtr dof = nullptr;

	if( postProcess != nullptr )
	{
		// filter by quality
		switch( m_quality )
		{
		case HIGH:
			godrays = postProcess->GetGodRays();
			filmGrain = postProcess->GetFilmGrain();
			fog = postProcess->GetFog();
			dynamicExposure = postProcess->GetDynamicExposure();
		case MEDIUM:
			bloom = postProcess->GetBloom();
			desaturate = postProcess->GetDesaturate();
			lut = postProcess->GetLut();
			vignette = postProcess->GetVignette();
		case LOW:
			signalLoss = postProcess->GetSignalLoss();
			fade = postProcess->GetFade();
		default:
			taa = postProcess->GetTaa();
			break;
		}

		if( g_postprocessDofEnabled )
		{
			dof = postProcess->GetDepthOfField();
		}
	}

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	renderContext.m_esm.PushRenderTarget();
	renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );

	if( m_sceneDirty )
	{
		SetDirtyIfNotNull( godrays );
		SetDirtyIfNotNull( bloom );
		SetDirtyIfNotNull( signalLoss );
		SetDirtyIfNotNull( dynamicExposure );
		SetDirtyIfNotNull( filmGrain );
		SetDirtyIfNotNull( desaturate );
		SetDirtyIfNotNull( fade );
		SetDirtyIfNotNull( lut );
		SetDirtyIfNotNull( vignette );
		SetDirtyIfNotNull( fog );
		SetDirtyIfNotNull( taa );
		SetDirtyIfNotNull( dof );
	}
	// Processing effects will set velocity map if it is needed
	m_scene->SetVelocityMap( nullptr );

	if( m_upscalingContextID == Tr2UpscalingAL::INVALID_CONTEXT_ID )
	{
		m_upscalingContextID = Tr2Renderer::GetUpscalingContextID();
	}
	
	auto upscalingInfo = renderContext.GetPrimaryRenderContext().GetUpscalingInfo( m_upscalingContextID );

	auto upscalingEnabled = upscalingInfo.technique != Tr2UpscalingAL::NONE;
	Tr2PostProcessRenderInfo::Texture output;
	if( upscalingEnabled )
	{
		output = m_renderInfo->GetTempTexture( upscalingInfo.displayWidth, upscalingInfo.displayHeight );
		if( upscalingInfo.temporal )
		{
			taa = nullptr;
		}
	}
	else
	{
		output = m_renderInfo->GetTempTexture();
	}

	ProcessSharpening( !upscalingInfo.hasSharpening, output->GetWidth(), output->GetHeight(), upscalingInfo.upscalingAmount );


	// Always copy
	auto nonMsaaSource = m_renderInfo->GetTempTexture();
	sourceBuffer->GetRenderTarget().Resolve( *nonMsaaSource, renderContext );

	if( ProcessFog( fog ) )
	{
		RenderFog( nonMsaaSource, renderContext, fog );
	}

	if( ProcessGodRays( godrays ) )
	{
		RenderGodRays( nonMsaaSource, renderContext, godrays );
	}

	if( ProcessDepthOfField( renderContext, dof ) )
	{
		bool temporal = upscalingInfo.temporal || ( taa && taa->IsActive() );
		RenderDepthOfField( nonMsaaSource, renderContext, dof, temporal, upscalingInfo.upscalingAmount );
	}

	bool hasDynamicExposure = ProcessDynamicExposure( renderContext, dynamicExposure, bloom );
	if( ProcessTaa( taa ) )
	{
		RenderTaa( nonMsaaSource, renderContext, taa, dynamicExposure );
	}

	if( hasDynamicExposure )
	{
		RenderDynamicExposure( nonMsaaSource, renderContext, dynamicExposure );
	}

	Tr2PostProcessRenderInfo::Texture upscaledSource;

	if( upscalingInfo.temporal )
	{
		auto upscalingContext = renderContext.GetPrimaryRenderContext().GetUpscalingContext( m_upscalingContextID );
		upscalingContext->SetHudLessTexture(output->GetTexture());
		upscaledSource = RenderUpscaling( nonMsaaSource, renderContext, upscalingContext, dynamicExposure );
		// upscale the temp textures so everything hence forth is correct
		uint32_t w, h;
		upscalingContext->GetDisplayDimensions( w, h );
		m_renderInfo->SetRenderSize( w, h );
		// need to reset the perframedata so we have the correct viewport size etc
		m_scene->ApplyUpscalingToPerFrameData( w, h, renderContext );
	}
	else
	{
		upscaledSource = nonMsaaSource;
	}

	// this needs to be after dynamic exposure, since bloom can be exposure dependent
	Tr2PostProcessRenderInfo::Texture bloomTexture;
	if( ProcessBloom( bloom, dynamicExposure ) )
	{
		bloomTexture = RenderBloom( upscaledSource, renderContext, bloom );
	}

	m_tonemappingEffect->SetParameter( BlueSharedString( "BlitCurrent" ), bloomTexture ? bloomTexture.GetRenderTarget() : m_renderInfo->GetBlackTexture() );
	m_tonemappingEffect->SetParameter( BlueSharedString( "BlitOriginal" ), upscaledSource );

	ProcessDesaturate( desaturate );
	ProcessFade( fade );
	ProcessLut( lut );
	ProcessVignette( vignette );

	bool doGrain = ProcessFilmGrain( filmGrain );
	if( !upscalingInfo.temporal || doGrain )
	{
		if( upscalingEnabled && !upscalingInfo.temporal )
		{
			
			auto temp = m_renderInfo->GetTempTexture();

			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
			DrawInto( *temp, Tr2LoadAction::DONT_CARE, m_tonemappingEffect, renderContext );
			
			auto upscalingContext = renderContext.GetPrimaryRenderContext().GetUpscalingContext( m_upscalingContextID );
			
			output = RenderUpscaling( temp, renderContext, upscalingContext, dynamicExposure );
			
			// upscale the temp textures so everything hence forth is correct
			uint32_t w, h;
			upscalingContext->GetDisplayDimensions( w, h );
			m_renderInfo->SetRenderSize( w, h );
			// need to reset the perframedata so we have the correct viewport size etc
			m_scene->ApplyUpscalingToPerFrameData( w, h, renderContext );
		}
		else
		{
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
			DrawInto( *output, Tr2LoadAction::DONT_CARE, m_tonemappingEffect, renderContext );
		}
		output = RenderSharpening( output, renderContext );

		if( doGrain )
		{
			RenderFilmGrain( output, renderContext, filmGrain );
		}
		else
		{
			Tr2Renderer::DrawTexture( renderContext, *output );
		}
	}
	else
	{
		Tr2Renderer::DrawTexture( renderContext, m_tonemappingEffect, *output, Vector2( 0, 0 ), Vector2( 1, 1 ) );
		output = RenderSharpening( output, renderContext );
	}

	if( ProcessSignalLoss( signalLoss ) )
	{
		RenderSignalLoss( output, renderContext, signalLoss );
	}

	renderContext.m_esm.PopDepthStencilBuffer();
	renderContext.m_esm.PopRenderTarget();
	
	m_sceneDirty = false;
	return RS_OK;
}

void TriStepRenderPostProcess::SetupExposureConversion( bool enable, float middleValue )
{
	if( enable )
	{
		if( !m_exposureTexture || !m_dynamicExposureToTextureShader )
		{
			m_exposureTexture = m_renderInfo->GetTempTexture( 1, 1, Tr2RenderContextEnum::EX_BIND_UNORDERED_ACCESS, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT );
			m_dynamicExposureToTextureShader.CreateInstance();
			m_dynamicExposureToTextureShader->StartUpdate();
			m_dynamicExposureToTextureShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/ExposureToTexture.fx" );
			m_dynamicExposureToTextureShader->SetParameter( BlueSharedString( "ExposureMiddleValue" ), middleValue );
			m_dynamicExposureToTextureShader->EndUpdate();
		}
	}
	else
	{
		m_dynamicExposureToTextureShader = nullptr;
		m_exposureTexture = nullptr;
	}
}

void TriStepRenderPostProcess::ProcessSharpening( bool enable, uint32_t displayWidth, uint32_t displayHeight, float upscalingAmount )
{
	if( enable && !m_fidelityFxCasShader )
	{
		float casIntensity = 0.5f;

		m_fidelityFxCasShader.CreateInstance();
		m_fidelityFxCasShader->StartUpdate();
		m_fidelityFxCasShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/CAS.fx" );
		
		AF1 outWidth = static_cast<AF1>( displayWidth );
		AF1 outHeight = static_cast<AF1>( displayHeight );
		
		AMDSharpening::CASConstants casConst;

		CasSetup( casConst.const0.u, casConst.const1.u, casIntensity, outWidth, outHeight, outWidth, outHeight );

		m_fidelityFxCasShader->SetParameter( BlueSharedString( "InputTexture" ), PLACEHOLDER );
		m_fidelityFxCasShader->SetParameter( BlueSharedString( "OutputTexture" ), PLACEHOLDER );
		m_fidelityFxCasShader->SetParameter( BlueSharedString( "const0" ), AMDSharpening::AsVector( casConst.const0 ) );
		m_fidelityFxCasShader->SetParameter( BlueSharedString( "const1" ), AMDSharpening::AsVector( casConst.const1 ) );

		m_fidelityFxCasShader->EndUpdate();

	}
}

Tr2PostProcessRenderInfo::Texture TriStepRenderPostProcess::RenderSharpening( Tr2PostProcessRenderInfo::Texture& input, Tr2RenderContext& renderContext )
{
	if( m_fidelityFxCasShader )
	{
		GPU_REGION( renderContext, "CAS Sharpening" );

		static const uint32_t CAS_THREAD_GROUP_WORK_REGION_DIM = 16;
		Tr2PostProcessRenderInfo::Texture output = m_renderInfo->GetTempTexture( 1.0f, Tr2RenderContextEnum::EX_BIND_UNORDERED_ACCESS );

		m_fidelityFxCasShader->SetParameter( BlueSharedString( "InputTexture" ), input );
		m_fidelityFxCasShader->SetParameter( BlueSharedString( "OutputTexture" ), output );

		auto renderWidth = output->GetWidth();
		auto renderHeight = output->GetHeight();

		auto dispatchX = ( renderWidth + ( CAS_THREAD_GROUP_WORK_REGION_DIM - 1 ) ) / CAS_THREAD_GROUP_WORK_REGION_DIM;
		auto dispatchY = ( renderHeight + ( CAS_THREAD_GROUP_WORK_REGION_DIM - 1 ) ) / CAS_THREAD_GROUP_WORK_REGION_DIM;
		Tr2Renderer::RunComputeShader( m_fidelityFxCasShader, dispatchX, dispatchY, 1, renderContext );
		return output;
	}
	return input;
}

// Helper function to blur certain channel of a source render target to a destination render target with a blur type (Big/Small)
void TriStepRenderPostProcess::Blur( Tr2RenderTarget& dest, Tr2RenderTarget& src, Tr2RenderContext& renderContext, PostProcessBlur::BlurContext& blurContext )
{
	GPU_REGION( renderContext, "Blur" );

	auto lookup = m_blurEffects.find( blurContext.Hash() );

	// Horizontal and vertical blur effects, IN THAT ORDER!
	std::pair<Tr2EffectPtr, Tr2EffectPtr> effects;
	if( lookup == m_blurEffects.end() )
	{
		// create the effect
		effects.first.CreateInstance();
		effects.first->StartUpdate();
		effects.first->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Blur.fx" );

		effects.second.CreateInstance();
		effects.second->StartUpdate();
		effects.second->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Blur.fx" );
		effects.second->SetParameter( BlueSharedString( "Direction" ), Vector2( 0, 1 ) );

		BlueSharedString blurTypeOption = PostProcessBlur::GetBlurTypeOptionValue( blurContext.type );
		effects.first->SetOption( BlueSharedString( "BLUR_TYPE" ), blurTypeOption );
		effects.second->SetOption( BlueSharedString( "BLUR_TYPE" ), blurTypeOption );

		BlueSharedString blurChannelOption = PostProcessBlur::GetBlurChannelOptionValue( blurContext.channel );
		effects.first->SetOption( BlueSharedString( "BLUR_CHANNEL" ), blurChannelOption );
		effects.second->SetOption( BlueSharedString( "BLUR_CHANNEL" ), blurChannelOption );

		BlueSharedString blurChannelProcessOption = PostProcessBlur::GetProcessTypeOptionValue( blurContext.process );
		effects.first->SetOption( BlueSharedString( "BLUR_PROCESS_TYPE" ), blurChannelProcessOption );
		effects.second->SetOption( BlueSharedString( "BLUR_PROCESS_TYPE" ), blurChannelProcessOption );

		effects.first->SetOption( BlueSharedString( "BLUR_FINALIZE_TYPE" ), PostProcessBlur::GetFinalizeTypeOptionValue( PostProcessBlur::BF_None ) );
		effects.second->SetOption( BlueSharedString( "BLUR_FINALIZE_TYPE" ), PostProcessBlur::GetFinalizeTypeOptionValue( blurContext.finalize ) );

		effects.first->EndUpdate();
		effects.second->EndUpdate();
		m_blurEffects.insert( std::pair( blurContext.Hash(), effects ) );
	}
	else
	{
		effects = lookup->second;
	}

	auto rt2 = m_renderInfo->GetTempTexture( blurContext.outputSize, Tr2RenderContextEnum::EX_NONE, src.GetFormat() );

	effects.first->SetParameter( BlueSharedString( "BlitCurrent" ), &src );
	effects.second->SetParameter( BlueSharedString( "BlitCurrent" ), rt2 );

	DrawInto( *rt2, Tr2LoadAction::DONT_CARE, effects.first, renderContext );
	DrawInto( dest, Tr2LoadAction::DONT_CARE, effects.second, renderContext );
}

void TriStepRenderPostProcess::DownSampleDepth( Tr2RenderContext& renderContext, Tr2RenderTarget* destination )
{
	if( !m_downsampleDepthEffect )
	{
		m_downsampleDepthEffect.CreateInstance();
		m_downsampleDepthEffect->StartUpdate();
		m_downsampleDepthEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/DownsampleDepth.fx" );
		m_downsampleDepthEffect->EndUpdate();
	}

	DrawInto( *destination, Tr2LoadAction::DONT_CARE, m_downsampleDepthEffect, renderContext );
}

bool TriStepRenderPostProcess::ProcessBloom( Tr2PPBloomEffect* bloom, Tr2PPDynamicExposureEffect* dynamicExposure )
{
	if( bloom && bloom->IsActive() )
	{
		bool exposureDependant = bloom->m_exposureDependency && m_exposure != nullptr;

		if( m_bloomHighPassFilter == nullptr )
		{
			m_bloomHighPassFilter.CreateInstance();
			m_bloomHighPassFilter->StartUpdate();
			m_bloomHighPassFilter->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/HighPassFilter.fx" );
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "LuminanceThreshold" ), bloom->m_luminanceThreshold );
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "LuminanceScale" ), bloom->m_luminanceScale );
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );

			bool hasDynamicExposure = dynamicExposure != nullptr && dynamicExposure->IsActive();
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "ExposureDependency" ), bloom->m_exposureDependency && hasDynamicExposure ? 1.0f : 0.0f );
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "Exposure" ), m_exposure );
			m_bloomHighPassFilter->EndUpdate();

			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "BloomBrightness" ), bloom->m_bloomBrightness );
			m_tonemappingEffect->SetParameter( BlueSharedString( "GrimeWeight" ), bloom->m_grimeWeight );
			m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "Grime" ), bloom->m_grimePath.c_str() );
			m_tonemappingEffect->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );

			m_tonemappingEffect->EndUpdate();

			bloom->SetDirty( false );
		}
		else if( bloom->IsDirty() )
		{
			m_bloomHighPassFilter->StartUpdate();
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "LuminanceThreshold" ), bloom->m_luminanceThreshold );
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "LuminanceScale" ), bloom->m_luminanceScale );
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "ExposureDependency" ), exposureDependant ? 1.0f : 0.0f );
			if( exposureDependant )
			{
				m_bloomHighPassFilter->SetParameter( BlueSharedString( "Exposure" ), m_exposure );
			}
			m_bloomHighPassFilter->EndUpdate();

			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "BloomBrightness" ), bloom->m_bloomBrightness );
			m_tonemappingEffect->SetParameter( BlueSharedString( "GrimeWeight" ), bloom->m_grimeWeight );

			TriTextureParameter* resource = dynamic_cast<TriTextureParameter*>( m_tonemappingEffect->GetResourceByName( "Grime" ) );
			resource->SetResourcePath( bloom->m_grimePath.c_str() );

			m_tonemappingEffect->EndUpdate();

			bloom->SetDirty( false );
		}
	}
	else
	{
		if( m_bloomHighPassFilter != nullptr )
		{
			m_bloomHighPassFilter = nullptr;

			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetBlackTexture() );
			m_tonemappingEffect->SetParameter( BlueSharedString( "Grime" ), m_renderInfo->GetBlackTexture() );
			m_tonemappingEffect->EndUpdate();
		}
	}

	return bloom != nullptr && bloom->IsActive();
}

Tr2PostProcessRenderInfo::Texture TriStepRenderPostProcess::RenderBloom( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPBloomEffect* bloom )
{
	GPU_REGION( renderContext, "Bloom" );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	auto rt1 = m_renderInfo->GetTempTexture( 0.5f );
	m_bloomHighPassFilter->SetParameter( BlueSharedString( "BlitCurrent" ), dest );
	DrawInto( *rt1, Tr2LoadAction::DONT_CARE, m_bloomHighPassFilter, renderContext );

	auto blurContext = PostProcessBlur::CreateBlurContext( 0.5f );
	Blur( *rt1, *rt1, renderContext, blurContext );

	return rt1;
}


bool TriStepRenderPostProcess::ProcessGodRays( Tr2PPGodRaysEffect* godrays )
{
	if( godrays && godrays->IsActive() )
	{
		if( m_godrayEffect == nullptr )
		{
			m_godrayEffect.CreateInstance();
			m_godrayEffect->StartUpdate();
			m_godrayEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Godrays.fx" );
			m_godrayEffect->SetParameter( BlueSharedString( "Color" ), Vector4( godrays->m_godRayColor ) );
			m_godrayEffect->SetParameter( BlueSharedString( "Intensity" ), Vector4( godrays->m_intensity, 0.0f, 1.0f, 1.0f ) );
			m_godrayEffect->SetParameter( BlueSharedString( "grFactors" ), godrays->grFactors );
			m_godrayEffect->AddResourceTexture2D( BlueSharedString( "NoiseTexMap" ), godrays->m_noiseTexturePath.c_str() );
			m_godrayEffect->SetParameter( BlueSharedString( "DepthMap" ), PLACEHOLDER );
			m_godrayEffect->EndUpdate();
			godrays->SetDirty( false );
		}
		else if( godrays->IsDirty() )
		{
			m_godrayEffect->StartUpdate();
			m_godrayEffect->SetParameter( BlueSharedString( "Color" ), Vector4( godrays->m_godRayColor ) );
			m_godrayEffect->SetParameter( BlueSharedString( "Intensity" ), Vector4( godrays->m_intensity, 0.0f, 1.0f, 1.0f ) );
			m_godrayEffect->SetParameter( BlueSharedString( "grFactors" ), godrays->grFactors );

			TriTextureParameter* resource = dynamic_cast<TriTextureParameter*>( m_godrayEffect->GetResourceByName( "NoiseTexMap" ) );
			resource->SetResourcePath( godrays->m_noiseTexturePath.c_str() );

			m_godrayEffect->EndUpdate();
			godrays->SetDirty( false );
		}
	}
	else
	{
		m_godrayEffect = nullptr;
	}

	return godrays != nullptr && godrays->IsActive();
}


void TriStepRenderPostProcess::RenderGodRays( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPGodRaysEffect* godrays )
{
	GPU_REGION( renderContext, "Godrays" );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	// Downsample depth
	auto rt1 = m_renderInfo->GetTempTexture( 0.5f );
	DownSampleDepth( renderContext, rt1 );

	// God rays
	auto rt2 = m_renderInfo->GetTempTexture( 0.5f );
	m_godrayEffect->SetParameter( BlueSharedString( "DepthMap" ), rt1 );
	renderContext.m_esm.PushRenderTarget( *rt2 );
	renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 0 ); // clear is needed because godray vertex shader can opt out of rendering
	Tr2Renderer::DrawScreenQuad( renderContext, m_godrayEffect );
	renderContext.m_esm.PopRenderTarget();

	// Blit
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
	DrawInto( *dest, Tr2LoadAction::LOAD, *rt2, renderContext );
}


bool TriStepRenderPostProcess::ProcessSignalLoss( Tr2PPSignalLossEffect* signalLoss )
{
	if( signalLoss && signalLoss->IsActive() )
	{
		if( m_signalLossEffect == nullptr )
		{
			m_signalLossEffect.CreateInstance();
			m_signalLossEffect->StartUpdate();
			m_signalLossEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/SignalLoss.fx" );
			m_signalLossEffect->SetParameter( BlueSharedString( "NoiseStrength" ), signalLoss->m_strength );
			m_signalLossEffect->EndUpdate();
			signalLoss->SetDirty( false );
		}
		else if( signalLoss->IsDirty() )
		{
			m_signalLossEffect->StartUpdate();
			m_signalLossEffect->SetParameter( BlueSharedString( "NoiseStrength" ), signalLoss->m_strength );
			m_signalLossEffect->EndUpdate();
			signalLoss->SetDirty( false );
		}
	}
	else
	{
		m_signalLossEffect = nullptr;
	}

	return signalLoss != nullptr && signalLoss->IsActive();
}


void TriStepRenderPostProcess::RenderSignalLoss( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPSignalLossEffect* signalLoss )
{
	GPU_REGION( renderContext, "Signal Loss" );
	
	Tr2Renderer::DrawTexture( renderContext, m_signalLossEffect, *dest, Vector2( 0, 0 ), Vector2( 1, 1 ) );
}


bool TriStepRenderPostProcess::ProcessDynamicExposure( Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure, Tr2PPBloomEffect* bloom )
{
	if( !m_exposure || !m_exposure->IsValid() )
	{
		m_exposure = nullptr;
		m_exposure.CreateInstance();
		m_exposure->Create( 8, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 2 );
		const float clearValue[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderContext.ClearUav( *m_exposure, clearValue );
	}
	if( dynamicExposure && dynamicExposure->IsActive() )
	{
		if( m_dynamicExposureCreateHistogramShader == nullptr || m_dynamicExposureMergeHistogramShader == nullptr || m_dynamicExposureMeasureExposureShader == nullptr )
		{
			m_localHistograms.CreateInstance();
			m_histogram.CreateInstance();

			m_localHistograms->Create( m_localHistogramCount, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_UINT, 2 );
			m_histogram->Create( 65, Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, 2 );

			m_dynamicExposureCreateHistogramShader.CreateInstance();
			m_dynamicExposureCreateHistogramShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/CreateHistograms.fx" );
			m_dynamicExposureCreateHistogramShader->StartUpdate();
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "MinLuminance" ), log( dynamicExposure->m_minLuminance ) );
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "MaxLuminance" ), log( dynamicExposure->m_maxLuminance ) );
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "ScreenTilesX" ), float( m_tilesX ) );
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "LocalHistograms" ), m_localHistograms );
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "BlitOriginal" ), PLACEHOLDER );
			m_dynamicExposureCreateHistogramShader->EndUpdate();

			m_dynamicExposureMergeHistogramShader.CreateInstance();
			m_dynamicExposureMergeHistogramShader->StartUpdate();
			m_dynamicExposureMergeHistogramShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/MergeHistograms.fx" );
			m_dynamicExposureMergeHistogramShader->SetParameter( BlueSharedString( "ScreenTilesX" ), float( m_tilesX ) );
			m_dynamicExposureMergeHistogramShader->SetParameter( BlueSharedString( "ScreenTilesY" ), float( m_tilesY ) );
			m_dynamicExposureMergeHistogramShader->SetParameter( BlueSharedString( "LocalHistograms" ), m_localHistograms );
			m_dynamicExposureMergeHistogramShader->SetParameter( BlueSharedString( "Histogram" ), m_histogram );
			m_dynamicExposureMergeHistogramShader->EndUpdate();

			m_dynamicExposureMeasureExposureShader.CreateInstance();
			m_dynamicExposureMeasureExposureShader->StartUpdate();
			m_dynamicExposureMeasureExposureShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/MeasureExposure.fx" );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "MinLuminance" ), log( dynamicExposure->m_minLuminance ) );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "MaxLuminance" ), log( dynamicExposure->m_maxLuminance ) );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "MinBrightness" ), dynamicExposure->m_minBrightness );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "MaxBrightness" ), dynamicExposure->m_maxBrightness );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "IncreaseSpeed" ), dynamicExposure->m_increaseSpeed );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "DecreaseSpeed" ), dynamicExposure->m_decreaseSpeed );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "MinExposure" ), dynamicExposure->m_minExposure );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "MaxExposure" ), dynamicExposure->m_maxExposure );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "Histogram" ), m_histogram );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "Exposure" ), m_exposure );

			m_dynamicExposureMeasureExposureShader->EndUpdate();

			// we also need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "Exposure" ), m_exposure );
			m_tonemappingEffect->SetParameter( BlueSharedString( "Histogram" ), m_histogram );
			m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureAdjust" ), pow( 2.0f, dynamicExposure->m_adjustment ) );
			m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureMiddleValue" ), dynamicExposure->m_middleValue );
			m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureInfluence" ), dynamicExposure->m_influence );
			m_tonemappingEffect->SetParameter( BlueSharedString( "MinExposure" ), dynamicExposure->m_minExposure );
			m_tonemappingEffect->SetParameter( BlueSharedString( "MaxExposure" ), dynamicExposure->m_maxExposure );

			m_tonemappingEffect->SetOption( BlueSharedString( "DYNAMIC_EXPOSURE_TOGGLE" ), BlueSharedString( "DYNAMIC_EXPOSURE_ENABLED" ) );

			m_tonemappingEffect->EndUpdate();

			// mark the bloom as dirty so it can decide what to do with the exposure
			if( bloom != nullptr )
			{
				bloom->SetDirty( true );
			}

			dynamicExposure->SetDirty( false );
		}
		else if( dynamicExposure->IsDirty() )
		{
			m_dynamicExposureCreateHistogramShader->StartUpdate();
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "MinLuminance" ), log( dynamicExposure->m_minLuminance ) );
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "MaxLuminance" ), log( dynamicExposure->m_maxLuminance ) );
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "MinBrightness" ), dynamicExposure->m_minBrightness );
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "MaxBrightness" ), dynamicExposure->m_maxBrightness );
			m_dynamicExposureCreateHistogramShader->EndUpdate();

			m_dynamicExposureMeasureExposureShader->StartUpdate();
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "MinLuminance" ), log( dynamicExposure->m_minLuminance ) );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "MaxLuminance" ), log( dynamicExposure->m_maxLuminance ) );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "MinBrightness" ), dynamicExposure->m_minBrightness );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "MaxBrightness" ), dynamicExposure->m_maxBrightness );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "IncreaseSpeed" ), dynamicExposure->m_increaseSpeed );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "DecreaseSpeed" ), dynamicExposure->m_decreaseSpeed );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "MinExposure" ), dynamicExposure->m_minExposure );
			m_dynamicExposureMeasureExposureShader->SetParameter( BlueSharedString( "MaxExposure" ), dynamicExposure->m_maxExposure );
			m_dynamicExposureMeasureExposureShader->EndUpdate();

			// we also need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureAdjust" ), pow( 2.0f, dynamicExposure->m_adjustment ) );
			m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureMiddleValue" ), dynamicExposure->m_middleValue );
			m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureInfluence" ), dynamicExposure->m_influence );
			m_tonemappingEffect->SetParameter( BlueSharedString( "MinExposure" ), dynamicExposure->m_minExposure );
			m_tonemappingEffect->SetParameter( BlueSharedString( "MaxExposure" ), dynamicExposure->m_maxExposure );
			m_tonemappingEffect->SetOption( BlueSharedString( "DYNAMIC_EXPOSURE_TOGGLE" ), BlueSharedString( "DYNAMIC_EXPOSURE_ENABLED" ) );
			m_tonemappingEffect->EndUpdate();

			// and the dynamic exposure to texture if it is applicable
			if( m_dynamicExposureToTextureShader )
			{
				m_dynamicExposureToTextureShader->StartUpdate();
				m_dynamicExposureToTextureShader->SetParameter( BlueSharedString( "ExposureMiddleValue" ), dynamicExposure->m_middleValue );
				m_dynamicExposureToTextureShader->EndUpdate();
			}

			dynamicExposure->SetDirty( false );
		}
	}
	else
	{
		if( m_dynamicExposureCreateHistogramShader != nullptr || m_dynamicExposureMeasureExposureShader != nullptr || m_dynamicExposureMergeHistogramShader != nullptr ||
			m_localHistograms != nullptr || m_histogram != nullptr )
		{
			m_dynamicExposureCreateHistogramShader = nullptr;
			m_dynamicExposureMeasureExposureShader = nullptr;
			m_dynamicExposureMergeHistogramShader = nullptr;
			m_localHistograms = nullptr;
			m_histogram = nullptr;

			// mark the bloom as dirty so it can decide what to do with the exposure
			if( bloom != nullptr )
			{
				bloom->SetDirty( true );
			}

			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetOption( BlueSharedString( "DYNAMIC_EXPOSURE_TOGGLE" ), BlueSharedString( "DYNAMIC_EXPOSURE_DISABLED" ) );
			m_tonemappingEffect->EndUpdate();
		}
	}

	return dynamicExposure != nullptr && dynamicExposure->IsActive();
}


void TriStepRenderPostProcess::RenderDynamicExposure( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure )
{
	GPU_REGION( renderContext, "Exposure" );

	m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "BlitOriginal" ), dest );

	uint32_t m_uintValue[4] = { 0, 0, 0, 0 };
	// Clear local histograms
	auto lhbuffer = m_localHistograms->GetGpuBuffer( 0 );
	renderContext.ClearUav( *lhbuffer, m_uintValue );

	// Clear histograms
	auto hbuffer = m_histogram->GetGpuBuffer( 0 );
	renderContext.ClearUav( *hbuffer, m_uintValue );

	// Create histograms
	Tr2Renderer::RunComputeShader( m_dynamicExposureCreateHistogramShader, m_tilesX, m_tilesY, 1, renderContext );

	// Merge histogram
	Tr2Renderer::RunComputeShader( m_dynamicExposureMergeHistogramShader, m_mergeHistogramXDim, 1, 1, renderContext );

	// Measure histogram
	Tr2Renderer::RunComputeShader( m_dynamicExposureMeasureExposureShader, 1, 1, 1, renderContext );
}

Tr2PostProcessRenderInfo::Texture TriStepRenderPostProcess::RenderUpscaling( Tr2RenderTarget* source, Tr2RenderContext& renderContext, Tr2UpscalingContextAL* upscalingContext, Tr2PPDynamicExposureEffect* dynamicExposure )
{
	GPU_REGION( renderContext, "Upscaling" );
	if( renderContext.GetPrimaryRenderContext().GetUpscalingInfo( m_upscalingContextID ).temporal )
	{
		m_scene->SetVelocityMap( m_velocityBuffer );
	}
	
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
	uint32_t w, h;
	upscalingContext->GetDisplayDimensions( w, h );

	Tr2UpscalingAL::DispatchParameters dispatchParameters = {};
	auto dispatchRequirements = upscalingContext->GetDispatchRequirements();
	auto dest = m_renderInfo->GetTempTexture( w, h, Tr2RenderContextEnum::EX_BIND_UNORDERED_ACCESS );
	dispatchParameters.output = dest.GetRenderTarget()->GetTexture();

	bool wantsExposure = dispatchRequirements & Tr2UpscalingAL::DispatchRequirements::OPTIONAL_EXPOSURE;
	bool canHaveExposure = dynamicExposure && dynamicExposure->IsActive();
	float middleValue = dynamicExposure ? dynamicExposure->m_middleValue : 0.0f;
	SetupExposureConversion( wantsExposure && canHaveExposure, middleValue );

	if( wantsExposure && m_exposureTexture )
	{
		GPU_REGION( renderContext, "ExposureToTexture" );
		m_dynamicExposureToTextureShader->SetParameter( BlueSharedString( "ExposureBuffer" ), m_exposure );
		m_dynamicExposureToTextureShader->SetParameter( BlueSharedString( "ExposureTexture" ), m_exposureTexture );
		Tr2Renderer::RunComputeShader( m_dynamicExposureToTextureShader, 1, 1, 1, renderContext );
		dispatchParameters.exposure = m_exposureTexture ? m_exposureTexture->GetTexture() : nullptr;
	}

	if( dispatchRequirements & Tr2UpscalingAL::DispatchRequirements::REACTIVE )
	{
		GPU_REGION( renderContext, "ReactiveMask" );
		if( !m_reactiveMask )
		{
			m_reactiveMask.CreateInstance();
			m_reactiveMask->SetName( "ReactiveMask" );
			m_reactiveMask->Create( source->GetWidth(), source->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM, source->GetMsaaType(), 0 );
		}

		m_reactiveMaskEffect->SetParameter( BlueSharedString( "OpaqueOnly" ), m_opaqueColorBuffer );
		m_reactiveMaskEffect->SetParameter( BlueSharedString( "OpaqueAndTransparency" ), source );
		DrawInto( *m_reactiveMask, Tr2LoadAction::DONT_CARE, m_reactiveMaskEffect, renderContext );
		dispatchParameters.reactive = m_reactiveMask ? m_reactiveMask->GetTexture() : nullptr;
	}

	auto view = Tr2Renderer::GetViewTransform();
	auto projection = Tr2Renderer::GetProjectionTransform();
	auto inverseProjection = Inverse( projection );
	auto reprojection = m_scene->GetReprojectionMatrix();
	auto invReprojection = Inverse( reprojection );

	dispatchParameters.aspectRatio = Tr2Renderer::GetAspectRatio();
	dispatchParameters.backClip = Tr2Renderer::GetBackClip();
	dispatchParameters.frontClip = Tr2Renderer::GetFrontClip();
	dispatchParameters.fieldOfView = Tr2Renderer::GetFieldOfView();
	dispatchParameters.frameTimeDelta = TimeAsFloat( BeOS->GetCurrentFrameTime() - m_lastFrameTime ) * 1000.0f;
	dispatchParameters.preExposure = 0.4f;

	memcpy( dispatchParameters.cameraPos, &Tr2Renderer::GetViewPosition(), 3 * sizeof( float ) );
	memcpy( dispatchParameters.cameraForward, &view.GetZ(), 3 * sizeof( float ) );
	memcpy( dispatchParameters.cameraUp, &view.GetY(), 3 * sizeof( float ) );
	memcpy( dispatchParameters.cameraRight, &view.GetX(), 3 * sizeof( float ) );
	memcpy( dispatchParameters.projection, &projection, 16 * sizeof( float ) );
	memcpy( dispatchParameters.invProjection, &inverseProjection, 16 * sizeof( float ) );
	memcpy( dispatchParameters.clipToPrevClip, &reprojection, 16 * sizeof( float ) );
	memcpy( dispatchParameters.prevClipToClip, &invReprojection, 16 * sizeof( float ) );

	dispatchParameters.depth = m_scene->GetDepth() ? m_scene->GetDepth()->GetTexture() : nullptr;
	dispatchParameters.input = source ? source->GetTexture() : nullptr;
	dispatchParameters.opaqueOnly = m_opaqueColorBuffer ? m_opaqueColorBuffer->GetTexture() : nullptr;
	dispatchParameters.velocity = m_velocityBuffer ? m_velocityBuffer->GetTexture() : nullptr;

	m_lastFrameTime = BeOS->GetCurrentFrameTime();
	{
		GPU_REGION( renderContext, "UpscalingTechnique" );

		upscalingContext->Dispatch( dispatchParameters );
	}
	return dest;
}

bool TriStepRenderPostProcess::ProcessFilmGrain( Tr2PPFilmGrainEffect* filmGrain )
{
	if( filmGrain && filmGrain->IsActive() )
	{
		if( filmGrain->IsDirty() || !m_grainShader )
		{
			if( !m_grainShader )
			{
				m_grainShader.CreateInstance();
				m_grainShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/FilmGrain.fx" );
			}

			m_grainShader->StartUpdate();


			//shared parameters
			m_grainShader->SetParameter( BlueSharedString( "GrainColorAmount" ), filmGrain->m_colored ? filmGrain->m_colorAmount : 0.0f );
			m_grainShader->SetParameter( BlueSharedString( "InputTexture" ), PLACEHOLDER );

			float threshold = 1.0f - filmGrain->m_grainDensity;
			float edge = 1.0f / ( filmGrain->m_grainContrast * filmGrain->m_grainSize );
			m_grainShader->SetParameter( BlueSharedString( "GrainSize" ), filmGrain->m_grainSize );
			m_grainShader->SetParameter( BlueSharedString( "GrainIntensity" ), filmGrain->m_intensity );
			m_grainShader->SetParameter( BlueSharedString( "GrainThreshold" ), threshold );
			m_grainShader->SetParameter( BlueSharedString( "GrainEdge" ), edge );
			m_grainShader->SetParameter( BlueSharedString( "BrightnessModifier" ), filmGrain->m_brightnessModifier );
			m_grainShader->AddResourceTexture2D( BlueSharedString( "NoiseTexture" ), "res:/texture/global/film_grain_noise.png" );

			m_grainShader->EndUpdate();

			filmGrain->SetDirty( false );
		}

		return true;
	}

	m_grainShader = nullptr;
	return false;
}

void TriStepRenderPostProcess::RenderFilmGrain( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPFilmGrainEffect* filmGrain )
{
	GPU_REGION( renderContext, "Film grain" );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
	m_grainShader->SetParameter( BlueSharedString( "InputTexture" ), dest );
	Tr2Renderer::DrawScreenQuad( renderContext, m_grainShader );
}

void TriStepRenderPostProcess::ProcessDesaturate( Tr2PPDesaturateEffect* desaturate )
{
	if( desaturate && desaturate->IsActive() )
	{
		if( desaturate->IsDirty() )
		{
			// we only need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "SaturationFactor" ), desaturate->m_intensity );
			m_tonemappingEffect->SetOption( BlueSharedString( "DESATURATE_TOGGLE" ), BlueSharedString( "DESATURATE_ENABLED" ) );
			m_tonemappingEffect->EndUpdate();

			desaturate->SetDirty( false );
			m_desaturateEnabled = true;
		}
	}
	else if( m_desaturateEnabled )
	{
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetOption( BlueSharedString( "DESATURATE_TOGGLE" ), BlueSharedString( "DESATURATE_DISABLED" ) );
		m_tonemappingEffect->EndUpdate();
		m_desaturateEnabled = false;
	}
}

void TriStepRenderPostProcess::ProcessFade( Tr2PPFadeEffect* fade )
{
	if( fade && fade->IsActive() )
	{
		if( fade->IsDirty() )
		{
			// we only need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "FadeColor" ), Vector4( fade->m_color ) );
			// A shader option can be placed here, although the complexity of checking when to toggle it may outweigh the benefits of the optimization
			m_tonemappingEffect->SetParameter( BlueSharedString( "FadeAmount" ), fade->m_intensity );
			m_tonemappingEffect->EndUpdate();

			fade->SetDirty( false );
			m_fadeEnabled = true;
		}
	}
	else if( m_fadeEnabled )
	{
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetParameter( BlueSharedString( "FadeAmount" ), 0.0f );
		m_tonemappingEffect->EndUpdate();
		m_fadeEnabled = false;
	}
}

void TriStepRenderPostProcess::ProcessLut( Tr2PPLutEffect* lut )
{
	if( lut && lut->IsActive() )
	{
		if( lut->IsDirty() )
		{
			// we only need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "LUTInfluence" ), lut->m_influence );
			auto resource = m_tonemappingEffect->GetResourceByName( "TexLUT" );
			if( !resource )
			{
				m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "TexLUT" ), lut->m_path.c_str() );
			}
			else
			{
				const auto param = dynamic_cast<TriTextureParameter*>( resource );
				const auto currPath = param->GetResourcePath();
				const std::string possibleNewPathStr = lut->m_path.c_str();

				const std::wstring possibleNewPathWstr( possibleNewPathStr.begin(), possibleNewPathStr.end() );

				if( currPath != possibleNewPathWstr )
				{
					param->SetResourcePath( lut->m_path.c_str() );
				}
			}
			m_tonemappingEffect->SetOption( BlueSharedString( "LUT_TOGGLE" ), BlueSharedString( "LUT_ENABLED" ) );
			m_tonemappingEffect->EndUpdate();

			lut->SetDirty( false );
			m_lutEnabled = true;
		}
	}
	else if( m_lutEnabled )
	{
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetOption( BlueSharedString( "LUT_TOGGLE" ), BlueSharedString( "LUT_DISABLED" ) );
		m_tonemappingEffect->EndUpdate();
		m_lutEnabled = false;
	}
}

void TriStepRenderPostProcess::ProcessVignette( Tr2PPVignetteEffect* vignette )
{
	if( vignette && vignette->IsActive() )
	{
		if( vignette->IsDirty() )
		{
			// we only need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();

			auto shapeResource = m_tonemappingEffect->GetResourceByName( "VignetteShape" );
			if( !shapeResource )
			{
				m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "VignetteShape" ), vignette->m_shapePath.c_str() );
			}
			else
			{
				dynamic_cast<TriTextureParameter*>( shapeResource )->SetResourcePath( vignette->m_shapePath.c_str() );
			}

			auto detailResource = m_tonemappingEffect->GetResourceByName( "VignetteDetail" );
			if( !detailResource )
			{
				m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "VignetteDetail" ), vignette->m_detailPath.c_str() );
			}
			else
			{
				dynamic_cast<TriTextureParameter*>( detailResource )->SetResourcePath( vignette->m_detailPath.c_str() );
			}

			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteDetailSize" ), Vector4( vignette->m_detail1Size[0], vignette->m_detail1Size[1], vignette->m_detail2Size[0], vignette->m_detail2Size[1] ) );
			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteDetailScroll" ), Vector4( vignette->m_detail1Scroll[0], vignette->m_detail1Scroll[1], vignette->m_detail2Scroll[0], vignette->m_detail2Scroll[1] ) );
			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteColor" ), Vector4( vignette->m_color ) );
			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteIntensity" ), Vector2( vignette->m_intensity, vignette->m_opacity ) );
			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteSineFrequency" ), vignette->m_sineFrequency );
			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteSineRange" ), Vector2( vignette->m_sineMinimum, vignette->m_sineMaximum ) );

			m_tonemappingEffect->SetOption( BlueSharedString( "VIGNETTE_TOGGLE" ), BlueSharedString( "VIGNETTE_ENABLED" ) );
			m_tonemappingEffect->EndUpdate();

			vignette->SetDirty( false );
			m_vignetteEnabled = true;
		}
	}
	else if( m_vignetteEnabled )
	{
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetOption( BlueSharedString( "VIGNETTE_TOGGLE" ), BlueSharedString( "VIGNETTE_DISABLED" ) );
		m_tonemappingEffect->EndUpdate();
		m_vignetteEnabled = false;
	}
}

bool TriStepRenderPostProcess::ProcessFog( Tr2PPFogEffect* fog )
{
	if( fog && fog->IsActive() )
	{
		if( m_fogColorEffect == nullptr || m_fogCompositeEffect == nullptr )
		{
			m_fogColorEffect.CreateInstance();
			m_fogColorEffect->StartUpdate();
			m_fogColorEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/EnvironmentFogColor.fx" );
			m_fogColorEffect->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_fogColorEffect->SetParameter( BlueSharedString( "Params" ), Vector4( fog->m_nebulaInfluence, fog->m_nebulaBlur, fog->m_originalBrightenOnly, fog->m_colorInfluence ) );
			m_fogColorEffect->SetParameter( BlueSharedString( "Color" ), Vector4( fog->m_color ) );
			m_fogColorEffect->EndUpdate();

			m_fogCompositeEffect.CreateInstance();
			m_fogCompositeEffect->StartUpdate();
			m_fogCompositeEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/EnvironmentFogComposit.fx" );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BlitOriginal" ), PLACEHOLDER ); // this used _fogsource in eve.yaml, but I'm trying _source here
			m_fogCompositeEffect->SetParameter( BlueSharedString( "FogParameters" ), Vector4( fog->m_totalAmount, fog->m_totalPower, fog->m_backgroundOcclusion, fog->m_intensity ) );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BrightnessAdjustment" ), Vector4( fog->m_brightnessThreshold0, fog->m_brightnessThreshold1, fog->m_brightnessAdjustmentAmount, 0.0f ) );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BlendFunction0" ), Vector4( fog->m_blendDistance0, fog->m_blendBias0, fog->m_blendAmount0, fog->m_blendPower0 ) );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BlendFunction1" ), Vector4( fog->m_blendDistance1, fog->m_blendBias1, fog->m_blendAmount1, fog->m_blendPower1 ) );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BlendFunction2" ), Vector4( fog->m_blendDistance2, fog->m_blendBias2, fog->m_blendAmount2, fog->m_blendPower2 ) );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "AreaSize" ), Vector4( fog->m_areaSize, fog->m_areaScale.x ) );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "AreaCenter" ), Vector4( fog->m_areaCenter, fog->m_areaScale.y ) );
			m_fogCompositeEffect->EndUpdate();

			fog->SetDirty( false );
		}
		if( fog->IsDirty() )
		{
			m_fogColorEffect->StartUpdate();
			m_fogColorEffect->SetParameter( BlueSharedString( "Params" ), Vector4( fog->m_nebulaInfluence, fog->m_nebulaBlur, fog->m_originalBrightenOnly, fog->m_colorInfluence ) );
			m_fogColorEffect->SetParameter( BlueSharedString( "Color" ), Vector4( fog->m_color ) );
			m_fogColorEffect->EndUpdate();

			m_fogCompositeEffect->StartUpdate();
			m_fogCompositeEffect->SetParameter( BlueSharedString( "FogParameters" ), Vector4( fog->m_totalAmount, fog->m_totalPower, fog->m_backgroundOcclusion, fog->m_intensity ) );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BrightnessAdjustment" ), Vector4( fog->m_brightnessThreshold0, fog->m_brightnessThreshold1, fog->m_brightnessAdjustmentAmount, 0.0f ) );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BlendFunction0" ), Vector4( fog->m_blendDistance0, fog->m_blendBias0, fog->m_blendAmount0, fog->m_blendPower0 ) );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BlendFunction1" ), Vector4( fog->m_blendDistance1, fog->m_blendBias1, fog->m_blendAmount1, fog->m_blendPower1 ) );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BlendFunction2" ), Vector4( fog->m_blendDistance2, fog->m_blendBias2, fog->m_blendAmount2, fog->m_blendPower2 ) );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "AreaSize" ), Vector4( fog->m_areaSize, fog->m_areaScale.x ) );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "AreaCenter" ), Vector4( fog->m_areaCenter, fog->m_areaScale.y ) );
			m_fogCompositeEffect->EndUpdate();

			fog->SetDirty( false );
		}
	}
	else
	{
		m_fogColorEffect = nullptr;
		m_fogCompositeEffect = nullptr;
	}
	return fog && fog->IsActive();
}

void TriStepRenderPostProcess::RenderFog( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPFogEffect* fog )
{
	GPU_REGION( renderContext, "Fog" );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	auto tempCopy = m_renderInfo->GetTempTexture();
	DrawInto( *tempCopy, Tr2LoadAction::DONT_CARE, *dest, renderContext );

	// render fog color
	auto rt1 = m_renderInfo->GetTempTexture( 0.5f );
	m_fogColorEffect->SetParameter( BlueSharedString( "BlitCurrent" ), dest );
	DrawInto( *rt1, Tr2LoadAction::DONT_CARE, m_fogColorEffect, renderContext );

	// blur
	auto blurContext = PostProcessBlur::CreateBlurContext( 0.5f );
	Blur( *rt1, *rt1, renderContext, blurContext );

	// final composite
	m_fogCompositeEffect->SetParameter( BlueSharedString( "BlitCurrent" ), rt1 );
	m_fogCompositeEffect->SetParameter( BlueSharedString( "BlitOriginal" ), tempCopy );
	DrawInto( *dest, Tr2LoadAction::DONT_CARE, m_fogCompositeEffect, renderContext );
}

bool TriStepRenderPostProcess::ProcessTaa( Tr2PPTaaEffect* taa )
{
	if( taa && taa->IsActive() )
	{
		if( 
				m_taaEffect == nullptr || m_taaCopyEffect == nullptr || m_accumulationBuffer0 == nullptr || m_accumulationBuffer1 == nullptr || m_cooldownBuffer == nullptr
		)
		{

			auto source = m_renderInfo->GetSourceBuffer();

			m_accumulationBuffer0.CreateInstance();
			m_accumulationBuffer0->Create( source->GetWidth(), source->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_UNORM );
			m_accumulationBuffer0->SetName( "m_accumulationBuffer0" );

			m_accumulationBuffer1.CreateInstance();
			m_accumulationBuffer1->Create( source->GetWidth(), source->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_UNORM );
			m_accumulationBuffer1->SetName( "m_accumulationBuffer1" );

			m_cooldownBuffer.CreateInstance();
			m_cooldownBuffer->Create( source->GetWidth(), source->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, 1, 0, Tr2RenderContextEnum::EX_BIND_UNORDERED_ACCESS );
			m_cooldownBuffer->SetName( "m_cooldownBuffer" );

			m_taaEffect.CreateInstance();
			m_taaEffect->StartUpdate();
			m_taaEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/TAA.fx" );
			m_taaEffect->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_taaEffect->SetParameter( BlueSharedString( "LastFrame" ), PLACEHOLDER );
			m_taaEffect->SetParameter( BlueSharedString( "VelocityMap" ), m_velocityBuffer );
			m_taaEffect->EndUpdate();

			m_taaCopyEffect.CreateInstance();
			m_taaCopyEffect->StartUpdate(); 
			m_taaCopyEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/TAACopy.fx" );
			m_taaCopyEffect->SetParameter( BlueSharedString( "AccumulationBuffer" ), PLACEHOLDER );
			m_taaCopyEffect->EndUpdate();


			m_taaFrameCounter = 0;

			taa->SetDirty( true );
		}
		
		if( taa->IsDirty() )
		{

			m_taaEffect->StartUpdate();

			m_taaEffect->SetParameter( BlueSharedString( "EarlyOutThreshold" ), taa->m_earlyOutThreshold );


			int quality = taa->m_quality;
			BlueSharedString quality_option;
			if( quality <= 1 )
			{
				quality_option = BlueSharedString( "QUALITY_LOW" );
			}
			else if( quality == 2 )
			{
				quality_option = BlueSharedString( "QUALITY_MEDIUM" );
			}
			else //if( quality >= 3 )
			{
				quality_option = BlueSharedString( "QUALITY_HIGH" );
			}
			m_taaEffect->SetOption( BlueSharedString( "QUALITY" ), quality_option );


			if(taa->m_showMotionVectors)
			{
				m_taaEffect->SetOption( BlueSharedString( "DEBUG" ), BlueSharedString( "DEBUG_SHOW_MOTION_VECTORS" ) );
			}
			else if( taa->m_showEarlyOutMask )
			{
				m_taaEffect->SetOption( BlueSharedString( "DEBUG" ), BlueSharedString( "DEBUG_SHOW_EARLY_OUT_MASK" ) );
			}
			else 
			{
				m_taaEffect->SetOption( BlueSharedString( "DEBUG" ), BlueSharedString( "DEBUG_NONE" ) );
			}

			m_taaEffect->EndUpdate();

			m_taaFrameCounter = 0;

			taa->SetDirty( false );
		}

		m_scene->SetVelocityMap( m_velocityBuffer );
	}
	else
	{
		if( m_taaEffect != nullptr || m_taaCopyEffect != nullptr || m_accumulationBuffer0 != nullptr || m_accumulationBuffer1 != nullptr || m_cooldownBuffer != nullptr )
		{
			m_taaEffect = nullptr;
			m_taaCopyEffect = nullptr;
			m_accumulationBuffer0 = nullptr;
			m_accumulationBuffer1 = nullptr;
			m_cooldownBuffer = nullptr;
		}
	}

	return taa && taa->IsActive();
}

void TriStepRenderPostProcess::RenderTaa( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPTaaEffect* taa, Tr2PPDynamicExposureEffect* dynamicExposure )
{
	GPU_REGION( renderContext, "TAA" );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	auto source = m_renderInfo->GetSourceBuffer();

	m_scene->GetPostProcessPSBuffer()->ApplyBuffer( renderContext );

	Tr2RenderTarget* input;
	Tr2RenderTarget* output;
	bool inputValid = true;

	uint32_t frame_count = m_taaFrameCounter++;
	if( ( frame_count & 1 ) == 0 )
	{
		input = m_accumulationBuffer0;
		output = m_accumulationBuffer1;
	}
	else
	{
		input = m_accumulationBuffer1;
		output = m_accumulationBuffer0;
	}


	Tr2EffectPtr effects[] = {m_taaEffect, m_taaCopyEffect};
	for( Tr2EffectPtr effect : effects )
	{
		effect->StartUpdate();

		if( dynamicExposure && dynamicExposure->IsActive() )
		{
			effect->SetParameter( BlueSharedString( "Exposure" ), m_exposure );
			effect->SetParameter( BlueSharedString( "ExposureAdjust" ), pow( 2.0f, dynamicExposure->m_adjustment ) );
			effect->SetParameter( BlueSharedString( "ExposureMiddleValue" ), dynamicExposure->m_middleValue );
			effect->SetParameter( BlueSharedString( "ExposureInfluence" ), dynamicExposure->m_influence );

			effect->SetOption( BlueSharedString( "DYNAMIC_EXPOSURE_TOGGLE" ), BlueSharedString( "DYNAMIC_EXPOSURE_ENABLED" ) );
		}
		else
		{
			effect->SetOption( BlueSharedString( "DYNAMIC_EXPOSURE_TOGGLE" ), BlueSharedString( "DYNAMIC_EXPOSURE_DISABLED" ) );
		}

		effect->EndUpdate();
	}
		


	m_taaEffect->SetParameter( BlueSharedString( "FrameIndex" ), frame_count );

	m_taaEffect->SetParameter( BlueSharedString( "CurrentFrame" ), dest );
	m_taaEffect->SetParameter( BlueSharedString( "CurrentFrameOpaque" ), m_opaqueColorBuffer );
	m_taaEffect->SetParameter( BlueSharedString( "AccumulationBuffer" ), input );
	m_taaEffect->SetParameter( BlueSharedString( "CooldownMap" ), m_cooldownBuffer );

	float max_weight = 0.96f;
	float weight = min( (float)frame_count / ( (float)frame_count + 1.0f ), max_weight );
	m_taaEffect->SetParameter( BlueSharedString( "BlendWeight" ), weight );

	DrawInto( *output, Tr2LoadAction::DONT_CARE, m_taaEffect, renderContext );


	m_taaCopyEffect->SetParameter( BlueSharedString( "AccumulationBuffer" ), output );

	DrawInto( *dest, Tr2LoadAction::DONT_CARE, m_taaCopyEffect, renderContext );
}

bool TriStepRenderPostProcess::ProcessDepthOfField( Tr2RenderContext& renderContext, Tr2PPDepthOfFieldEffect* fx )
{
	if( fx && fx->IsActive() )
	{
		if( !m_depthOfFieldBokehBlurShader || !m_depthOfFieldCoCShader || !m_depthOfFieldBokehFillShader )
		{


			// we just created the effect
			m_depthOfFieldBokehBlurShader.CreateInstance();
			m_depthOfFieldBokehBlurShader->StartUpdate();
			m_depthOfFieldBokehBlurShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Bokeh.fx" );
			m_depthOfFieldBokehBlurShader->SetParameter( BlueSharedString( "CoCMap" ), PLACEHOLDER );
			m_depthOfFieldBokehBlurShader->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_depthOfFieldBokehBlurShader->SetOption( BlueSharedString( "BOKEH_PIXEL_METHOD" ), BlueSharedString( "BOKEH_PIXEL_AVERAGE" ) );
			m_depthOfFieldBokehBlurShader->SetOption( BlueSharedString( "BOKEH_SHAPE" ), fx->GetBokehShapeString() );
			m_depthOfFieldBokehBlurShader->EndUpdate();

			m_depthOfFieldBokehFillShader.CreateInstance();
			m_depthOfFieldBokehFillShader->StartUpdate();
			m_depthOfFieldBokehFillShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Bokeh.fx" );
			m_depthOfFieldBokehFillShader->SetParameter( BlueSharedString( "CoCMap" ), PLACEHOLDER );
			m_depthOfFieldBokehFillShader->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_depthOfFieldBokehFillShader->SetOption( BlueSharedString( "BOKEH_PIXEL_METHOD" ), BlueSharedString( "BOKEH_PIXEL_MAX" ) );
			m_depthOfFieldBokehFillShader->SetOption( BlueSharedString( "BOKEH_SHAPE" ), fx->GetBokehShapeString() );
			m_depthOfFieldBokehFillShader->EndUpdate();

			m_depthOfFieldBokehTAAShader.CreateInstance();
			m_depthOfFieldBokehTAAShader->StartUpdate();
			m_depthOfFieldBokehTAAShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/BokehTAA.fx" );
			m_depthOfFieldBokehTAAShader->SetParameter( BlueSharedString( "CoCMap" ), PLACEHOLDER );
			m_depthOfFieldBokehTAAShader->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_depthOfFieldBokehTAAShader->SetOption( BlueSharedString( "BOKEH_SHAPE" ), fx->GetBokehShapeString() );
			m_depthOfFieldBokehTAAShader->EndUpdate();

			m_depthOfFieldCoCShader.CreateInstance();
			m_depthOfFieldCoCShader->StartUpdate();
			m_depthOfFieldCoCShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/CircleOfConfusion.fx" );
			m_depthOfFieldCoCShader->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_depthOfFieldCoCShader->SetParameter( BlueSharedString( "FocalInfo" ), Vector4( fx->m_focalDistance, fx->m_focalLength, fx->m_scale, 0.0 ) );
			m_depthOfFieldCoCShader->SetOption( BlueSharedString( "CoCMode" ), BlueSharedString( "CoCBack" ) );
			m_depthOfFieldCoCShader->SetOption( BlueSharedString( "COC_OUTPUT_CHANNEL_COUNT" ), fx->m_foregroundBlurNeeded ? BlueSharedString( "COC_OUTPUT_CHANNEL_COUNT_2" ) : BlueSharedString( "COC_OUTPUT_CHANNEL_COUNT_1" ) );
			m_depthOfFieldCoCShader->EndUpdate();
		}
		else if( fx->IsDirty() )
		{
			// we just changed the effect
			m_depthOfFieldCoCShader->StartUpdate();
			m_depthOfFieldCoCShader->SetParameter( BlueSharedString( "FocalInfo" ), Vector4( fx->m_focalDistance, fx->m_focalLength, fx->m_scale, 0.0 ) );
			m_depthOfFieldCoCShader->SetOption( BlueSharedString( "COC_OUTPUT_CHANNEL_COUNT" ), fx->m_foregroundBlurNeeded ? BlueSharedString( "COC_OUTPUT_CHANNEL_COUNT_2" ) : BlueSharedString( "COC_OUTPUT_CHANNEL_COUNT_1" ) );
			m_depthOfFieldCoCShader->EndUpdate();

			m_depthOfFieldBokehBlurShader->StartUpdate();
			m_depthOfFieldBokehBlurShader->SetOption( BlueSharedString( "BOKEH_SHAPE" ), fx->GetBokehShapeString() );
			m_depthOfFieldBokehBlurShader->EndUpdate();

			m_depthOfFieldBokehFillShader->StartUpdate();
			m_depthOfFieldBokehFillShader->SetOption( BlueSharedString( "BOKEH_SHAPE" ), fx->GetBokehShapeString() );
			m_depthOfFieldBokehFillShader->EndUpdate();

			m_depthOfFieldBokehTAAShader->StartUpdate();
			m_depthOfFieldBokehTAAShader->SetOption( BlueSharedString( "BOKEH_SHAPE" ), fx->GetBokehShapeString() );
			m_depthOfFieldBokehTAAShader->EndUpdate();
		}
		fx->SetDirty( false );
	}
	else
	{
		if( m_depthOfFieldCoCShader || m_depthOfFieldBokehBlurShader || m_depthOfFieldBokehFillShader || m_depthOfFieldBokehTAAShader )
		{
			// we have just deleted the effect
			m_depthOfFieldCoCShader = nullptr;
			m_depthOfFieldBokehBlurShader = nullptr;
			m_depthOfFieldBokehFillShader = nullptr;
			m_depthOfFieldBokehTAAShader = nullptr;
		}
	}
	return fx && fx->IsActive();
}

void TriStepRenderPostProcess::RenderDepthOfField( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPDepthOfFieldEffect* depthOfField, bool temporal, float upscalingAmount )
{
	GPU_REGION( renderContext, "DepthOfField" );
	{
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

		{
			auto coc = m_renderInfo->GetTempTexture( depthOfField->m_cocScale, Tr2RenderContextEnum::EX_NONE, Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM );

			auto blur = m_renderInfo->GetTempTexture();

			if( !depthOfField->m_foregroundBlurNeeded )
			{
				GPU_REGION( renderContext, "CoC" );
				DrawInto( *coc, Tr2LoadAction::DONT_CARE, m_depthOfFieldCoCShader, renderContext );
				if( depthOfField->m_debug == Tr2PPDepthOfFieldEffect::DofDebug_CoC )
				{
					DrawInto( *dest, Tr2LoadAction::DONT_CARE, *coc, renderContext );
					return;
				}
			}
			else
			{
				GPU_REGION( renderContext, "CoC" );
				auto coc2 = m_renderInfo->GetTempTexture( depthOfField->m_cocScale, Tr2RenderContextEnum::EX_NONE, Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_UNORM );

				DrawInto( *coc2, Tr2LoadAction::DONT_CARE, m_depthOfFieldCoCShader, renderContext );
				if( depthOfField->m_debug == Tr2PPDepthOfFieldEffect::DofDebug_CoC )
				{
					DrawInto( *dest, Tr2LoadAction::DONT_CARE, *coc2, renderContext );
					return;
				}
				{
					GPU_REGION( renderContext, "CoC Blur" );

					auto blurContext = PostProcessBlur::CreateBlurContext( PostProcessBlur::BT_Big,
																		   PostProcessBlur::BC_r,
																		   PostProcessBlur::BP_Maximum,
																		   PostProcessBlur::BF_MaxOfAllChannels,
																		   depthOfField->m_cocScale );
					Blur( *coc, *coc2, renderContext, blurContext );
					if( depthOfField->m_debug == Tr2PPDepthOfFieldEffect::DofDebug_CoCBlurred )
					{
						DrawInto( *dest, Tr2LoadAction::DONT_CARE, *coc, renderContext );
						return;
					}
				}
			}

			float adjustedScale = depthOfField->m_scale / upscalingAmount;

			if( depthOfField->m_useTAAFriendlyBokeh )
			{

				float GOLDEN_ANGLE = TRI_PI * ( 3.0f - sqrt( 5.0f ) );
				float angle = 0;
				float samplesPerPixel = 2.0 / 5.0;

				if (temporal)
				{
					//Vary between 4 different rotations, so that it has the same period as the TAA jitter
					//This allows it to detect some kinds of flickering and remove it.
					if( ( m_bokehFrameCounter & 1 ) != 0 )
						angle += TRI_PI;
					if( ( m_bokehFrameCounter & 2 ) != 0 )
						angle += 0.5f * GOLDEN_ANGLE;
					m_bokehFrameCounter++;

					//Also, reduce the target quality, so that we get more noise and the 3x3 neighborhood clamping can reduce shimmer/flicker!
					//In practice, we'll be accumulating 4x as many samples thanks to the rotation above, so this is fine.
					samplesPerPixel = 1.0 / 5.0;
				}

				{
					GPU_REGION( renderContext, "Bokeh" );
					m_depthOfFieldBokehTAAShader->SetParameter( BlueSharedString( "BlitCurrent" ), dest );
					m_depthOfFieldBokehTAAShader->SetParameter( BlueSharedString( "CoCMap" ), coc );
					m_depthOfFieldBokehTAAShader->SetParameter( BlueSharedString( "BokehInfo" ), Vector4( adjustedScale, angle, samplesPerPixel, 0.0f ) );
					DrawInto( *blur, Tr2LoadAction::DONT_CARE, m_depthOfFieldBokehTAAShader, renderContext );
				}
				{

					GPU_REGION( renderContext, "Copy back" );
					DrawInto( *dest, Tr2LoadAction::DONT_CARE, *blur, renderContext );
				}
			}
			else
			{

				{
					GPU_REGION( renderContext, "Bokeh Blend" );
					m_depthOfFieldBokehBlurShader->SetParameter( BlueSharedString( "BlitCurrent" ), dest );
					m_depthOfFieldBokehBlurShader->SetParameter( BlueSharedString( "CoCMap" ), coc );
					m_depthOfFieldBokehBlurShader->SetParameter( BlueSharedString( "BokehInfo" ), Vector4( adjustedScale, 0.0f, 0.0f, 0.0f ) );
					DrawInto( *blur, Tr2LoadAction::DONT_CARE, m_depthOfFieldBokehBlurShader, renderContext );
					if( depthOfField->m_debug == Tr2PPDepthOfFieldEffect::DofDebug_BokehBlend )
					{
						DrawInto( *dest, Tr2LoadAction::DONT_CARE, *blur, renderContext );
						return;
					}
				}
				{
					GPU_REGION( renderContext, "Bokeh Fill" );
					m_depthOfFieldBokehFillShader->SetParameter( BlueSharedString( "BlitCurrent" ), blur );
					m_depthOfFieldBokehFillShader->SetParameter( BlueSharedString( "CoCMap" ), coc );
					m_depthOfFieldBokehFillShader->SetParameter( BlueSharedString( "BokehInfo" ), Vector4( adjustedScale, 0.0f, 0.0f, 0.0f ) );
					DrawInto( *dest, Tr2LoadAction::DONT_CARE, m_depthOfFieldBokehFillShader, renderContext );
				}
			}
		}
	}
}


void TriStepRenderPostProcess::SetRenderTarget( Tr2RenderTarget* rt )
{
	if( rt != GetRenderTarget() )
	{
		m_renderInfo->SetSourceBuffer( rt );
		if( rt != nullptr )
		{
			m_tilesX = rt->GetWidth() / HISTOGRAM_TILE_SIZE_X + 1;
			m_tilesY = rt->GetHeight() / HISTOGRAM_TILE_SIZE_Y + 1;
			m_localHistogramCount = m_tilesX * m_tilesY * 16;
			m_mergeHistogramXDim = m_tilesX * m_tilesY / NUM_TILES_PER_THREAD_GROUP + 1;
		}
	}
}

Tr2RenderTargetPtr TriStepRenderPostProcess::GetRenderTarget() const
{
	return m_renderInfo->GetSourceBuffer().GetRenderTarget();
}
