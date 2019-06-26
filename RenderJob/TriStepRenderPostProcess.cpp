#include "StdAfx.h"
#include "TriStepRenderPostProcess.h"
#include "PostProcess/Tr2PostProcess2.h"
#include "Shader/Parameter/TriTextureParameter.h"

const uint32_t HISTOGRAM_TILE_SIZE_X = 16;
const uint32_t HISTOGRAM_TILE_SIZE_Y = 16;
const uint32_t NUM_TILES_PER_THREAD_GROUP = 256;


TriStepRenderPostProcess::TriStepRenderPostProcess( IRoot* lockobj ) :
	m_quality( HIGH ),
	m_tilesX( 0 ), 
	m_tilesY( 0 ),
	m_localHistogramCount( 0 ),
	m_mergeHistogramXDim( 0 ),
	m_desaturateEnabled( false ),
	m_fadeEnabled( false ),
	m_filmGrainEnabled( false ),
	m_lutEnabled( false ),
	m_vignetteEnabled( false )
{
	m_renderInfo.CreateInstance();
	m_tonemappingEffect.CreateInstance();
	m_tonemappingEffect->StartUpdate();
	m_tonemappingEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/ToneMapping.fx" );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteDetailScroll" ), Vector4( 0.0, 0.0, 0.0, 0.0 ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "GrainColorAmount" ), 0.600000023842 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "Tonemapping" ), 1.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "Desaturate" ), 0.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteColor" ), Vector4( 1.0, 1.0, 1.0, 1.0 ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteSineRange" ), Vector4( 0.0, 1.0, 0.0, 0.0 ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "GrainIntensity" ), 0.00300000002608 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "ColoredGrain" ), 1.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "LUTEnabled" ), 0.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "FadeAmount" ), 0.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "GrimeWeight" ), 0.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureAdjust" ), 2.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "DynamicExposure" ), 0.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "GrainSize" ), 2.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteEnabled" ), 0.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "GrainLuminanceExponent" ), 0.20000000298 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "FadeColor" ), Vector4( 0.0, 0.0, 0.0, 0.0 ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "FilmGrain" ), 1.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureMiddleValue" ), 0.5 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteDetailSize" ), Vector4( 16.0, 16.0, 16.0, 16.0 ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "LUTInfluence" ), 0.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteSineFrequency" ), 1.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureInfluence" ), 1.0 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "BloomBrightness" ), 0.20000000298 );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteIntensity" ), Vector4( 0.0, 0.0, 0.0, 0.0 ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "SaturationFactor" ), 1.0 );
	m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "Grime" ), "res:/texture/global/black.dds" );
	m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "TexLUT" ), "res:/dx9/scene/postprocess/LUTdefault.dds" );
	m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "VignetteDetail" ), "res:/texture/global/white.dds" );
	m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "VignetteShape" ), "res:/texture/global/black.dds" );
	m_tonemappingEffect->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetBlackBuffer() );
	m_tonemappingEffect->SetParameter( BlueSharedString( "BlitOriginal" ), m_renderInfo->GetSourceBufferCopy() );

	m_tonemappingEffect->EndUpdate();
}

TriStepRenderPostProcess::~TriStepRenderPostProcess( void )
{
}

void TriStepRenderPostProcess::py__init__( EveSpaceScene* scene, Tr2RenderTarget* source  )
{
	m_scene = scene;
	m_scene->SetupTAA( m_velocityBuffer, 0, TAA_NONE );

	m_renderInfo->SetSourceBuffer( source );
	
	if( source != nullptr )
	{
		m_tilesX = source->GetWidth() / HISTOGRAM_TILE_SIZE_X + 1;
		m_tilesY = source->GetHeight() / HISTOGRAM_TILE_SIZE_Y + 1;
		m_localHistogramCount = m_tilesX * m_tilesY * 16;
		m_mergeHistogramXDim = m_tilesX * m_tilesY / NUM_TILES_PER_THREAD_GROUP + 1;
	}

	m_tonemappingEffect->StartUpdate();
	m_tonemappingEffect->SetParameter( BlueSharedString( "BlitOriginal" ), m_renderInfo->GetSourceBufferCopy() );
	m_tonemappingEffect->EndUpdate();
}


