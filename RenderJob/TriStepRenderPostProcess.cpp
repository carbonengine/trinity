#include "StdAfx.h"
#include "TriStepRenderPostProcess.h"
#include "PostProcess/Tr2PostProcess2.h"
#include "Shader/Parameter/TriTextureParameter.h"
#include "PostProcess/Effects/Tr2PPFidelityFXEffect.h"

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

void DrawInto( Tr2TextureAL& dest, Tr2LoadAction::Type loadAction, Tr2Effect* effect, Tr2RenderContext& renderContext )
{
	renderContext.RenderPassHint( { loadAction, Tr2StoreAction::STORE }, {} );
	renderContext.m_esm.PushRenderTarget( dest );
	Tr2Renderer::DrawScreenQuad( renderContext, effect );
	renderContext.m_esm.PopRenderTarget();
}

}


TriStepRenderPostProcess::TriStepRenderPostProcess(IRoot* lockobj) :
	m_quality(HIGH),
	m_tilesX(0),
	m_tilesY(0),
	m_localHistogramCount(0),
	m_mergeHistogramXDim(0),
	m_desaturateEnabled(false),
	m_fadeEnabled(false),
	m_lutEnabled(false),
	m_vignetteEnabled(false),
	m_sceneDirty( false )
{
	m_renderInfo.CreateInstance();
	m_tonemappingEffect.CreateInstance();
	m_tonemappingEffect->StartUpdate();
	m_tonemappingEffect->SetEffectPathName("res:/Graphics/Effect/Managed/Space/PostProcess/ToneMapping.fx");
	m_tonemappingEffect->SetParameter(BlueSharedString("VignetteDetailScroll"), Vector4(0.0, 0.0, 0.0, 0.0));
	m_tonemappingEffect->SetParameter(BlueSharedString("GrainColorAmount"), 0.600000023842f);
	m_tonemappingEffect->SetOption(BlueSharedString("TONE_MAPPING_TOGGLE"), BlueSharedString("TONE_MAPPING_ENABLED"));
	m_tonemappingEffect->SetOption(BlueSharedString("DESATURATE_TOGGLE"), BlueSharedString("DESATURATE_DISABLED"));
	m_tonemappingEffect->SetParameter(BlueSharedString("VignetteColor"), Vector4(1.0, 1.0, 1.0, 1.0));
	m_tonemappingEffect->SetParameter(BlueSharedString("VignetteSineRange"), Vector4(0.0, 1.0, 0.0, 0.0));
	m_tonemappingEffect->SetParameter(BlueSharedString("GrainIntensity"), 0.00300000002608f);
	m_tonemappingEffect->SetOption(BlueSharedString("COLORED_GRAIN_TOGGLE"), BlueSharedString("COLORED_GRAIN_DISABLED"));
	m_tonemappingEffect->SetOption(BlueSharedString("LUT_TOGGLE"), BlueSharedString("LUT_DISABLED"));
	m_tonemappingEffect->SetParameter(BlueSharedString("FadeAmount"), 0.0f);
	m_tonemappingEffect->SetParameter(BlueSharedString("GrimeWeight"), 0.0f);
	m_tonemappingEffect->SetParameter(BlueSharedString("ExposureAdjust"), 1.0f);
	m_tonemappingEffect->SetOption(BlueSharedString("DYNAMIC_EXPOSURE_TOGGLE"), BlueSharedString("DYNAMIC_EXPOSURE_DISABLED"));
	m_tonemappingEffect->SetParameter(BlueSharedString("GrainSize"), 2.0f);
	m_tonemappingEffect->SetOption(BlueSharedString("VIGNETTE_TOGGLE"), BlueSharedString("VIGNETTE_DISABLED"));
	m_tonemappingEffect->SetParameter(BlueSharedString("GrainLuminanceExponent"), 0.20000000298f);
	m_tonemappingEffect->SetParameter(BlueSharedString("FadeColor"), Vector4(0.0, 0.0, 0.0, 0.0));
	m_tonemappingEffect->SetOption(BlueSharedString("FILM_GRAIN_TOGGLE"), BlueSharedString("FILM_GRAIN_DISABLED"));
	m_tonemappingEffect->SetParameter(BlueSharedString("ExposureMiddleValue"), 0.5f);
	m_tonemappingEffect->SetParameter(BlueSharedString("VignetteDetailSize"), Vector4(16.0, 16.0, 16.0, 16.0));
	m_tonemappingEffect->SetParameter(BlueSharedString("LUTInfluence"), 0.0f);
	m_tonemappingEffect->SetParameter(BlueSharedString("VignetteSineFrequency"), 1.0f);
	m_tonemappingEffect->SetParameter(BlueSharedString("ExposureInfluence"), 1.0f);
	m_tonemappingEffect->SetParameter(BlueSharedString("BloomBrightness"), 0.20000000298f);
	m_tonemappingEffect->SetParameter(BlueSharedString("VignetteIntensity"), Vector4(0.0, 0.0, 0.0, 0.0));
	m_tonemappingEffect->SetParameter(BlueSharedString("SaturationFactor"), 1.0f);
	m_tonemappingEffect->AddResourceTexture2D(BlueSharedString("Grime"), "res:/texture/global/black.dds");
	m_tonemappingEffect->AddResourceTexture2D(BlueSharedString("TexLUT"), "res:/dx9/scene/postprocess/LUTdefault.dds");
	m_tonemappingEffect->AddResourceTexture2D(BlueSharedString("VignetteDetail"), "res:/texture/global/white.dds");
	m_tonemappingEffect->AddResourceTexture2D(BlueSharedString("VignetteShape"), "res:/texture/global/black.dds");
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
}

TriStepRenderPostProcess::~TriStepRenderPostProcess(void)
{
	m_scene = nullptr;
}

void TriStepRenderPostProcess::py__init__(EveSpaceScene* scene, Tr2RenderTarget* source)
{
	if (scene == nullptr)
	{
		return;
	}

	m_scene = scene;
	m_sceneDirty = true;
	m_scene->SetupTAA(m_velocityBuffer, 0, TAA_NONE);

	SetRenderTarget( source );
}

void SetDirtyIfNotNull(Tr2PPEffect *effect)
{
	if (nullptr != effect)
	{
		effect->SetDirty(true);
	}
}

bool TriStepRenderPostProcess::OnModified( Be::Var* value )
{
	m_sceneDirty = true;
	return true;
}