TriStepResult TriStepRenderPostProcess::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	auto sourceBuffer = m_renderInfo->GetSourceBuffer();

	if( !sourceBuffer )
	{
		return RS_OK;
	}

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

	if( postProcess != nullptr )
	{
		// filter by quality
		switch( m_quality )
		{
		case HIGH:
			godrays = postProcess->GetGodRays();
			filmGrain = postProcess->GetFilmGrain();
			fog = postProcess->GetFog();
#if TRINITY_PLATFORM != TRINITY_DIRECTX9
			dynamicExposure = postProcess->GetDynamicExposure();
#endif
		case MEDIUM:
			bloom = postProcess->GetBloom();
			desaturate = postProcess->GetDesaturate();
			lut = postProcess->GetLut();
			vignette = postProcess->GetVignette();
		case LOW:
			signalLoss = postProcess->GetSignalLoss();
			fade = postProcess->GetFade();
		default:
			break;
		}
#if TRINITY_PLATFORM == TRINITY_DIRECTX11
		if( Tr2Renderer::GetShaderModel() == TR2SM_3_0_DEPTH )
		{
			taa = postProcess->GetTaa();
		}
#endif
	}
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	Tr2Renderer::PushRenderTarget( renderContext );
	Tr2Renderer::PushDepthStencilBuffer( nullDS, renderContext );

	if( ProcessFog( fog ) )
	{
		RenderFog( renderContext, fog );
	}

	if( ProcessGodRays( godrays ) )
	{
		RenderGodRays( renderContext, godrays );
	}

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	// copy the source to the source copy buffer
	if( sourceBuffer->GetMsaaType() > 1 )
	{
		sourceBuffer->GetRenderTarget().Resolve( *m_renderInfo->GetSourceBufferCopyDirectly(), renderContext );
	}

	if( ProcessTaa( taa ) )
	{
		RenderTaa( renderContext, taa );
	}

	if( ProcessDynamicExposure( dynamicExposure ) )
	{
		RenderDynamicExposure( renderContext, dynamicExposure );
	}

	if( ProcessBloom( bloom) )
	{
		RenderBloom( renderContext, bloom );
	}

	ProcessFilmGrain( filmGrain );
	ProcessDesaturate( desaturate );
	ProcessFade( fade );
	ProcessLut( lut );
	ProcessVignette( vignette );

	Tr2Renderer::DrawTexture( m_tonemappingEffect, Vector2( 0, 0 ), Vector2( 1, 1 ) );
	
	if( ProcessSignalLoss( signalLoss ) )
	{
		RenderSignalLoss( renderContext, signalLoss );
	}

	Tr2Renderer::PopDepthStencilBuffer( renderContext );
	Tr2Renderer::PopRenderTarget( renderContext );

	return RS_OK;
}


bool TriStepRenderPostProcess::ProcessBloom( Tr2PPBloomEffect* bloom )
{
	if( bloom && bloom->IsActive() )
	{
		if( m_bloomHighPassFilter == nullptr || m_bloomHorizontalBlur == nullptr || m_bloomVerticalBlur == nullptr )
		{
			m_bloomHighPassFilter.CreateInstance();
			m_bloomHighPassFilter->StartUpdate();
			m_bloomHighPassFilter->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/HighPassFilter.fx" );
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "LuminanceThreshold" ), bloom->m_luminanceThreshold );
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "LuminanceScale" ), bloom->m_luminanceScale );
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetSourceBufferCopy() );

			m_bloomHighPassFilter->SetParameter( BlueSharedString( "ExposureDependency" ), m_exposure != nullptr );
			if( m_exposure != nullptr )
			{
				m_bloomHighPassFilter->SetParameter( BlueSharedString( "Exposure" ), m_exposure );
			}
			m_bloomHighPassFilter->EndUpdate();

			m_bloomHorizontalBlur.CreateInstance();
			m_bloomHorizontalBlur->StartUpdate();
			m_bloomHorizontalBlur->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/BlurBig.fx" );
			m_bloomHorizontalBlur->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetRt1Buffer() );
			m_bloomHorizontalBlur->EndUpdate();

			m_bloomVerticalBlur.CreateInstance();
			m_bloomVerticalBlur->StartUpdate();
			m_bloomVerticalBlur->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/BlurBig.fx" );
			m_bloomVerticalBlur->SetParameter( BlueSharedString( "Direction" ), Vector2( 0, 1 ) );
			m_bloomVerticalBlur->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetRt2Buffer() );
			m_bloomVerticalBlur->EndUpdate();

			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "BloomBrightness" ), bloom->m_bloomBrightness );
			m_tonemappingEffect->SetParameter( BlueSharedString( "GrimeWeight" ), bloom->m_grimeWeight );
			m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "GrimePath" ), bloom->m_grimePath.c_str() );
			m_tonemappingEffect->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetRt1Buffer() );

			m_tonemappingEffect->EndUpdate();

			bloom->SetDirty( false );
		}
		else if( bloom->IsDirty() )
		{
			m_bloomHighPassFilter->StartUpdate();
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "LuminanceThreshold" ), bloom->m_luminanceThreshold );
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "LuminanceScale" ), bloom->m_luminanceScale );
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "ExposureDependency" ), bloom->m_exposureDependency );
			m_bloomHighPassFilter->EndUpdate();

			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "BloomBrightness" ), bloom->m_bloomBrightness );
			m_tonemappingEffect->SetParameter( BlueSharedString( "GrimeWeight" ), bloom->m_grimeWeight );

			TriTextureParameter* resource = dynamic_cast< TriTextureParameter* >( m_tonemappingEffect->GetResourceByName( "GrimePath" ) );
			resource->SetResourcePath( bloom->m_grimePath.c_str() );

			m_tonemappingEffect->EndUpdate();

			bloom->SetDirty( false );
		}

	}
	else
	{
		if( m_bloomHighPassFilter != nullptr || m_bloomHorizontalBlur != nullptr || m_bloomVerticalBlur != nullptr )
		{
			m_bloomHighPassFilter = nullptr;
			m_bloomHorizontalBlur = nullptr;
			m_bloomVerticalBlur = nullptr;

			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetBlackBuffer() );
			m_tonemappingEffect->EndUpdate();
			}		
		}

	return bloom != nullptr && bloom->IsActive();
}

void TriStepRenderPostProcess::RenderBloom( Tr2RenderContext& renderContext, Tr2PPBloomEffect* bloom )
{
	if( !bloom->IsActive() )
	{
		return;
	}
	auto rt1 = m_renderInfo->GetRt1Buffer();
	auto rt2 = m_renderInfo->GetRt2Buffer();
	auto sourceCopy = m_renderInfo->GetSourceBufferCopy();

	Tr2Renderer::PushRenderTarget( *rt1, renderContext );

	HRESULT hr = renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0x00000000, 1.0, 0 );
	if( !SUCCEEDED( hr ) )
	{
		CCP_LOGERR( "Bloom RT1 clear failed" );
	}

	Tr2Renderer::DrawScreenQuad( m_bloomHighPassFilter );
	Tr2Renderer::PopRenderTarget( renderContext );

	Tr2Renderer::PushRenderTarget( *rt2, renderContext );
	hr = renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0x00000000, 1.0, 0 );
	if( !SUCCEEDED( hr ) )
	{
		CCP_LOGERR( "Bloom RT2 clear failed" );
	}
	Tr2Renderer::DrawScreenQuad( m_bloomHorizontalBlur );
	Tr2Renderer::PopRenderTarget( renderContext );

	Tr2Renderer::PushRenderTarget( *rt1, renderContext );
	Tr2Renderer::DrawScreenQuad( m_bloomVerticalBlur );
	Tr2Renderer::PopRenderTarget( renderContext );
}