TriStepResult TriStepRenderPostProcess::Execute(Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext)
{
	if( !m_renderInfo->Setup( renderContext ) )
	{
		return RS_FAILED;
	}

	auto sourceBuffer = m_renderInfo->GetSourceBuffer();

	if (!sourceBuffer)
	{
		return RS_OK;
	}

	if (m_scene == nullptr)
	{
		return RS_OK;
	}

	GPU_REGION( renderContext, "Post-processing" );

	Tr2PostProcess2Ptr postProcess = m_scene->GetPostProcess();

	Tr2PPGodRaysEffect* godrays = nullptr;
	Tr2PPBloomEffect* bloom = nullptr;
	Tr2PPSignalLossEffect* signalLoss = nullptr;
	Tr2PPDynamicExposureEffectPtr dynamicExposure = nullptr;
	Tr2PPFidelityFXEffectPtr fidelity = nullptr;
	Tr2PPFilmGrainEffectPtr filmGrain = nullptr;
	Tr2PPDesaturateEffectPtr desaturate = nullptr;
	Tr2PPFadeEffectPtr fade = nullptr;
	Tr2PPLutEffectPtr lut = nullptr;
	Tr2PPVignetteEffectPtr vignette = nullptr;
	Tr2PPFogEffectPtr fog = nullptr;
	Tr2PPTaaEffectPtr taa = nullptr;
	Tr2PPDepthOfFieldEffectPtr dof = nullptr;

	if (postProcess != nullptr)
	{
		// filter by quality
		switch (m_quality)
		{
		case HIGH:
			godrays = postProcess->GetGodRays();
			filmGrain = postProcess->GetFilmGrain();
			fog = postProcess->GetFog();
			dynamicExposure = postProcess->GetDynamicExposure();
			fidelity = postProcess->GetFidelityFX();
			dof = postProcess->GetDepthOfField();
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
		if (Tr2Renderer::GetShaderModel() == TR2SM_3_0_DEPTH)
		{
			taa = postProcess->GetTaa();
		}
	}
	renderContext.m_esm.ApplyStandardStates(Tr2EffectStateManager::RM_FULLSCREEN);

	renderContext.m_esm.PushRenderTarget();
	renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );

	if(m_sceneDirty)
	{
		SetDirtyIfNotNull(godrays);
		SetDirtyIfNotNull(bloom);
		SetDirtyIfNotNull(signalLoss);
		SetDirtyIfNotNull(dynamicExposure);
		SetDirtyIfNotNull(filmGrain);
		SetDirtyIfNotNull(desaturate);
		SetDirtyIfNotNull(fade);
		SetDirtyIfNotNull(lut);
		SetDirtyIfNotNull(vignette);
		SetDirtyIfNotNull(fog);
		SetDirtyIfNotNull(taa);
		SetDirtyIfNotNull(fidelity);
		SetDirtyIfNotNull(dof);
		m_sceneDirty = false;
	}

    // Always resolve (if no msaa then we copy)
	auto nonMsaaSource = m_renderInfo->GetTempTexture();
	sourceBuffer->GetRenderTarget().Resolve( *nonMsaaSource, renderContext );
	
	if (ProcessFog(fog))
	{
		RenderFog( nonMsaaSource, renderContext, fog );
	}

	if (ProcessGodRays(godrays))
	{
		RenderGodRays( nonMsaaSource, renderContext, godrays );
	}

	if( ProcessDepthOfField( renderContext, dof ) )
	{
		RenderDepthOfField( nonMsaaSource, renderContext, dof );
	}

	if (ProcessTaa(taa))
	{
		RenderTaa( nonMsaaSource, renderContext, taa );
	}

	if (ProcessDynamicExposure(renderContext, dynamicExposure, bloom))
	{
		RenderDynamicExposure( nonMsaaSource, renderContext, dynamicExposure );
	}

	
	// this needs to be after dynamic exposure, since bloom can be exposure dependent
	Tr2PostProcessRenderInfo::Texture bloomTexture;
	if( ProcessBloom( bloom, dynamicExposure ) )
	{
		bloomTexture = RenderBloom( nonMsaaSource, renderContext, bloom );
	}

	m_tonemappingEffect->SetParameter( BlueSharedString( "BlitCurrent" ), bloomTexture ? bloomTexture.GetRenderTarget() : m_renderInfo->GetBlackTexture() );
	m_tonemappingEffect->SetParameter( BlueSharedString( "BlitOriginal" ), nonMsaaSource );

	ProcessDesaturate(desaturate);
	ProcessFade(fade);
	ProcessLut(lut);
	ProcessVignette(vignette);
	

	bool doFidelity = ProcessFidelityFX( renderContext, fidelity );
	bool doGrain = ProcessFilmGrain( filmGrain );

	if( doFidelity || doGrain )
	{
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
		auto temp = m_renderInfo->GetTempTexture();
		DrawInto( *temp, Tr2LoadAction::DONT_CARE, m_tonemappingEffect, renderContext );
		nonMsaaSource = Tr2PostProcessRenderInfo::Texture();

		if( doFidelity )
		{
			temp = RenderFidelityFX( temp, renderContext, fidelity );
		}
		if( doGrain )
		{
			RenderFilmGrain( temp, renderContext, filmGrain );
		}
		else
		{
			Tr2Renderer::DrawTexture( renderContext, *temp );
		}
	}
	else
	{
		Tr2Renderer::DrawTexture( renderContext, m_tonemappingEffect, Vector2( 0, 0 ), Vector2( 1, 1 ) );
	}

	if (ProcessSignalLoss(signalLoss))
	{
		RenderSignalLoss(renderContext, signalLoss);
	}

	renderContext.m_esm.PopDepthStencilBuffer();
	renderContext.m_esm.PopRenderTarget();

	return RS_OK;
}