bool TriStepRenderPostProcess::ProcessGodRays( Tr2PPGodRaysEffect* godrays )
{
	if( godrays && godrays->IsActive() )
	{
		if( m_godRayDownSampleEffect == nullptr || m_godrayEffect == nullptr )
		{
			m_godRayDownSampleEffect.CreateInstance();
			m_godRayDownSampleEffect->StartUpdate();
			m_godRayDownSampleEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/DownsampleDepth.fx" );
			m_godRayDownSampleEffect->EndUpdate();

			m_godrayEffect.CreateInstance();
			m_godrayEffect->StartUpdate();
			m_godrayEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Godrays.fx" );
			m_godrayEffect->SetParameter( BlueSharedString( "Color" ), Vector4( godrays->m_godRayColor ) );
			m_godrayEffect->SetParameter( BlueSharedString( "Intensity" ), Vector4(godrays->m_intensity, 0.0f, 1.0f, 1.0f ) );
			m_godrayEffect->SetParameter( BlueSharedString( "grFactors" ), godrays->grFactors );
			m_godrayEffect->AddResourceTexture2D( BlueSharedString( "NoiseTexMap" ), godrays->m_noiseTexturePath.c_str() );
			m_godrayEffect->SetParameter( BlueSharedString( "DepthMap" ), m_renderInfo->GetRt1Buffer() );
			m_godrayEffect->EndUpdate();
			godrays->SetDirty( false );
		}
		else if( godrays->IsDirty() )
		{
			m_godrayEffect->StartUpdate();
			m_godrayEffect->SetParameter( BlueSharedString( "Color" ), Vector4( godrays->m_godRayColor ) );
			m_godrayEffect->SetParameter( BlueSharedString( "Intensity" ), Vector4( godrays->m_intensity, 0.0f, 1.0f, 1.0f ) );
			m_godrayEffect->SetParameter( BlueSharedString( "grFactors" ), godrays->grFactors );

			TriTextureParameter* resource = dynamic_cast< TriTextureParameter* >( m_godrayEffect->GetResourceByName( "NoiseTexMap" ) );
			resource->SetResourcePath( godrays->m_noiseTexturePath.c_str() );

			m_godrayEffect->EndUpdate();
			godrays->SetDirty( false );
		}
	}
	else 
	{		
		m_godRayDownSampleEffect = nullptr;
		m_godrayEffect = nullptr;
	}

	return godrays != nullptr && godrays->IsActive();
}


void TriStepRenderPostProcess::RenderGodRays( Tr2RenderContext& renderContext, Tr2PPGodRaysEffect* godrays)
{
	auto rt1 = m_renderInfo->GetRt1Buffer();
	auto rt2 = m_renderInfo->GetRt2Buffer();
	auto backBufferRT = m_renderInfo->GetSourceBuffer();

	// Downsample 
	Tr2Renderer::PushRenderTarget( *rt1, renderContext );
	Tr2Renderer::DrawScreenQuad( m_godRayDownSampleEffect );
	Tr2Renderer::PopRenderTarget( renderContext );

	// God rays
	Tr2Renderer::PushRenderTarget( *rt2, renderContext );
	HRESULT hr = renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0x00000000, 1.0, 0 );

	if( !SUCCEEDED( hr ) )
	{
		CCP_LOGERR( "Godray clear failed" );
	}

	Tr2Renderer::DrawScreenQuad( m_godrayEffect );
	Tr2Renderer::PopRenderTarget( renderContext );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
	// Blit
	Tr2Renderer::PushRenderTarget( *backBufferRT, renderContext );
	Tr2Renderer::DrawTexture( *rt2, Vector2( 0, 0 ), Vector2( 1, 1 ) );
	Tr2Renderer::PopRenderTarget( renderContext );
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


void TriStepRenderPostProcess::RenderSignalLoss( Tr2RenderContext& renderContext, Tr2PPSignalLossEffect* signalLoss )
{
	Tr2Renderer::PushRenderTarget( renderContext );
	Tr2Renderer::DrawScreenQuad( m_signalLossEffect );
	Tr2Renderer::PopRenderTarget( renderContext );
}


bool TriStepRenderPostProcess::ProcessDynamicExposure( Tr2PPDynamicExposureEffect* dynamicExposure )
{
	if( dynamicExposure && dynamicExposure->IsActive() )
	{
		if( m_dynamicExposureCreateHistogramShader == nullptr || m_dynamicExposureMergeHistogramShader == nullptr || m_dynamicExposureMeasureExposureShader == nullptr )
		{
			m_exposure.CreateInstance();
			m_localHistograms.CreateInstance();
			m_histogram.CreateInstance();
			
			m_exposure->Create( 8, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 2 );
			m_localHistograms->Create( m_localHistogramCount, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_UINT, 2 );
			m_histogram->Create( 65, Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, 2 );

			m_dynamicExposureCreateHistogramShader.CreateInstance();
			m_dynamicExposureCreateHistogramShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/CreateHistograms.fx" );
			m_dynamicExposureCreateHistogramShader->StartUpdate();
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "MinLuminance" ), log(dynamicExposure->m_minLuminance) );
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "MaxLuminance" ), log( dynamicExposure->m_maxLuminance ) );
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "ScreenTilesX" ), float( m_tilesX ) );
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "LocalHistograms" ), m_localHistograms );
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "BlitOriginal" ), m_renderInfo->GetSourceBufferCopy() );
			m_dynamicExposureCreateHistogramShader->EndUpdate();

			m_dynamicExposureMergeHistogramShader.CreateInstance();
			m_dynamicExposureMergeHistogramShader->StartUpdate();
			m_dynamicExposureMergeHistogramShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/MergeHistograms.fx" );
			m_dynamicExposureMergeHistogramShader->SetParameter( BlueSharedString( "ScreenTilesX" ), float( m_tilesX ) );
			m_dynamicExposureMergeHistogramShader->SetParameter( BlueSharedString( "ScreenTilesY" ), float( m_tilesY ) );
			m_dynamicExposureMergeHistogramShader->SetParameter( BlueSharedString( "LocalHistograms"), m_localHistograms );
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
			m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureInfluence" ), dynamicExposure->m_influence);
			m_tonemappingEffect->SetParameter( BlueSharedString( "MinExposure" ), dynamicExposure->m_minExposure );
			m_tonemappingEffect->SetParameter( BlueSharedString( "MaxExposure" ), dynamicExposure->m_maxExposure );

			// TODO replace with an option
			m_tonemappingEffect->SetParameter( BlueSharedString( "DynamicExposure" ), 1.0f );

			m_tonemappingEffect->EndUpdate();

			// attach the exposure buffer to the bloom
			if( m_bloomHighPassFilter != nullptr )
			{
				m_bloomHighPassFilter->StartUpdate();
				m_bloomHighPassFilter->SetParameter( BlueSharedString( "Exposure" ), m_exposure );
				m_bloomHighPassFilter->EndUpdate();
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
			// TODO replace with an option
			m_tonemappingEffect->SetParameter( BlueSharedString( "DynamicExposure" ), 1.0f );
			m_tonemappingEffect->EndUpdate();

			dynamicExposure->SetDirty( false );
		}
	}	
	else
	{
		if( m_dynamicExposureCreateHistogramShader != nullptr || m_dynamicExposureMeasureExposureShader != nullptr || m_dynamicExposureMergeHistogramShader != nullptr ||
			m_localHistograms != nullptr || m_histogram != nullptr || m_exposure != nullptr )
		{
			m_dynamicExposureCreateHistogramShader = nullptr;
			m_dynamicExposureMeasureExposureShader = nullptr;
			m_dynamicExposureMergeHistogramShader = nullptr;
			m_localHistograms = nullptr;
			m_histogram = nullptr;
			m_exposure = nullptr;

			// TODO replace with an option
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "DynamicExposure" ), 0.0f );
			m_tonemappingEffect->EndUpdate();
		}
		
	}

	return dynamicExposure != nullptr && dynamicExposure->IsActive();
}