// Helper function to blur certain channel of a source render target to a destination render target with a blur type (Big/Small)
void TriStepRenderPostProcess::Blur( Tr2RenderTarget* dest, Tr2RenderTarget* src, Tr2RenderContext& renderContext, BlurType blurType, BlurChannel blurChannel, float size )
{
	uint32_t identifier = blurChannel * 10 + blurType;
	auto lookup = m_blurEffects.find( identifier );


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


		switch( blurType )
		{
		case BlurType::Big:
			effects.first->SetOption( BlueSharedString( "BLUR_TYPE" ), BlueSharedString( "BLUR_BIG" ) );
			effects.second->SetOption( BlueSharedString( "BLUR_TYPE" ), BlueSharedString( "BLUR_BIG" ) );
			break;
		case BlurType::Small:
			effects.first->SetOption( BlueSharedString( "BLUR_TYPE" ), BlueSharedString( "BLUR_SMALL" ) );
			effects.second->SetOption( BlueSharedString( "BLUR_TYPE" ), BlueSharedString( "BLUR_SMALL" ) );
			break;
		default:
			break;
		}

		switch( blurChannel )
		{
		case BlurChannel::r:
			effects.first->SetOption( BlueSharedString( "BLUR_CHANNEL" ), BlueSharedString( "BLUR_CHANNEL_R" ) );
			effects.second->SetOption( BlueSharedString( "BLUR_CHANNEL" ), BlueSharedString( "BLUR_CHANNEL_R" ) );
			break;
		case BlurChannel::g:
			effects.first->SetOption( BlueSharedString( "BLUR_CHANNEL" ), BlueSharedString( "BLUR_CHANNEL_G" ) );
			effects.second->SetOption( BlueSharedString( "BLUR_CHANNEL" ), BlueSharedString( "BLUR_CHANNEL_G" ) );
			break;
		case BlurChannel::b:
			effects.first->SetOption( BlueSharedString( "BLUR_CHANNEL" ), BlueSharedString( "BLUR_CHANNEL_B" ) );
			effects.second->SetOption( BlueSharedString( "BLUR_CHANNEL" ), BlueSharedString( "BLUR_CHANNEL_B" ) );
			break;
		case BlurChannel::rgba:
			effects.first->SetOption( BlueSharedString( "BLUR_CHANNEL" ), BlueSharedString( "BLUR_CHANNEL_RGBA" ) );
			effects.second->SetOption( BlueSharedString( "BLUR_CHANNEL" ), BlueSharedString( "BLUR_CHANNEL_RGBA" ) );
			break;
		default:
			break;
		}

		effects.first->EndUpdate();
		effects.second->EndUpdate();
		m_blurEffects.insert( std::pair( identifier, effects ) );
	}
	else
	{
		effects = lookup->second;
	}
	
	auto rt2 = m_renderInfo->GetTempTexture( 1.0f );
	effects.first->SetParameter( BlueSharedString( "BlitCurrent" ), src );
	DrawInto( *rt2, Tr2LoadAction::DONT_CARE, effects.first, renderContext );

	effects.second->SetParameter( BlueSharedString( "BlitCurrent" ), rt2 );
	DrawInto( *dest, Tr2LoadAction::DONT_CARE, effects.second, renderContext );
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
			m_bloomHighPassFilter->SetEffectPathName("res:/Graphics/Effect/Managed/Space/PostProcess/HighPassFilter.fx");
			m_bloomHighPassFilter->SetParameter(BlueSharedString("LuminanceThreshold"), bloom->m_luminanceThreshold);
			m_bloomHighPassFilter->SetParameter(BlueSharedString("LuminanceScale"), bloom->m_luminanceScale);
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );

			bool hasDynamicExposure = dynamicExposure != nullptr && dynamicExposure->IsActive();
			m_bloomHighPassFilter->SetParameter(BlueSharedString("ExposureDependency"), bloom->m_exposureDependency && hasDynamicExposure ? 1.0f : 0.0f);
			m_bloomHighPassFilter->SetParameter(BlueSharedString("Exposure"), m_exposure);
			m_bloomHighPassFilter->EndUpdate();

			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter(BlueSharedString("BloomBrightness"), bloom->m_bloomBrightness);
			m_tonemappingEffect->SetParameter(BlueSharedString("GrimeWeight"), bloom->m_grimeWeight);
			m_tonemappingEffect->AddResourceTexture2D(BlueSharedString("Grime"), bloom->m_grimePath.c_str());
			m_tonemappingEffect->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );

			m_tonemappingEffect->EndUpdate();

			bloom->SetDirty(false);
		}
		else if( bloom->IsDirty() )
		{
			m_bloomHighPassFilter->StartUpdate();
			m_bloomHighPassFilter->SetParameter(BlueSharedString("LuminanceThreshold"), bloom->m_luminanceThreshold);
			m_bloomHighPassFilter->SetParameter(BlueSharedString("LuminanceScale"), bloom->m_luminanceScale);
			m_bloomHighPassFilter->SetParameter( BlueSharedString( "ExposureDependency" ), exposureDependant ? 1.0f : 0.0f );
			if( exposureDependant )
			{
				m_bloomHighPassFilter->SetParameter( BlueSharedString( "Exposure" ), m_exposure );
			}
			m_bloomHighPassFilter->EndUpdate();

			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter(BlueSharedString("BloomBrightness"), bloom->m_bloomBrightness);
			m_tonemappingEffect->SetParameter(BlueSharedString("GrimeWeight"), bloom->m_grimeWeight);

			TriTextureParameter* resource = dynamic_cast<TriTextureParameter*>(m_tonemappingEffect->GetResourceByName("Grime"));
			resource->SetResourcePath(bloom->m_grimePath.c_str());

			m_tonemappingEffect->EndUpdate();

			bloom->SetDirty(false);
		}

	}
	else
	{
		if( m_bloomHighPassFilter != nullptr )
		{
			m_bloomHighPassFilter = nullptr;

			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter(BlueSharedString("BlitCurrent"), m_renderInfo->GetBlackTexture());
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

	Blur( rt1, rt1, renderContext );

	return rt1;
}


bool TriStepRenderPostProcess::ProcessGodRays(Tr2PPGodRaysEffect* godrays)
{
	if (godrays && godrays->IsActive())
	{
		if( m_godrayEffect == nullptr )
		{
			m_godrayEffect.CreateInstance();
			m_godrayEffect->StartUpdate();
			m_godrayEffect->SetEffectPathName("res:/Graphics/Effect/Managed/Space/PostProcess/Godrays.fx");
			m_godrayEffect->SetParameter(BlueSharedString("Color"), Vector4(godrays->m_godRayColor));
			m_godrayEffect->SetParameter(BlueSharedString("Intensity"), Vector4(godrays->m_intensity, 0.0f, 1.0f, 1.0f));
			m_godrayEffect->SetParameter(BlueSharedString("grFactors"), godrays->grFactors);
			m_godrayEffect->AddResourceTexture2D(BlueSharedString("NoiseTexMap"), godrays->m_noiseTexturePath.c_str());
			m_godrayEffect->SetParameter( BlueSharedString( "DepthMap" ), PLACEHOLDER );
			m_godrayEffect->EndUpdate();
			godrays->SetDirty(false);
		}
		else if( godrays->IsDirty() )
		{
			m_godrayEffect->StartUpdate();
			m_godrayEffect->SetParameter(BlueSharedString("Color"), Vector4(godrays->m_godRayColor));
			m_godrayEffect->SetParameter(BlueSharedString("Intensity"), Vector4(godrays->m_intensity, 0.0f, 1.0f, 1.0f));
			m_godrayEffect->SetParameter(BlueSharedString("grFactors"), godrays->grFactors);

			TriTextureParameter* resource = dynamic_cast<TriTextureParameter*>(m_godrayEffect->GetResourceByName("NoiseTexMap"));
			resource->SetResourcePath(godrays->m_noiseTexturePath.c_str());

			m_godrayEffect->EndUpdate();
			godrays->SetDirty(false);
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


bool TriStepRenderPostProcess::ProcessSignalLoss(Tr2PPSignalLossEffect* signalLoss)
{
	if (signalLoss && signalLoss->IsActive())
	{
		if (m_signalLossEffect == nullptr)
		{
			m_signalLossEffect.CreateInstance();
			m_signalLossEffect->StartUpdate();
			m_signalLossEffect->SetEffectPathName("res:/Graphics/Effect/Managed/Space/PostProcess/SignalLoss.fx");
			m_signalLossEffect->SetParameter(BlueSharedString("NoiseStrength"), signalLoss->m_strength);
			m_signalLossEffect->EndUpdate();
			signalLoss->SetDirty(false);
		}
		else if (signalLoss->IsDirty())
		{
			m_signalLossEffect->StartUpdate();
			m_signalLossEffect->SetParameter(BlueSharedString("NoiseStrength"), signalLoss->m_strength);
			m_signalLossEffect->EndUpdate();
			signalLoss->SetDirty(false);
		}
	}
	else
	{
		m_signalLossEffect = nullptr;
	}

	return signalLoss != nullptr && signalLoss->IsActive();
}


void TriStepRenderPostProcess::RenderSignalLoss(Tr2RenderContext& renderContext, Tr2PPSignalLossEffect* signalLoss)
{
	GPU_REGION( renderContext, "Signal Loss" );

	renderContext.m_esm.PushRenderTarget();
	Tr2Renderer::DrawScreenQuad( renderContext, m_signalLossEffect);
	renderContext.m_esm.PopRenderTarget();
}


bool TriStepRenderPostProcess::ProcessDynamicExposure( Tr2RenderContext &renderContext, Tr2PPDynamicExposureEffect* dynamicExposure, Tr2PPBloomEffect* bloom)
{
	if( !m_exposure || !m_exposure->IsValid() )
	{
		m_exposure = nullptr;
		m_exposure.CreateInstance();
		m_exposure->Create(8, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 2);
		const float clearValue[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderContext.ClearUav( *m_exposure, clearValue);
	}
	if (dynamicExposure && dynamicExposure->IsActive())
	{
		if (m_dynamicExposureCreateHistogramShader == nullptr || m_dynamicExposureMergeHistogramShader == nullptr || m_dynamicExposureMeasureExposureShader == nullptr)
		{
			m_localHistograms.CreateInstance();
			m_histogram.CreateInstance();

			m_localHistograms->Create(m_localHistogramCount, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_UINT, 2);
			m_histogram->Create(65, Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, 2);

			m_dynamicExposureCreateHistogramShader.CreateInstance();
			m_dynamicExposureCreateHistogramShader->SetEffectPathName("res:/Graphics/Effect/Managed/Space/PostProcess/CreateHistograms.fx");
			m_dynamicExposureCreateHistogramShader->StartUpdate();
			m_dynamicExposureCreateHistogramShader->SetParameter(BlueSharedString("MinLuminance"), log(dynamicExposure->m_minLuminance));
			m_dynamicExposureCreateHistogramShader->SetParameter(BlueSharedString("MaxLuminance"), log(dynamicExposure->m_maxLuminance));
			m_dynamicExposureCreateHistogramShader->SetParameter(BlueSharedString("ScreenTilesX"), float(m_tilesX));
			m_dynamicExposureCreateHistogramShader->SetParameter(BlueSharedString("LocalHistograms"), m_localHistograms);
			m_dynamicExposureCreateHistogramShader->SetParameter( BlueSharedString( "BlitOriginal" ), PLACEHOLDER );
			m_dynamicExposureCreateHistogramShader->EndUpdate();

			m_dynamicExposureMergeHistogramShader.CreateInstance();
			m_dynamicExposureMergeHistogramShader->StartUpdate();
			m_dynamicExposureMergeHistogramShader->SetEffectPathName("res:/Graphics/Effect/Managed/Space/PostProcess/MergeHistograms.fx");
			m_dynamicExposureMergeHistogramShader->SetParameter(BlueSharedString("ScreenTilesX"), float(m_tilesX));
			m_dynamicExposureMergeHistogramShader->SetParameter(BlueSharedString("ScreenTilesY"), float(m_tilesY));
			m_dynamicExposureMergeHistogramShader->SetParameter(BlueSharedString("LocalHistograms"), m_localHistograms);
			m_dynamicExposureMergeHistogramShader->SetParameter(BlueSharedString("Histogram"), m_histogram);
			m_dynamicExposureMergeHistogramShader->EndUpdate();

			m_dynamicExposureMeasureExposureShader.CreateInstance();
			m_dynamicExposureMeasureExposureShader->StartUpdate();
			m_dynamicExposureMeasureExposureShader->SetEffectPathName("res:/Graphics/Effect/Managed/Space/PostProcess/MeasureExposure.fx");
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("MinLuminance"), log(dynamicExposure->m_minLuminance));
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("MaxLuminance"), log(dynamicExposure->m_maxLuminance));
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("MinBrightness"), dynamicExposure->m_minBrightness);
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("MaxBrightness"), dynamicExposure->m_maxBrightness);
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("IncreaseSpeed"), dynamicExposure->m_increaseSpeed);
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("DecreaseSpeed"), dynamicExposure->m_decreaseSpeed);
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("MinExposure"), dynamicExposure->m_minExposure);
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("MaxExposure"), dynamicExposure->m_maxExposure);
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("Histogram"), m_histogram);
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("Exposure"), m_exposure);

			m_dynamicExposureMeasureExposureShader->EndUpdate();

			// we also need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter(BlueSharedString("Exposure"), m_exposure);
			m_tonemappingEffect->SetParameter(BlueSharedString("Histogram"), m_histogram);
			m_tonemappingEffect->SetParameter(BlueSharedString("ExposureAdjust"), pow(2.0f, dynamicExposure->m_adjustment));
			m_tonemappingEffect->SetParameter(BlueSharedString("ExposureMiddleValue"), dynamicExposure->m_middleValue);
			m_tonemappingEffect->SetParameter(BlueSharedString("ExposureInfluence"), dynamicExposure->m_influence);
			m_tonemappingEffect->SetParameter(BlueSharedString("MinExposure"), dynamicExposure->m_minExposure);
			m_tonemappingEffect->SetParameter(BlueSharedString("MaxExposure"), dynamicExposure->m_maxExposure);

			m_tonemappingEffect->SetOption(BlueSharedString("DYNAMIC_EXPOSURE_TOGGLE"), BlueSharedString("DYNAMIC_EXPOSURE_ENABLED"));

			m_tonemappingEffect->EndUpdate();

			// mark the bloom as dirty so it can decide what to do with the exposure
			if( bloom != nullptr )
			{
				bloom->SetDirty( true );
			}

			dynamicExposure->SetDirty(false);
		}
		else if (dynamicExposure->IsDirty())
		{
			m_dynamicExposureCreateHistogramShader->StartUpdate();
			m_dynamicExposureCreateHistogramShader->SetParameter(BlueSharedString("MinLuminance"), log(dynamicExposure->m_minLuminance));
			m_dynamicExposureCreateHistogramShader->SetParameter(BlueSharedString("MaxLuminance"), log(dynamicExposure->m_maxLuminance));
			m_dynamicExposureCreateHistogramShader->SetParameter(BlueSharedString("MinBrightness"), dynamicExposure->m_minBrightness);
			m_dynamicExposureCreateHistogramShader->SetParameter(BlueSharedString("MaxBrightness"), dynamicExposure->m_maxBrightness);
			m_dynamicExposureCreateHistogramShader->EndUpdate();

			m_dynamicExposureMeasureExposureShader->StartUpdate();
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("MinLuminance"), log(dynamicExposure->m_minLuminance));
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("MaxLuminance"), log(dynamicExposure->m_maxLuminance));
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("MinBrightness"), dynamicExposure->m_minBrightness);
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("MaxBrightness"), dynamicExposure->m_maxBrightness);
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("IncreaseSpeed"), dynamicExposure->m_increaseSpeed);
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("DecreaseSpeed"), dynamicExposure->m_decreaseSpeed);
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("MinExposure"), dynamicExposure->m_minExposure);
			m_dynamicExposureMeasureExposureShader->SetParameter(BlueSharedString("MaxExposure"), dynamicExposure->m_maxExposure);
			m_dynamicExposureMeasureExposureShader->EndUpdate();

			// we also need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter(BlueSharedString("ExposureAdjust"), pow(2.0f, dynamicExposure->m_adjustment));
			m_tonemappingEffect->SetParameter(BlueSharedString("ExposureMiddleValue"), dynamicExposure->m_middleValue);
			m_tonemappingEffect->SetParameter(BlueSharedString("ExposureInfluence"), dynamicExposure->m_influence);
			m_tonemappingEffect->SetParameter(BlueSharedString("MinExposure"), dynamicExposure->m_minExposure);
			m_tonemappingEffect->SetParameter(BlueSharedString("MaxExposure"), dynamicExposure->m_maxExposure);
			m_tonemappingEffect->SetOption(BlueSharedString("DYNAMIC_EXPOSURE_TOGGLE"), BlueSharedString("DYNAMIC_EXPOSURE_ENABLED"));
			m_tonemappingEffect->EndUpdate();

			dynamicExposure->SetDirty(false);
		}
	}
	else
	{
		if (m_dynamicExposureCreateHistogramShader != nullptr || m_dynamicExposureMeasureExposureShader != nullptr || m_dynamicExposureMergeHistogramShader != nullptr ||
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
			m_tonemappingEffect->SetOption(BlueSharedString("DYNAMIC_EXPOSURE_TOGGLE"), BlueSharedString("DYNAMIC_EXPOSURE_DISABLED"));
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
	auto lhbuffer = m_localHistograms->GetGpuBuffer(0);
	renderContext.ClearUav(*lhbuffer, m_uintValue);

	// Clear histograms
	auto hbuffer = m_histogram->GetGpuBuffer(0);
	renderContext.ClearUav(*hbuffer, m_uintValue);

	// Create histograms
	Tr2Renderer::RunComputeShader(m_dynamicExposureCreateHistogramShader, m_tilesX, m_tilesY, 1, renderContext);

	// Merge histogram
	Tr2Renderer::RunComputeShader(m_dynamicExposureMergeHistogramShader, m_mergeHistogramXDim, 1, 1, renderContext);

	// Measure histogram
	Tr2Renderer::RunComputeShader(m_dynamicExposureMeasureExposureShader, 1, 1, 1, renderContext);
}

bool TriStepRenderPostProcess::ProcessFidelityFX( Tr2RenderContext& renderContext, Tr2PPFidelityFXEffect* fx )
{
	if( fx && fx->IsActive() )
	{
		if( fx->IsDirty() || !m_fidelityFXShader )
		{
			if( !m_fidelityFXShader )
			{
				m_fidelityFXShader.CreateInstance();
				m_fidelityFXShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/CAS.fx" );
			}

			AF1 outWidth = static_cast<AF1>( m_renderInfo->GetSourceBuffer()->GetWidth() );
			AF1 outHeight = static_cast<AF1>( m_renderInfo->GetSourceBuffer()->GetHeight() );

			union
			{
				AU1 u[4];
				float f[4];
			} const0 = {}, const1 = {};
			CasSetup( const0.u, const1.u, fx->m_intensity, outWidth, outHeight, outWidth, outHeight );

			m_fidelityFXShader->StartUpdate();
			m_fidelityFXShader->SetParameter( BlueSharedString( "InputTexture" ), PLACEHOLDER );
			m_fidelityFXShader->SetParameter( BlueSharedString( "OutputTexture" ), PLACEHOLDER );
			m_fidelityFXShader->SetParameter( BlueSharedString( "const0" ), Vector4( const0.f[0], const0.f[1], const0.f[2], const0.f[3] ) );
			m_fidelityFXShader->SetParameter( BlueSharedString( "const1" ), Vector4( const1.f[0], const1.f[1], const1.f[2], const1.f[3] ) );
			m_fidelityFXShader->EndUpdate();

			fx->SetDirty( false );
		}

		return true;
	}
	else
	{
		m_fidelityFXShader = nullptr;
		return false;
	}
}

Tr2PostProcessRenderInfo::Texture TriStepRenderPostProcess::RenderFidelityFX( Tr2RenderTarget* src, Tr2RenderContext& renderContext, Tr2PPFidelityFXEffect* fx )
{
	const uint32_t CAS_THREAD_GROUP_WORK_REGION_DIM = 16;

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	auto temp = m_renderInfo->GetTempTexture( 1.f, Tr2RenderContextEnum::EX_BIND_UNORDERED_ACCESS );
	m_fidelityFXShader->SetParameter( BlueSharedString( "InputTexture" ), src );
	m_fidelityFXShader->SetParameter( BlueSharedString( "OutputTexture" ), temp );
	int dispatchX = ( src->GetWidth() + ( CAS_THREAD_GROUP_WORK_REGION_DIM - 1 ) ) / CAS_THREAD_GROUP_WORK_REGION_DIM;
	int dispatchY = ( src->GetHeight() + ( CAS_THREAD_GROUP_WORK_REGION_DIM - 1 ) ) / CAS_THREAD_GROUP_WORK_REGION_DIM;
	Tr2Renderer::RunComputeShader( m_fidelityFXShader, dispatchX, dispatchY, 1, renderContext );
	return temp;
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
			m_grainShader->SetOption( BlueSharedString( "COLORED_GRAIN_TOGGLE" ),
				BlueSharedString( filmGrain->m_colored ? "COLORED_GRAIN_ENABLED" : "COLORED_GRAIN_DISABLED" ) );
			m_grainShader->SetParameter( BlueSharedString( "GrainIntensity" ), filmGrain->m_intensity );
			m_grainShader->SetParameter( BlueSharedString( "ColoredGrain" ), filmGrain->m_colored ? 1.0f : 0.0f );
			m_grainShader->SetParameter( BlueSharedString( "GrainColorAmount" ), filmGrain->m_colorAmount );
			m_grainShader->SetParameter( BlueSharedString( "GrainSize" ), filmGrain->m_grainSize );
			m_grainShader->SetParameter( BlueSharedString( "GrainLuminanceExponent" ), filmGrain->m_luminanceExponent );
			
			m_grainShader->SetParameter( BlueSharedString( "InputTexture" ), PLACEHOLDER );
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

void TriStepRenderPostProcess::ProcessDesaturate(Tr2PPDesaturateEffect* desaturate)
{
	if (desaturate && desaturate->IsActive())
	{
		if (desaturate->IsDirty())
		{
			// we only need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter(BlueSharedString("SaturationFactor"), desaturate->m_intensity);
			m_tonemappingEffect->SetOption(BlueSharedString("DESATURATE_TOGGLE"), BlueSharedString("DESATURATE_ENABLED"));
			m_tonemappingEffect->EndUpdate();

			desaturate->SetDirty(false);
			m_desaturateEnabled = true;
		}
	}
	else if (m_desaturateEnabled)
	{
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetOption(BlueSharedString("DESATURATE_TOGGLE"), BlueSharedString("DESATURATE_DISABLED"));
		m_tonemappingEffect->EndUpdate();
		m_desaturateEnabled = false;
	}
}

void TriStepRenderPostProcess::ProcessFade(Tr2PPFadeEffect* fade)
{
	if (fade && fade->IsActive())
	{
		if (fade->IsDirty())
		{
			// we only need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter(BlueSharedString("FadeColor"), Vector4(fade->m_color));
			// A shader option can be placed here, although the complexity of checking when to toggle it may outweigh the benefits of the optimization
			m_tonemappingEffect->SetParameter(BlueSharedString("FadeAmount"), fade->m_intensity);
			m_tonemappingEffect->EndUpdate();

			fade->SetDirty(false);
			m_fadeEnabled = true;
		}
	}
	else if (m_fadeEnabled)
	{
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetParameter(BlueSharedString("FadeAmount"), 0.0f);
		m_tonemappingEffect->EndUpdate();
		m_fadeEnabled = false;
	}
}

void TriStepRenderPostProcess::ProcessLut(Tr2PPLutEffect* lut)
{
	if (lut && lut->IsActive())
	{
		if (lut->IsDirty())
		{
			// we only need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter(BlueSharedString("LUTInfluence"), lut->m_influence);
			auto resource = m_tonemappingEffect->GetResourceByName("TexLUT");
			if (!resource)
			{
				m_tonemappingEffect->AddResourceTexture2D(BlueSharedString("TexLUT"), lut->m_path.c_str());
			}
			else
			{
				const auto param = dynamic_cast<TriTextureParameter*>(resource);
				const auto currPath = param->GetResourcePath();
				const std::string possibleNewPathStr = lut->m_path.c_str();

				const std::wstring possibleNewPathWstr(possibleNewPathStr.begin(), possibleNewPathStr.end());

				if (currPath != possibleNewPathWstr)
				{
					param->SetResourcePath(lut->m_path.c_str());
				}
			}
			m_tonemappingEffect->SetOption(BlueSharedString("LUT_TOGGLE"), BlueSharedString("LUT_ENABLED"));
			m_tonemappingEffect->EndUpdate();

			lut->SetDirty(false);
			m_lutEnabled = true;
		}
	}
	else if (m_lutEnabled)
	{
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetOption(BlueSharedString("LUT_TOGGLE"), BlueSharedString("LUT_DISABLED"));
		m_tonemappingEffect->EndUpdate();
		m_lutEnabled = false;
	}
}

void TriStepRenderPostProcess::ProcessVignette(Tr2PPVignetteEffect* vignette)
{
	if (vignette && vignette->IsActive())
	{
		if (vignette->IsDirty())
		{
			// we only need to update the tonemapping buffer
			m_tonemappingEffect->StartUpdate();

			auto shapeResource = m_tonemappingEffect->GetResourceByName("VignetteShape");
			if (!shapeResource)
			{
				m_tonemappingEffect->AddResourceTexture2D(BlueSharedString("VignetteShape"), vignette->m_shapePath.c_str());
			}
			else
			{
				dynamic_cast<TriTextureParameter*>(shapeResource)->SetResourcePath(vignette->m_shapePath.c_str());
			}

			auto detailResource = m_tonemappingEffect->GetResourceByName("VignetteDetail");
			if (!detailResource)
			{
				m_tonemappingEffect->AddResourceTexture2D(BlueSharedString("VignetteDetail"), vignette->m_detailPath.c_str());
			}
			else
			{
				dynamic_cast<TriTextureParameter*>(detailResource)->SetResourcePath(vignette->m_detailPath.c_str());
			}

			m_tonemappingEffect->SetParameter(BlueSharedString("VignetteDetailSize"), Vector4(vignette->m_detail1Size[0], vignette->m_detail1Size[1], vignette->m_detail2Size[0], vignette->m_detail2Size[1]));
			m_tonemappingEffect->SetParameter(BlueSharedString("VignetteDetailScroll"), Vector4(vignette->m_detail1Scroll[0], vignette->m_detail1Scroll[1], vignette->m_detail2Scroll[0], vignette->m_detail2Scroll[1]));
			m_tonemappingEffect->SetParameter(BlueSharedString("VignetteColor"), Vector4(vignette->m_color));
			m_tonemappingEffect->SetParameter(BlueSharedString("VignetteIntensity"), Vector2(vignette->m_intensity, vignette->m_opacity));
			m_tonemappingEffect->SetParameter(BlueSharedString("VignetteSineFrequency"), vignette->m_sineFrequency);
			m_tonemappingEffect->SetParameter(BlueSharedString("VignetteSineRange"), Vector2(vignette->m_sineMinimum, vignette->m_sineMaximum));

			m_tonemappingEffect->SetOption(BlueSharedString("VIGNETTE_TOGGLE"), BlueSharedString("VIGNETTE_ENABLED"));
			m_tonemappingEffect->EndUpdate();

			vignette->SetDirty(false);
			m_vignetteEnabled = true;
		}
	}
	else if (m_vignetteEnabled)
	{
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetOption(BlueSharedString("VIGNETTE_TOGGLE"), BlueSharedString("VIGNETTE_DISABLED"));
		m_tonemappingEffect->EndUpdate();
		m_vignetteEnabled = false;
	}
}

bool TriStepRenderPostProcess::ProcessFog(Tr2PPFogEffect* fog)
{
	if (fog && fog->IsActive())
	{
		if (m_fogColorEffect == nullptr || m_fogCompositeEffect == nullptr)
		{
			m_fogColorEffect.CreateInstance();
			m_fogColorEffect->StartUpdate();
			m_fogColorEffect->SetEffectPathName("res:/Graphics/Effect/Managed/Space/PostProcess/EnvironmentFogColor.fx");
			m_fogColorEffect->SetParameter(BlueSharedString("BlitCurrent"), PLACEHOLDER);
			m_fogColorEffect->SetParameter(BlueSharedString("Params"), Vector4(fog->m_nebulaInfluence, fog->m_nebulaBlur, fog->m_originalBrightenOnly, fog->m_colorInfluence));
			m_fogColorEffect->SetParameter(BlueSharedString("Color"), Vector4(fog->m_color));
			m_fogColorEffect->EndUpdate();

			m_fogCompositeEffect.CreateInstance();
			m_fogCompositeEffect->StartUpdate();
			m_fogCompositeEffect->SetEffectPathName("res:/Graphics/Effect/Managed/Space/PostProcess/EnvironmentFogComposit.fx");
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_fogCompositeEffect->SetParameter( BlueSharedString( "BlitOriginal" ), PLACEHOLDER ); // this used _fogsource in eve.yaml, but I'm trying _source here
			m_fogCompositeEffect->SetParameter(BlueSharedString("FogParameters"), Vector4(fog->m_totalAmount, fog->m_totalPower, fog->m_backgroundOcclusion, fog->m_intensity));
			m_fogCompositeEffect->SetParameter(BlueSharedString("BrightnessAdjustment"), Vector4(fog->m_brightnessThreshold0, fog->m_brightnessThreshold1, fog->m_brightnessAdjustmentAmount, 0.0f));
			m_fogCompositeEffect->SetParameter(BlueSharedString("BlendFunction0"), Vector4(fog->m_blendDistance0, fog->m_blendBias0, fog->m_blendAmount0, fog->m_blendPower0));
			m_fogCompositeEffect->SetParameter(BlueSharedString("BlendFunction1"), Vector4(fog->m_blendDistance1, fog->m_blendBias1, fog->m_blendAmount1, fog->m_blendPower1));
			m_fogCompositeEffect->SetParameter(BlueSharedString("BlendFunction2"), Vector4(fog->m_blendDistance2, fog->m_blendBias2, fog->m_blendAmount2, fog->m_blendPower2));
			m_fogCompositeEffect->SetParameter(BlueSharedString("AreaSize"), Vector4(fog->m_areaSize, fog->m_areaScale.x));
			m_fogCompositeEffect->SetParameter(BlueSharedString("AreaCenter"), Vector4(fog->m_areaCenter, fog->m_areaScale.y));
			m_fogCompositeEffect->EndUpdate();

			fog->SetDirty(false);
		}
		if (fog->IsDirty())
		{
			m_fogColorEffect->StartUpdate();
			m_fogColorEffect->SetParameter(BlueSharedString("Params"), Vector4(fog->m_nebulaInfluence, fog->m_nebulaBlur, fog->m_originalBrightenOnly, fog->m_colorInfluence));
			m_fogColorEffect->SetParameter(BlueSharedString("Color"), Vector4(fog->m_color));
			m_fogColorEffect->EndUpdate();

			m_fogCompositeEffect->StartUpdate();
			m_fogCompositeEffect->SetParameter(BlueSharedString("FogParameters"), Vector4(fog->m_totalAmount, fog->m_totalPower, fog->m_backgroundOcclusion, fog->m_intensity));
			m_fogCompositeEffect->SetParameter(BlueSharedString("BrightnessAdjustment"), Vector4(fog->m_brightnessThreshold0, fog->m_brightnessThreshold1, fog->m_brightnessAdjustmentAmount, 0.0f));
			m_fogCompositeEffect->SetParameter(BlueSharedString("BlendFunction0"), Vector4(fog->m_blendDistance0, fog->m_blendBias0, fog->m_blendAmount0, fog->m_blendPower0));
			m_fogCompositeEffect->SetParameter(BlueSharedString("BlendFunction1"), Vector4(fog->m_blendDistance1, fog->m_blendBias1, fog->m_blendAmount1, fog->m_blendPower1));
			m_fogCompositeEffect->SetParameter(BlueSharedString("BlendFunction2"), Vector4(fog->m_blendDistance2, fog->m_blendBias2, fog->m_blendAmount2, fog->m_blendPower2));
			m_fogCompositeEffect->SetParameter(BlueSharedString("AreaSize"), Vector4(fog->m_areaSize, fog->m_areaScale.x));
			m_fogCompositeEffect->SetParameter(BlueSharedString("AreaCenter"), Vector4(fog->m_areaCenter, fog->m_areaScale.y));
			m_fogCompositeEffect->EndUpdate();

			fog->SetDirty(false);
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
	Blur( rt1, rt1, renderContext );

	// final composite
	m_fogCompositeEffect->SetParameter( BlueSharedString( "BlitCurrent" ), rt1 );
	m_fogCompositeEffect->SetParameter( BlueSharedString( "BlitOriginal" ), tempCopy );
	DrawInto( *dest, Tr2LoadAction::DONT_CARE, m_fogCompositeEffect, renderContext );
}

bool TriStepRenderPostProcess::ProcessTaa(Tr2PPTaaEffect* taa)
{
	if (taa && taa->IsActive())
	{
		if( m_taaEffect == nullptr || m_accumulationBuffer == nullptr || m_velocityBuffer == nullptr )
		{
			auto source = m_renderInfo->GetSourceBuffer();

			m_accumulationBuffer.CreateInstance();
			m_accumulationBuffer->Create(source->GetWidth(), source->GetHeight(), 1, source->GetFormat());
			m_accumulationBuffer->m_name = "AccumulationBuffer";

			m_taaEffect.CreateInstance();
			m_taaEffect->StartUpdate();
			m_taaEffect->SetEffectPathName("res:/Graphics/Effect/Managed/Space/PostProcess/TAA.fx");
			m_taaEffect->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_taaEffect->SetParameter(BlueSharedString("LastFrame"), m_accumulationBuffer);

			m_velocityBuffer.CreateInstance();
			m_velocityBuffer->m_name = "VelocityMap";
			m_velocityBuffer->Create(source->GetWidth(), source->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_FLOAT, source->GetMsaaType(), 0);

			if (source->GetMsaaType() > 1)
			{
				m_taaEffect->SetParameter(BlueSharedString("VelocityMapMSAA"), m_velocityBuffer);
				m_taaEffect->SetParameter(BlueSharedString("VelocityMap"), m_renderInfo->GetBlackTexture());
			}
			else
			{
				m_taaEffect->SetParameter(BlueSharedString("VelocityMap"), m_velocityBuffer);
				m_taaEffect->SetParameter(BlueSharedString("VelocityMapMSAA"), m_renderInfo->GetBlackTexture());
			}

			m_taaEffect->SetParameter(BlueSharedString("BlendingParams0"), taa->m_blendParams0);
			m_taaEffect->SetParameter(BlueSharedString("BlendingParams1"), taa->m_blendParams1);
			m_taaEffect->SetParameter(BlueSharedString("BlendingParams2"), taa->m_blendParams2);
			m_taaEffect->SetParameter(BlueSharedString("DistanceParams"), taa->m_distanceParams);
			m_taaEffect->SetParameter(BlueSharedString("EnhancementParams"), taa->m_enhancementParams);
			m_taaEffect->EndUpdate();

			m_scene->SetupTAA(m_velocityBuffer, 0.5f, TAA_3X);
			taa->SetDirty( false );
		}
		else if( taa->IsDirty() )
		{
			m_taaEffect->StartUpdate();
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
		if (m_taaEffect != nullptr || m_accumulationBuffer != nullptr || m_velocityBuffer != nullptr)
		{
			m_taaEffect = nullptr;
			m_accumulationBuffer = nullptr;
			m_velocityBuffer = nullptr;
			m_scene->SetupTAA(m_velocityBuffer, 0, TAA_NONE);
		}
	}
	return taa && taa->IsActive();
}

void TriStepRenderPostProcess::RenderTaa( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPTaaEffect* taa )
{
	GPU_REGION( renderContext, "TAA" );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	auto source = m_renderInfo->GetSourceBuffer();

	m_scene->GetPostProcessPSBuffer()->ApplyBuffer(renderContext);

	auto temp = m_renderInfo->GetTempTexture();
	DrawInto( *temp, Tr2LoadAction::DONT_CARE, *dest, renderContext );

	m_taaEffect->SetParameter( BlueSharedString( "BlitCurrent" ), temp );
	DrawInto( *dest, Tr2LoadAction::DONT_CARE, m_taaEffect, renderContext );
	DrawInto( *m_accumulationBuffer, Tr2LoadAction::DONT_CARE, *dest, renderContext );
}

bool TriStepRenderPostProcess::ProcessDepthOfField( Tr2RenderContext& renderContext, Tr2PPDepthOfFieldEffect* fx )
{
	if( fx && fx->IsActive() )
	{
		if( !m_depthOfFieldBokehBlendShader || !m_depthOfFieldCoCShader || !m_depthOfFieldBokehBackgroundBlurShader ||! m_depthOfFieldBokehBackgroundFillShader ||
			!m_depthOfFieldBokehForegroundBlurShader || !m_depthOfFieldBokehForegroundFillShader )
		{
			// we just created the effect
			m_depthOfFieldBokehBlendShader.CreateInstance();
			m_depthOfFieldBokehBlendShader->StartUpdate();
			m_depthOfFieldBokehBlendShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/DepthOfFieldBlend.fx" );
			m_depthOfFieldBokehBlendShader->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_depthOfFieldBokehBlendShader->SetParameter( BlueSharedString( "BokehForegroundBlurMap" ), PLACEHOLDER );
			m_depthOfFieldBokehBlendShader->SetParameter( BlueSharedString( "BokehBackgroundBlurMap" ), PLACEHOLDER );
			m_depthOfFieldBokehBlendShader->SetParameter( BlueSharedString( "CoCMap" ), PLACEHOLDER );
			m_depthOfFieldBokehBlendShader->SetParameter( BlueSharedString( "BokehInfo" ), Vector4( fx->m_scale, 0.0, 0.0, 0.0 ) );
			m_depthOfFieldBokehBlendShader->EndUpdate();

			m_depthOfFieldBokehBackgroundBlurShader.CreateInstance();
			m_depthOfFieldBokehBackgroundBlurShader->StartUpdate();
			m_depthOfFieldBokehBackgroundBlurShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Bokeh.fx" );
			m_depthOfFieldBokehBackgroundBlurShader->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_depthOfFieldBokehBackgroundBlurShader->SetParameter( BlueSharedString( "CoCMap" ), PLACEHOLDER );
			m_depthOfFieldBokehBackgroundBlurShader->SetParameter( BlueSharedString( "BokehInfo" ), Vector4( fx->m_scale * 2.0f, 0.0, 1.0, 0.0 ) );
			m_depthOfFieldBokehBackgroundBlurShader->SetOption( BlueSharedString( "BOKEH_PIXEL_METHOD" ), BlueSharedString( "BOKEH_PIXEL_AVERAGE" ) );
			m_depthOfFieldBokehBackgroundBlurShader->SetOption( BlueSharedString( "BOKEH_SHAPE" ), BlueSharedString( "BOKEH_SHAPE_DISK" ) );
			m_depthOfFieldBokehBackgroundBlurShader->EndUpdate();

			m_depthOfFieldBokehBackgroundFillShader.CreateInstance();
			m_depthOfFieldBokehBackgroundFillShader->StartUpdate();
			m_depthOfFieldBokehBackgroundFillShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Bokeh.fx" );
			m_depthOfFieldBokehBackgroundFillShader->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_depthOfFieldBokehBackgroundFillShader->SetParameter( BlueSharedString( "CoCMap" ), PLACEHOLDER );
			m_depthOfFieldBokehBackgroundFillShader->SetParameter( BlueSharedString( "BokehInfo" ), Vector4( fx->m_scale, 0.0, 1.0, 0.0 ) );
			m_depthOfFieldBokehBackgroundFillShader->SetOption( BlueSharedString( "BOKEH_PIXEL_METHOD" ), BlueSharedString( "BOKEH_PIXEL_MAX" ) );
			m_depthOfFieldBokehBackgroundFillShader->SetOption( BlueSharedString( "BOKEH_SHAPE" ), BlueSharedString( "BOKEH_SHAPE_DISK" ) );
			m_depthOfFieldBokehBackgroundFillShader->EndUpdate();

			m_depthOfFieldBokehForegroundBlurShader.CreateInstance();
			m_depthOfFieldBokehForegroundBlurShader->StartUpdate();
			m_depthOfFieldBokehForegroundBlurShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Bokeh.fx" );
			m_depthOfFieldBokehForegroundBlurShader->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_depthOfFieldBokehForegroundBlurShader->SetParameter( BlueSharedString( "CoCMap" ), PLACEHOLDER );
			m_depthOfFieldBokehForegroundBlurShader->SetParameter( BlueSharedString( "BokehInfo" ), Vector4( fx->m_scale * 2.0f, 1.0, 0.0, 0.0 ) );
			m_depthOfFieldBokehForegroundBlurShader->SetOption( BlueSharedString( "BOKEH_PIXEL_METHOD" ), BlueSharedString( "BOKEH_PIXEL_AVERAGE" ) );
			m_depthOfFieldBokehForegroundBlurShader->SetOption( BlueSharedString( "BOKEH_SHAPE" ), BlueSharedString( "BOKEH_SHAPE_DISK" ) );
			m_depthOfFieldBokehForegroundBlurShader->EndUpdate();

			m_depthOfFieldBokehForegroundFillShader.CreateInstance();
			m_depthOfFieldBokehForegroundFillShader->StartUpdate();
			m_depthOfFieldBokehForegroundFillShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Bokeh.fx" );
			m_depthOfFieldBokehForegroundFillShader->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			m_depthOfFieldBokehForegroundFillShader->SetParameter( BlueSharedString( "CoCMap" ), PLACEHOLDER );
			m_depthOfFieldBokehForegroundFillShader->SetParameter( BlueSharedString( "BokehInfo" ), Vector4( fx->m_scale, 1.0, 0.0, 0.0 ) );
			m_depthOfFieldBokehForegroundFillShader->SetOption( BlueSharedString( "BOKEH_PIXEL_METHOD" ), BlueSharedString( "BOKEH_PIXEL_MAX" ) );
			m_depthOfFieldBokehForegroundFillShader->SetOption( BlueSharedString( "BOKEH_SHAPE" ), BlueSharedString( "BOKEH_SHAPE_DISK" ) );
			m_depthOfFieldBokehForegroundFillShader->EndUpdate();

			m_depthOfFieldCoCShader.CreateInstance();
			m_depthOfFieldCoCShader->StartUpdate();
			m_depthOfFieldCoCShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/CircleOfConfusion.fx" );
			m_depthOfFieldCoCShader->SetParameter( BlueSharedString( "FocalInfo" ), Vector4( fx->m_focalDistance, fx->m_focalLength, 0.0, 0.0 ) );
			m_depthOfFieldCoCShader->EndUpdate();
		}
		else if( fx->IsDirty() )
		{
			// we just changed the effect
			m_depthOfFieldCoCShader->StartUpdate();
			m_depthOfFieldCoCShader->SetParameter( BlueSharedString( "FocalInfo" ), Vector4( fx->m_focalDistance, fx->m_focalLength, 0.0, 0.0 ) );
			m_depthOfFieldCoCShader->EndUpdate();

			m_depthOfFieldBokehBackgroundBlurShader->StartUpdate();
			m_depthOfFieldBokehBackgroundBlurShader->SetParameter( BlueSharedString( "BokehInfo" ), Vector4( fx->m_scale * 2.0f, 0.0, 1.0, 0.0 ) );
			m_depthOfFieldBokehBackgroundBlurShader->EndUpdate();

			m_depthOfFieldBokehBackgroundFillShader->StartUpdate();
			m_depthOfFieldBokehBackgroundFillShader->SetParameter( BlueSharedString( "BokehInfo" ), Vector4( fx->m_scale, 0.0, 1.0, 0.0 ) );
			m_depthOfFieldBokehBackgroundFillShader->EndUpdate();

			m_depthOfFieldBokehForegroundBlurShader->StartUpdate();
			m_depthOfFieldBokehForegroundBlurShader->SetParameter( BlueSharedString( "BokehInfo" ), Vector4( fx->m_scale * 2.0f, 1.0, 0.0, 0.0 ) );
			m_depthOfFieldBokehForegroundBlurShader->EndUpdate();

			m_depthOfFieldBokehForegroundFillShader->StartUpdate();
			m_depthOfFieldBokehForegroundFillShader->SetParameter( BlueSharedString( "BokehInfo" ), Vector4( fx->m_scale, 1.0, 0.0, 0.0 ) );
			m_depthOfFieldBokehForegroundFillShader->EndUpdate();
		}
		fx->SetDirty( false );

	}
	else
	{
		if( m_depthOfFieldBokehBlendShader || m_depthOfFieldCoCShader || m_depthOfFieldBokehBackgroundBlurShader || m_depthOfFieldBokehBackgroundFillShader || m_depthOfFieldBokehForegroundFillShader || m_depthOfFieldBokehForegroundBlurShader )
		{
			// we have just deleted the effect
			m_depthOfFieldBokehBlendShader = nullptr;
			m_depthOfFieldCoCShader = nullptr;
			m_depthOfFieldBokehBackgroundBlurShader = nullptr;
			m_depthOfFieldBokehBackgroundFillShader = nullptr;
			m_depthOfFieldBokehForegroundBlurShader = nullptr;
			m_depthOfFieldBokehForegroundFillShader = nullptr;
		}
	}
	return fx && fx->IsActive();
}

void TriStepRenderPostProcess::RenderDepthOfField( Tr2RenderTarget* dest, Tr2RenderContext& renderContext, Tr2PPDepthOfFieldEffect* depthOfField )
{
	GPU_REGION( renderContext, "DepthOfField" );
	{
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

		auto result = m_renderInfo->GetTempTexture( 1.0f );
		auto fgBlur = m_renderInfo->GetTempTexture( 1.0f );
		auto fgFill = m_renderInfo->GetTempTexture( 1.0f );
		auto bgBlur = m_renderInfo->GetTempTexture( 1.0f );
		auto bgFill = m_renderInfo->GetTempTexture( 1.0f );

		auto coc = m_renderInfo->GetTempTexture( 0.5, Tr2RenderContextEnum::EX_NONE, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM );
		{
			GPU_REGION( renderContext, "CoC" );
			// Render the CoC - r is near, g is far, black is focus
			DrawInto( *coc, Tr2LoadAction::DONT_CARE, m_depthOfFieldCoCShader, renderContext );
			// Blur the near CoC
			Blur( coc, coc, renderContext, BlurType::Small, BlurChannel::r, 1.0 );
		} 
		{
			GPU_REGION( renderContext, "Bokeh Foreground Blur" );
			// Bokeh foreground Blur
			m_depthOfFieldBokehForegroundBlurShader->SetParameter( BlueSharedString( "BlitCurrent" ), dest );
			m_depthOfFieldBokehForegroundBlurShader->SetParameter( BlueSharedString( "CoCMap" ), coc );
			DrawInto( *fgBlur, Tr2LoadAction::DONT_CARE, m_depthOfFieldBokehForegroundBlurShader, renderContext );
		}
		{
			GPU_REGION( renderContext, "Bokeh Background Blur" );
			// Bokeh background Blur
			m_depthOfFieldBokehBackgroundBlurShader->SetParameter( BlueSharedString( "BlitCurrent" ), dest );
			m_depthOfFieldBokehBackgroundBlurShader->SetParameter( BlueSharedString( "CoCMap" ), coc );
			DrawInto( *bgBlur, Tr2LoadAction::DONT_CARE, m_depthOfFieldBokehBackgroundBlurShader, renderContext );
		}
		{
			GPU_REGION( renderContext, "Bokeh Foreground Fill" );
			// Bokeh foreground Fill
			m_depthOfFieldBokehForegroundFillShader->SetParameter( BlueSharedString( "BlitCurrent" ), fgBlur );
			m_depthOfFieldBokehForegroundFillShader->SetParameter( BlueSharedString( "CoCMap" ), coc );
			DrawInto( *fgFill, Tr2LoadAction::DONT_CARE, m_depthOfFieldBokehForegroundFillShader, renderContext );
		}
		{
			GPU_REGION( renderContext, "Bokeh Background Fill" );
			// Bokeh background Fill
			m_depthOfFieldBokehBackgroundFillShader->SetParameter( BlueSharedString( "BlitCurrent" ), bgBlur );
			m_depthOfFieldBokehBackgroundFillShader->SetParameter( BlueSharedString( "CoCMap" ), coc );
			DrawInto( *bgFill, Tr2LoadAction::DONT_CARE, m_depthOfFieldBokehBackgroundFillShader, renderContext );
		}
		{
			GPU_REGION( renderContext, "Bokeh Blend" );
			// Depth of Field Blend
			m_depthOfFieldBokehBlendShader->SetParameter( BlueSharedString( "BlitCurrent" ), dest );
			m_depthOfFieldBokehBlendShader->SetParameter( BlueSharedString( "BokehForegroundBlurMap" ), fgFill );
			m_depthOfFieldBokehBlendShader->SetParameter( BlueSharedString( "BokehBackgroundBlurMap" ), bgFill );
			m_depthOfFieldBokehBlendShader->SetParameter( BlueSharedString( "CoCMap" ), coc );
			DrawInto( *result, Tr2LoadAction::DONT_CARE, m_depthOfFieldBokehBlendShader, renderContext );
		}
		
		DrawInto( *dest, Tr2LoadAction::LOAD, *result, renderContext );
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