void TriStepRenderPostProcess::RenderDynamicExposure( Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure )
{
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


void TriStepRenderPostProcess::ProcessFilmGrain( Tr2PPFilmGrainEffect* filmGrain )
{
	if( filmGrain && filmGrain->IsActive() )
	{
		if( filmGrain->IsDirty() )
		{
			// we only need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "GrainIntensity" ),filmGrain->m_intensity );
			m_tonemappingEffect->SetParameter( BlueSharedString( "ColoredGrain" ), filmGrain->m_colored );
			m_tonemappingEffect->SetParameter( BlueSharedString( "GrainColorAmount" ), filmGrain->m_colorAmount );
			m_tonemappingEffect->SetParameter( BlueSharedString( "GrainSize" ), filmGrain->m_grainSize );
			m_tonemappingEffect->SetParameter( BlueSharedString( "GrainLuminanceExponent" ), filmGrain->m_luminanceExponent );
			// TODO replace with an option
			m_tonemappingEffect->SetParameter( BlueSharedString( "FilmGrain" ), 1.0f );
			m_tonemappingEffect->EndUpdate();

			filmGrain->SetDirty( false );
			m_filmGrainEnabled = true;
		}
	}
	else if( m_filmGrainEnabled )
	{
		// TODO replace with an option
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetParameter( BlueSharedString( "FilmGrain" ), 0.0f );
		m_tonemappingEffect->EndUpdate();
		m_filmGrainEnabled = false;
	}
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
			// TODO replace with an option
			m_tonemappingEffect->SetParameter( BlueSharedString( "Desaturate" ), 1.0f );
			m_tonemappingEffect->EndUpdate();

			desaturate->SetDirty( false );
			m_desaturateEnabled = true;
		}
	}
	else if( m_desaturateEnabled )
	{
		// TODO replace with an option
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetParameter( BlueSharedString( "Desaturate" ), 0.0f );
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
			// TODO replace with an option
			m_tonemappingEffect->SetParameter( BlueSharedString( "FadeAmount" ), fade->m_intensity );
			m_tonemappingEffect->EndUpdate();

			fade->SetDirty( false );
			m_fadeEnabled = true;
		}
	}
	else if( m_fadeEnabled )
	{
		// TODO replace with an option
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
				dynamic_cast< TriTextureParameter* >( resource )->SetResourcePath( lut->m_path.c_str() );
			}
			// TODO replace with an option
			m_tonemappingEffect->SetParameter( BlueSharedString( "LUTEnabled" ), 1.0f );
			m_tonemappingEffect->EndUpdate();

			lut->SetDirty( false );
			m_lutEnabled = true;
		}
	}
	else if( m_lutEnabled )
	{
		// TODO replace with an option
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetParameter( BlueSharedString( "LUTEnabled" ), 0.0f );
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
				dynamic_cast< TriTextureParameter* >( shapeResource )->SetResourcePath( vignette->m_shapePath.c_str() );
			}

			auto detailResource = m_tonemappingEffect->GetResourceByName( "VignetteDetail" );
			if( !detailResource )
			{
				m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "VignetteDetail" ), vignette->m_detailPath.c_str() );
			}
			else
			{
				dynamic_cast< TriTextureParameter* >( detailResource )->SetResourcePath( vignette->m_detailPath.c_str() );
			}

			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteDetailSize" ), Vector4( vignette->m_detail1Size[0], vignette->m_detail1Size[1], vignette->m_detail2Size[0], vignette->m_detail2Size[1] ) );
			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteDetailScroll" ), Vector4( vignette->m_detail1Scroll[0], vignette->m_detail1Scroll[1], vignette->m_detail2Scroll[0], vignette->m_detail2Scroll[1] ) );
			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteColor" ), Vector4( vignette->m_color ) );
			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteIntensity" ), Vector2( vignette->m_intensity, vignette->m_opacity ) );
			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteSineFrequency" ), vignette->m_sineFrequency );
			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteSineRange" ), Vector2( vignette->m_sineMinimum, vignette->m_sineMaximum) );

			// TODO replace with an option
			m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteEnabled" ), 1.0f );
			m_tonemappingEffect->EndUpdate();

			vignette->SetDirty( false );
			m_vignetteEnabled = true;
		}
	}
	else if( m_vignetteEnabled )
	{
		// TODO replace with an option
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteEnabled" ), 0.0f );
		m_tonemappingEffect->EndUpdate();
		m_vignetteEnabled = false;
	}
}

bool TriStepRenderPostProcess::ProcessFog( Tr2PPFogEffect* fog )
{
	if( fog && fog->IsActive() )
	{
		if( m_fogColorEffect == nullptr || m_fogCompositeEffect == nullptr || m_fogHorizontalBlurEffect == nullptr || m_fogVerticalBlurEffect == nullptr )
		{
			m_fogColorEffect.CreateInstance();
			m_fogColorEffect->StartUpdate();
			m_fogColorEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/EnvironmentFogColor.fx" );
			m_fogColorEffect->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetSourceBufferCopy() );
			m_fogColorEffect->SetParameter( BlueSharedString( "Params" ), Vector4( fog->m_nebulaInfluence, fog->m_nebulaBlur, fog->m_originalBrightenOnly, fog->m_colorInfluence ) );
			m_fogColorEffect->SetParameter( BlueSharedString( "Color" ), Vector4( fog->m_color ) );
			m_fogColorEffect->EndUpdate();

			m_fogHorizontalBlurEffect.CreateInstance();
			m_fogHorizontalBlurEffect->StartUpdate();
			m_fogHorizontalBlurEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/BlurBig.fx" );
			m_fogHorizontalBlurEffect->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetRt1Buffer() );
			m_fogHorizontalBlurEffect->EndUpdate();

			m_fogVerticalBlurEffect.CreateInstance();
			m_fogVerticalBlurEffect->StartUpdate();
			m_fogVerticalBlurEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/BlurBig.fx" );
			m_fogVerticalBlurEffect->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetRt2Buffer() );
			m_fogVerticalBlurEffect->SetParameter( BlueSharedString( "Direction" ), Vector2( 0.0f, 1.0f ) );
			m_fogVerticalBlurEffect->EndUpdate();

			m_fogCompositeEffect.CreateInstance();
			m_fogCompositeEffect->StartUpdate();
			m_fogCompositeEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/EnvironmentFogComposit.fx" );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetRt1Buffer() );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BlitOriginal" ), m_renderInfo->GetSourceBufferCopyDirectly( ) ); // this used _fogsource in eve.yaml, but I'm trying _source here
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
		m_fogHorizontalBlurEffect = nullptr;
		m_fogVerticalBlurEffect = nullptr;
	}
	return fog && fog->IsActive();
}

void TriStepRenderPostProcess::RenderFog( Tr2RenderContext& renderContext, Tr2PPFogEffect* fog )
{
	auto sourceBuffer = m_renderInfo->GetSourceBuffer();
	if( sourceBuffer->GetMsaaType() > 1 )
	{
		sourceBuffer->GetRenderTarget().Resolve( *m_renderInfo->GetSourceBufferCopyDirectly(), renderContext );
	}
	else
	{
		Tr2Renderer::PushRenderTarget( *m_renderInfo->GetSourceBufferCopyDirectly(), renderContext );
		Tr2Renderer::DrawTexture( *sourceBuffer );
		renderContext.PopRenderTarget();
	}

	// render fog color
	Tr2Renderer::PushRenderTarget( *m_renderInfo->GetRt1Buffer(), renderContext );
	Tr2Renderer::DrawScreenQuad( m_fogColorEffect );
	Tr2Renderer::PopRenderTarget( renderContext );

	// horizontal blur
	Tr2Renderer::PushRenderTarget( *m_renderInfo->GetRt2Buffer(), renderContext );
	Tr2Renderer::DrawScreenQuad( m_fogHorizontalBlurEffect);
	Tr2Renderer::PopRenderTarget( renderContext );

	// vertical blur
	Tr2Renderer::PushRenderTarget( *m_renderInfo->GetRt1Buffer(), renderContext );
	Tr2Renderer::DrawScreenQuad( m_fogVerticalBlurEffect );
	Tr2Renderer::PopRenderTarget( renderContext );

	// final composite
	Tr2Renderer::PushRenderTarget( *m_renderInfo->GetSourceBuffer(), renderContext );
	Tr2Renderer::DrawScreenQuad( m_fogCompositeEffect );
	Tr2Renderer::PopRenderTarget( renderContext );
}

bool TriStepRenderPostProcess::ProcessTaa( Tr2PPTaaEffect* taa )
{
	if( taa && taa->IsActive() )
	{
		if( m_taaEffect == nullptr || m_accumulationBuffer == nullptr || m_velocityBuffer == nullptr )
		{
			auto source = m_renderInfo->GetSourceBuffer();

			m_accumulationBuffer.CreateInstance();
			m_accumulationBuffer->Create( source->GetWidth(), source->GetHeight(), 1, source->GetFormat() );
			m_accumulationBuffer->m_name = "AccumulationBuffer";

			m_taaEffect.CreateInstance();
			m_taaEffect->StartUpdate();
			m_taaEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/TAA.fx" );
			m_taaEffect->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetSourceBufferCopyDirectly() );
			m_taaEffect->SetParameter( BlueSharedString( "LastFrame" ), m_accumulationBuffer );

			m_velocityBuffer.CreateInstance();
			m_velocityBuffer->m_name = "VelocityMap";
			m_velocityBuffer->Create( source->GetWidth(), source->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_FLOAT, source->GetMsaaType(), 0 );

			if( source->GetMsaaQuality() > 1 )
			{
				m_taaEffect->SetParameter( BlueSharedString( "VelocityMapMSAA" ), m_velocityBuffer );
				m_taaEffect->SetParameter( BlueSharedString( "VelocityMap" ), m_renderInfo->GetBlackBuffer() );
			}
			else
			{
				m_taaEffect->SetParameter( BlueSharedString( "VelocityMap" ), m_velocityBuffer );
				m_taaEffect->SetParameter( BlueSharedString( "VelocityMapMSAA" ), m_renderInfo->GetBlackBuffer() );
			}

			m_taaEffect->SetParameter( BlueSharedString( "BlendingParams0" ), taa->m_blendParams0 );
			m_taaEffect->SetParameter( BlueSharedString( "BlendingParams1" ), taa->m_blendParams1 );
			m_taaEffect->SetParameter( BlueSharedString( "BlendingParams2" ), taa->m_blendParams2 );
			m_taaEffect->SetParameter( BlueSharedString( "DistanceParams" ), taa->m_distanceParams );
			m_taaEffect->SetParameter( BlueSharedString( "EnhancementParams" ), taa->m_enhancementParams );
			m_taaEffect->EndUpdate();

			m_scene->SetupTAA( m_velocityBuffer, 0.5f, TAA_3X );
			taa->SetDirty( false );
		}
	} 
	else
	{	
		if( m_taaEffect != nullptr || m_accumulationBuffer != nullptr || m_velocityBuffer != nullptr )
		{
			m_taaEffect = nullptr;
			m_accumulationBuffer = nullptr;
			m_velocityBuffer = nullptr;
			m_scene->SetupTAA( m_velocityBuffer, 0, TAA_NONE );
		}
	}
	return taa && taa->IsActive();
}

void TriStepRenderPostProcess::RenderTaa( Tr2RenderContext& renderContext, Tr2PPTaaEffect* taa )
{
	auto source = m_renderInfo->GetSourceBuffer();

	m_scene->GetPostProcessPSBuffer()->ApplyBuffer( renderContext );

	Tr2Renderer::PushRenderTarget( *source, renderContext );
	Tr2Renderer::DrawScreenQuad( m_taaEffect );
	Tr2Renderer::PopRenderTarget( renderContext );

	if( source->GetMsaaType() > 1 )
	{
		source->GetRenderTarget().Resolve( *m_renderInfo->GetSourceBufferCopy(), renderContext );
	}

	m_renderInfo->GetSourceBufferCopy()->GetRenderTarget().Resolve( *m_accumulationBuffer, renderContext );

}
