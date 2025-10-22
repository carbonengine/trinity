#include "StdAfx.h"
#include "TriStepRenderPostProcess.h"
#include "PostProcess/Tr2PostProcess2.h"
#include "Shader/Parameter/TriTextureParameter.h"
#include "Include/TriMath.h"
#include "TriSettingsRegistrar.h"

// FidelityFX headers
#define A_CPU
#include "ffx_a.h"
#include "ffx_cas.h"

extern float g_eveSpaceSceneGammaBrightness;

bool g_upscalingDebugView = false;
TRI_REGISTER_SETTING( "upscalingDebugView", g_upscalingDebugView );

bool g_frameGenDebugView = false;
TRI_REGISTER_SETTING( "frameGenDebugView", g_frameGenDebugView );

bool g_newBloom = true;
TRI_REGISTER_SETTING( "newBloom", g_newBloom );

int32_t g_dynamicExposureQualityRequirement = TriStepRenderPostProcess::PostProcessingQuality::MEDIUM;
TRI_REGISTER_SETTING( "dynamicExposureQualityRequirement", g_dynamicExposureQualityRequirement );

namespace
{
const uint32_t HISTOGRAM_TILE_SIZE_X = 16;
const uint32_t HISTOGRAM_TILE_SIZE_Y = 16;
const uint32_t NUM_TILES_PER_THREAD_GROUP = 256;
static Tr2UpscalingAL::Result s_lastUpscalingResult;

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

namespace GaussianDistribution
{

float NormalDistribution( float x, float sigma, float weight )
{
	const float dx = std::abs( x );
	const float clampedOneMinusDX = std::max( 0.0f, 1.0f - ( dx * dx ) );

	if( weight > 1.0f )
	{
		return std::pow( clampedOneMinusDX, weight );
	}
	const float gaussian = std::exp( -TRI_PI * 5.0f * ( ( dx * dx ) / ( sigma * sigma ) ) );
	return Lerp( gaussian, clampedOneMinusDX, weight );
}

GaussianData CalculateGaussianPassParameters( float radius, float centerWeight, float normalizingFactor, Vector3 overallWeight, Vector2 direction )
{

	float clampedRadius = std::clamp( radius, 0.0001f, (float)Bloom::MAX_FILTER_STEPS - 1 );
	int integerRadius = (int)std::ceilf( clampedRadius );

	std::vector<std::pair<float, float>> taps;
	taps.reserve( Bloom::MAX_FILTER_STEPS );
	float weightSum = 0;
	// calculate the weights and offsets
	for( int i = -integerRadius; i <= integerRadius; i += 2 )
	{
		float weight = NormalDistribution( (float)i, clampedRadius, centerWeight );

		// if we are in the last step, we don't want to tap outside of the radius, so we will set this to 0
		float offsetWeight = i == integerRadius ? 0 : NormalDistribution( (float)i + 1, clampedRadius, centerWeight );

		float sampleWeight = weight + offsetWeight;
		if( sampleWeight == 0 )
		{
			continue;
		}
		float offset = ( (float)i + offsetWeight / sampleWeight ) * normalizingFactor;

		// add the weight to the sum, so pixels that are outside of the screen still decrease the values of the pixels that land inside of the screen
		// this is done so the pixels inside of the screen don't become brighter
		weightSum += weight + offsetWeight;

		if( offset < -1 || offset > 1 )
		{
			continue;
		}

		taps.push_back( std::make_pair( sampleWeight, offset ) );
	}

	// Since we pack the 2x weight and offset into a vector4 it is important that it is a multiple of 2...
	if( taps.size() % 2 > 0 )
	{
		taps.push_back( std::make_pair( 0.0f, 0.0f ) );
	}

	GaussianData data;
	data.overallWeight = overallWeight;
	data.count = (uint32_t)taps.size() / 2;
	
	std::fill( std::begin( data.weightOffset ), std::end( data.weightOffset ), Vector4(0,0,0,0) );

	uint32_t index = 0;

	// Fill in the data
	for( uint32_t i = 0; i < taps.size(); i+=2 )
	{
		// pack 2 weight and offset into a vector4
		float weight1 = taps[i].first / weightSum;
		float offset1 = taps[i].second;
		float weight2 = taps[i + 1].first / weightSum;
		float offset2 = taps[i + 1].second;

		data.weightOffset[index++] = Vector4(weight1, offset1, weight2, offset2);
	}

	return data;
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
	m_lutsEnabled( 0 ),
	m_vignetteEnabled( false ),
	m_sceneDirty( false ),
	m_lastFrameTime( std::numeric_limits<long long>().max() ),
	m_upscalingContextID( Tr2UpscalingAL::INVALID_CONTEXT_ID ),
	m_bloomDebugMode( BloomDebugMode::BLOOM_DEBUG_NONE ),
	m_useNewBloom( g_newBloom )
{
	m_renderInfo.CreateInstance();
	m_tonemappingEffect.CreateInstance();
	m_tonemappingEffect->StartUpdate();
	m_tonemappingEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/ToneMapping.fx" );
	m_tonemappingEffect->SetParameter( BlueSharedString( "VignetteDetailScroll" ), Vector4( 0.0, 0.0, 0.0, 0.0 ) );
	m_tonemappingEffect->SetParameter( BlueSharedString( "GrainColorAmount" ), 0.600000023842f );
	m_tonemappingEffect->SetOption( BlueSharedString( "TONE_MAPPING_METHOD" ), BlueSharedString( "TONE_MAPPING_ACES" ) );
	m_tonemappingEffect->SetOption( BlueSharedString( "COLOR_CORRECTION_TOGGLE" ), BlueSharedString( "COLOR_CORRECTION_ENABLED" ) );
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
	m_tonemappingEffect->SetParameter(BlueSharedString("LUTInfluence_0"), 0.0f);
	m_tonemappingEffect->SetParameter(BlueSharedString("LUTInfluence_1"), 0.0f);
	m_tonemappingEffect->SetParameter(BlueSharedString("LUTInfluence_2"), 0.0f);
	m_tonemappingEffect->SetParameter(BlueSharedString("LUTInfluence_3"), 0.0f);
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
	m_tonemappingEffect->SetParameter( BlueSharedString( "OutputGamma" ), g_eveSpaceSceneGammaBrightness );

	m_tonemappingEffect->EndUpdate();

	m_reactiveMaskEffect.CreateInstance();
	m_reactiveMaskEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/ReactiveMask.fx" );

	m_transparencyMaskEffect.CreateInstance();
	m_transparencyMaskEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/TransparencyMask.fx" );
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

	if( m_scene == nullptr )
	{
		return RS_OK;
	}

	auto sourceBuffer = m_renderInfo->GetSourceBuffer();

	if( !sourceBuffer || !sourceBuffer->IsValid() )
	{
		CCP_LOGERR( "TriStepRenderPostProcess::Execute: Source buffer is invalid!" );
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
	std::vector<const Tr2PPLutEffect*> luts = std::vector<const Tr2PPLutEffect*>();
	Tr2PPVignetteEffectPtr vignette = nullptr;
	Tr2PPFogEffectPtr fog = nullptr;
	Tr2PPTaaEffectPtr taa = nullptr;
	Tr2PPDepthOfFieldEffectPtr dof = nullptr;
	Tr2PPTonemappingEffectPtr tonemapping = nullptr;
	Tr2PPColorCorrectionEffectPtr colorCorrection = nullptr;

	if( postProcess != nullptr )
	{
		// filter by quality
		switch( m_quality )
		{
		case HIGH:
			godrays = postProcess->GetGodRays();
			filmGrain = postProcess->GetFilmGrain();
			fog = postProcess->GetFog();
		case MEDIUM:
			bloom = postProcess->GetBloom();
			desaturate = postProcess->GetDesaturate();
			vignette = postProcess->GetVignette();
		case LOW:
			tonemapping = postProcess->GetTonemapping();
			signalLoss = postProcess->GetSignalLoss();
			fade = postProcess->GetFade();
			postProcess->GetLuts( luts );
		default:
			taa = postProcess->GetTaa();
			colorCorrection = postProcess->GetColorCorrection();
			break;
		}

		dynamicExposure = m_quality >= g_dynamicExposureQualityRequirement ? postProcess->GetDynamicExposure() : nullptr;

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
		SetDirtyIfNotNull( vignette );
		SetDirtyIfNotNull( fog );
		SetDirtyIfNotNull( taa );
		SetDirtyIfNotNull( dof );
		SetDirtyIfNotNull( tonemapping );
		SetDirtyIfNotNull( colorCorrection );
	}
	// Processing effects will set velocity map if it is needed
	m_scene->SetVelocityMap( nullptr );

	if( m_upscalingContextID == Tr2UpscalingAL::INVALID_CONTEXT_ID )
	{
		m_upscalingContextID = Tr2Renderer::GetUpscalingContextID();
	}
	
	auto upscalingInfo = renderContext.GetPrimaryRenderContext().GetUpscalingInfo( m_upscalingContextID );
	auto upscalingContext = renderContext.GetPrimaryRenderContext().GetUpscalingContext( m_upscalingContextID );

	auto upscalingEnabled = upscalingInfo.technique != Tr2UpscalingAL::NONE;
	Tr2PostProcessRenderInfo::Texture output;
	if( upscalingEnabled )
	{
		output = m_renderInfo->GetTempTexture( upscalingInfo.displayWidth, upscalingInfo.displayHeight, Tr2RenderContextEnum::EX_NONE, renderContext.GetPrimaryRenderContext().GetBackBufferFormat() );
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

	bool hasDynamicExposure = ProcessDynamicExposure( renderContext, dynamicExposure, bloom, postProcess );
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
		// need to set the hudless texture as it is needed for frame generation (and needs to be set before the upscaling call)
		upscalingContext->SetHudLessTexture( output->GetTexture() );

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
	m_tonemappingEffect->SetParameter( BlueSharedString( "OutputGamma" ), g_eveSpaceSceneGammaBrightness );

	if( bloom && m_bloomDebugMode != BloomDebugMode::BLOOM_DEBUG_NONE )
	{
		m_tonemappingEffect->SetParameter( BlueSharedString( "BlitOriginal" ), bloomTexture );
	}

	ProcessDesaturate( desaturate );
	ProcessFade( fade );
	ProcessLut( luts );
	ProcessVignette( vignette );

	if( colorCorrection )
	{
		ProcessColorCorrection( colorCorrection );
	}
	else
	{
		m_tonemappingEffect->SetOption( BlueSharedString( "COLOR_CORRECTION_TOGGLE" ), BlueSharedString( "COLOR_CORRECTION_DISABLED" ) );
	}

	if( tonemapping )
	{
		ProcessTonemapping( tonemapping );
	}
	else
	{
		m_tonemappingEffect->SetOption( BlueSharedString( "TONE_MAPPING_METHOD" ), BlueSharedString( "TONE_MAPPING_DISABLED" ) );
	}

	bool doGrain = ProcessFilmGrain( filmGrain );
	if( !upscalingInfo.temporal || doGrain )
	{
		if( upscalingEnabled && !upscalingInfo.temporal )
		{
			
			auto temp = m_renderInfo->GetTempTexture( "Tonemapping Result" );

			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
			DrawInto( *temp, Tr2LoadAction::DONT_CARE, m_tonemappingEffect, renderContext );
						
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
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
		DrawInto( *output, Tr2LoadAction::DONT_CARE, m_tonemappingEffect, renderContext );
		output = RenderSharpening( output, renderContext );
		Tr2Renderer::DrawTexture( renderContext, *output );
	}

	if( ProcessSignalLoss( signalLoss ) )
	{
		RenderSignalLoss( output, renderContext, signalLoss );
	}

	RenderDynamicExposureDebug( renderContext, dynamicExposure );

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

		if( m_downSamplerLuminancePreserve == nullptr || m_downSampler == nullptr || m_upsamplerVertical == nullptr || m_upsamplerHorizontal == nullptr )
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
			m_tonemappingEffect->SetParameter( BlueSharedString( "GrimeWeight" ), bloom->m_grimeWeight );
			m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( "Grime" ), bloom->m_grimePath.c_str() );
			m_tonemappingEffect->SetParameter( BlueSharedString( "BlitCurrent" ), PLACEHOLDER );
			if(m_useNewBloom)
			{
				m_tonemappingEffect->SetParameter( BlueSharedString( "BloomBrightness" ), 1.0f );
			}
			else
			{
				m_tonemappingEffect->SetParameter( BlueSharedString( "BloomBrightness" ), bloom->m_bloomBrightness );
			}

			m_tonemappingEffect->EndUpdate();

			m_downSamplerLuminancePreserve.CreateInstance();
			m_downSamplerLuminancePreserve->StartUpdate();
			m_downSamplerLuminancePreserve->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Downsample.fx" );
			m_downSamplerLuminancePreserve->SetParameter( BlueSharedString( "Exposure" ), m_exposure );
			m_downSamplerLuminancePreserve->SetOption( BlueSharedString( "EXPOSURE_DEPENDANCE" ), hasDynamicExposure ? BlueSharedString( "EXPOSURE_DEPENDANCE_ON" ) : BlueSharedString("EXPOSURE_DEPENDANCE_OFF" ));
			m_downSamplerLuminancePreserve->SetOption( BlueSharedString( "LUNINANCE_PRESERVE" ), BlueSharedString( "LUNINANCE_PRESERVE_ON" ) );
			m_downSamplerLuminancePreserve->SetParameter( BlueSharedString( "LuminanceThreshold" ), bloom->m_luminanceThreshold );

			m_downSamplerLuminancePreserve->EndUpdate();
			
			m_downSampler.CreateInstance();
			m_downSampler->StartUpdate();
			m_downSampler->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Downsample.fx" );
			m_downSampler->SetOption( BlueSharedString( "LUNINANCE_PRESERVE" ), BlueSharedString( "LUNINANCE_PRESERVE_OFF" ) );
			m_downSampler->EndUpdate();
			
			m_upsamplerHorizontal.CreateInstance();
			m_upsamplerHorizontal->StartUpdate();
			m_upsamplerHorizontal->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Upsample.fx" );
			m_upsamplerHorizontal->EndUpdate();

			m_upsamplerVertical.CreateInstance();
			m_upsamplerVertical->StartUpdate();
			m_upsamplerVertical->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Upsample.fx" );
			m_upsamplerVertical->SetOption( BlueSharedString( "UPSAMPLING_STEP" ), BlueSharedString( "UPSAMPLING_STEP_SECOND" ) );
			m_upsamplerVertical->EndUpdate();

			m_bloomConstantBuffer = Tr2ConstantBufferAL();

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

			m_downSamplerLuminancePreserve->StartUpdate();
			m_downSamplerLuminancePreserve->SetParameter( BlueSharedString( "LuminanceThreshold" ), bloom->m_luminanceThreshold );
			m_downSamplerLuminancePreserve->EndUpdate();

			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "GrimeWeight" ), bloom->m_grimeWeight );
			if( m_useNewBloom )
			{
				m_tonemappingEffect->SetParameter( BlueSharedString( "BloomBrightness" ), 1.0f );
			}
			else
			{
				m_tonemappingEffect->SetParameter( BlueSharedString( "BloomBrightness" ), bloom->m_bloomBrightness );
			}

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
			m_downSamplerLuminancePreserve = nullptr;
			m_downSampler = nullptr;
			m_upsamplerVertical = nullptr;
			m_upsamplerHorizontal = nullptr;
			m_bloomDebugShader = nullptr;
			m_tonemappingEffect->StartUpdate();
			m_tonemappingEffect->SetParameter( BlueSharedString( "BlitCurrent" ), m_renderInfo->GetBlackTexture() );
			m_tonemappingEffect->SetParameter( BlueSharedString( "Grime" ), m_renderInfo->GetBlackTexture() );
			m_tonemappingEffect->EndUpdate();
		}
	}

	return bloom != nullptr && bloom->IsActive();
}


Tr2PostProcessRenderInfo::Texture TriStepRenderPostProcess::RenderBloom( Tr2PostProcessRenderInfo::Texture& dest, Tr2RenderContext& renderContext, Tr2PPBloomEffect* bloom )
{
	GPU_REGION( renderContext, "Bloom" );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	if( !m_useNewBloom )
	{
		auto rt1 = m_renderInfo->GetTempTexture( "Bloom", 0.5f );
		m_bloomHighPassFilter->SetParameter( BlueSharedString( "BlitCurrent" ), dest );
		DrawInto( *rt1, Tr2LoadAction::DONT_CARE, m_bloomHighPassFilter, renderContext );

		auto blurContext = PostProcessBlur::CreateBlurContext( 0.5f );
		Blur( *rt1, *rt1, renderContext, blurContext );

		return rt1;
	}

	int depth = 0;
	auto black = m_renderInfo->GetBlackTexture();

	float currentSize = 0.5f;
	uint32_t minDim = std::min( dest.GetRenderTarget()->GetHeight(), dest.GetRenderTarget()->GetWidth() );

	std::array<Tr2PostProcessRenderInfo::Texture, Bloom::MAX_BLOOM_STEPS> downsampleTexture;
	std::array<Tr2PostProcessRenderInfo::Texture, Bloom::MAX_BLOOM_STEPS> upsampleHorizontalTexture;
	std::array<Tr2PostProcessRenderInfo::Texture, Bloom::MAX_BLOOM_STEPS> upsampleTexture;

	for( int i = 0; i < Bloom::MAX_BLOOM_STEPS; ++i )
	{
		if( (uint32_t)( (float)minDim * currentSize ) == 0 )
		{
			break;
		}
		auto name = "Downsample_" + std::to_string( i );
		downsampleTexture[i] = m_renderInfo->GetTempTexture( name.c_str(), currentSize );

		name = "Upsample_" + std::to_string( i );
		upsampleTexture[i] = m_renderInfo->GetTempTexture( name.c_str(), currentSize );

		if( m_bloomDebugMode != BloomDebugMode::BLOOM_DEBUG_NONE )
		{
			name = "Upsample_Horizontal_" + std::to_string( i );
			upsampleHorizontalTexture[i] = m_renderInfo->GetTempTexture( name.c_str(), currentSize );
		}
		
		currentSize *= 0.5f;
		++depth;
	}

	auto lastRt = dest;
	{
		GPU_REGION( renderContext, "Downsample" )
		for( int i = 0; i < depth; ++i )
		{
			std::string name = "Downsample Step " + std::to_string( i );
			GPU_REGION( renderContext, name.c_str() );
			auto rt = downsampleTexture[i];
			auto effect = i == 0 && bloom->m_luminanceThreshold > -1.0f ? m_downSamplerLuminancePreserve : m_downSampler;
			auto invTexelSize = Vector2( 1.0f / (float)lastRt.GetRenderTarget()->GetWidth(), 1.0f / (float)lastRt.GetRenderTarget()->GetHeight() );

			effect->SetParameter( BlueSharedString( "BlitCurrent" ), lastRt );

			auto downsampleInfo = DownsampleData
			{
				invTexelSize
			};
			FillAndSetConstants( m_bloomConstantBuffer, &downsampleInfo, sizeof( downsampleInfo ), Tr2RenderContextEnum::PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
				
			DrawInto( *rt, Tr2LoadAction::DONT_CARE, effect, renderContext );

			lastRt = rt;
		}
	}
	
	{
		// do a two pass downsampling with gaussian blur on the downsampled texture
		// then the last upsampled texture will be added on top

		GPU_REGION( renderContext, "Upsample" );

		float tintScale = ( 1.0f / Bloom::MAX_BLOOM_STEPS ) * bloom->m_bloomBrightness;

		Vector2 directionalWeight = Vector2( std::max( bloom->m_directionalWeight, 0.0f ), std::fabsf(bloom->m_directionalWeight) );
		for( int i = depth - 1; i >= 0; --i )
		{
			lastRt = i == depth - 1 ? black : lastRt;

			auto currentMip = downsampleTexture[i];
			auto currentUpsampled = upsampleTexture[i];
			
			if( m_bloomDebugMode != BloomDebugMode::BLOOM_DEBUG_NONE )
			{
				currentUpsampled = upsampleHorizontalTexture[i];
			}

			float radiusInPixels = std::max( (float)currentMip->GetWidth(), (float)currentMip->GetHeight() ) * bloom->m_sizeScale * bloom->m_stepSizes[i] * 0.01f;

			auto invTexelSize = Vector2( 1.0f / (float)currentMip->GetWidth(), 1.0f / (float)currentMip->GetHeight() );
			std::string name = "Horizontal Step " + std::to_string( i );
			GPU_REGION( renderContext, name.c_str() );

			m_upsamplerHorizontal->SetParameter( BlueSharedString( "BlitCurrent" ), currentMip );
			auto gaussianOutput = GaussianDistribution::CalculateGaussianPassParameters( radiusInPixels, directionalWeight.x, invTexelSize.x, Vector3( 1, 1, 1 ), Vector2( 1, 0 ) );
			FillAndSetConstants( m_bloomConstantBuffer, &gaussianOutput, sizeof( gaussianOutput ), Tr2RenderContextEnum::PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
			DrawInto( *currentUpsampled, Tr2LoadAction::DONT_CARE, m_upsamplerHorizontal, renderContext );
		}
		for( int i = depth - 1; i >= 0; --i )
		{
			lastRt = i == depth - 1 ? black : lastRt;

			auto currentMip = downsampleTexture[i];
			auto currentUpsampled = upsampleTexture[i];

			if( m_bloomDebugMode != BloomDebugMode::BLOOM_DEBUG_NONE )
			{
				currentUpsampled = upsampleHorizontalTexture[i];
				currentMip = upsampleTexture[i];
			}

			float radiusInPixels = std::max( (float)currentMip->GetWidth(), (float)currentMip->GetHeight() ) * bloom->m_sizeScale * bloom->m_stepSizes[i] * 0.01f;

			auto invTexelSize = Vector2( 1.0f / (float)currentMip->GetWidth(), 1.0f / (float)currentMip->GetHeight() );
			
			std::string name = "Vertical Step " + std::to_string( i );
			GPU_REGION( renderContext, name.c_str() );

			// current upsampled is the horizontally blurred mip
			m_upsamplerVertical->SetParameter( BlueSharedString( "BlitCurrent" ), currentUpsampled );
			m_upsamplerVertical->SetParameter( BlueSharedString( "LastMip" ), lastRt );

			Vector4 tint = bloom->m_stepTints[i] * tintScale;
			auto gaussianOutput = GaussianDistribution::CalculateGaussianPassParameters( radiusInPixels, directionalWeight.y, invTexelSize.y, tint.GetXYZ(), Vector2( 0, 1 ) );
			FillAndSetConstants( m_bloomConstantBuffer, &gaussianOutput, sizeof( gaussianOutput ), Tr2RenderContextEnum::PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
			// draw into the downsample texture, because they will not be used again
			DrawInto( *currentMip, Tr2LoadAction::DONT_CARE, m_upsamplerVertical, renderContext );
			
			lastRt = currentMip;
		}
	}

	if( m_bloomDebugMode != BloomDebugMode::BLOOM_DEBUG_NONE )
	{
		return RenderBloomDebug( downsampleTexture, upsampleTexture, dest, renderContext );
	}
	else
	{
		m_bloomDebugShader = nullptr;
	}
	
	return lastRt;
}

Tr2PostProcessRenderInfo::Texture TriStepRenderPostProcess::RenderBloomDebug( std::array<Tr2PostProcessRenderInfo::Texture, Bloom::MAX_BLOOM_STEPS>& downsample, 
std::array<Tr2PostProcessRenderInfo::Texture, Bloom::MAX_BLOOM_STEPS>& upsample,
																			  Tr2PostProcessRenderInfo::Texture& blitCurrent,
																			  Tr2RenderContext& renderContext )
{	
	if( m_bloomDebugShader == nullptr )
	{
		m_bloomDebugShader.CreateInstance();
		m_bloomDebugShader->StartUpdate();
		m_bloomDebugShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/BloomDebug.fx" );
		m_bloomDebugShader->EndUpdate();
	}

	m_bloomDebugShader->StartUpdate();
	switch( m_bloomDebugMode )
	{
	case BloomDebugMode::BLOOM_DEBUG_ALL:
		m_bloomDebugShader->SetOption( BlueSharedString( "BLOOM_DEBUG_MODE" ), BlueSharedString( "BLOOM_DEBUG_MODE_ALL" ) );
		break;
	case BloomDebugMode::BLOOM_DEBUG_STEP1:
		m_bloomDebugShader->SetOption( BlueSharedString( "BLOOM_DEBUG_MODE" ), BlueSharedString( "BLOOM_DEBUG_MODE_1" ) );
		break;
	case BloomDebugMode::BLOOM_DEBUG_STEP2:
		m_bloomDebugShader->SetOption( BlueSharedString( "BLOOM_DEBUG_MODE" ), BlueSharedString( "BLOOM_DEBUG_MODE_2" ) );
		break;
	case BloomDebugMode::BLOOM_DEBUG_STEP3:
		m_bloomDebugShader->SetOption( BlueSharedString( "BLOOM_DEBUG_MODE" ), BlueSharedString( "BLOOM_DEBUG_MODE_3" ) );
		break;
	case BloomDebugMode::BLOOM_DEBUG_STEP4:
		m_bloomDebugShader->SetOption( BlueSharedString( "BLOOM_DEBUG_MODE" ), BlueSharedString( "BLOOM_DEBUG_MODE_4" ) );
		break;
	case BloomDebugMode::BLOOM_DEBUG_STEP5:
		m_bloomDebugShader->SetOption( BlueSharedString( "BLOOM_DEBUG_MODE" ), BlueSharedString( "BLOOM_DEBUG_MODE_5" ) );
		break;
	case BloomDebugMode::BLOOM_DEBUG_STEP6:
		m_bloomDebugShader->SetOption( BlueSharedString( "BLOOM_DEBUG_MODE" ), BlueSharedString( "BLOOM_DEBUG_MODE_6" ) );
		break;
	default:
		break;
	}
	
	m_bloomDebugShader->EndUpdate();
	
	GPU_REGION( renderContext, "Debug" );
	m_bloomDebugShader->SetParameter( BlueSharedString( "BlitCurrent" ), blitCurrent );
	for( int i = 0; i < Bloom::MAX_BLOOM_STEPS; ++i )
	{
		m_bloomDebugShader->SetParameter( BlueSharedString( "Downsample" + std::to_string( i + 1 ) ), downsample[i] );
		m_bloomDebugShader->SetParameter( BlueSharedString( "Upsample" + std::to_string( i + 1 ) ), upsample[i] );
	}

	auto debugView = m_renderInfo->GetTempTexture();

	DrawInto( *debugView, Tr2LoadAction::DONT_CARE, m_bloomDebugShader, renderContext );

	return debugView;
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
	auto rt1 = m_renderInfo->GetTempTexture( "Down-sampled Depth", 0.5f );
	DownSampleDepth( renderContext, rt1 );

	// God rays
	auto rt2 = m_renderInfo->GetTempTexture( "God rays", 0.5f );
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


bool TriStepRenderPostProcess::ProcessDynamicExposure( Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure, Tr2PPBloomEffect* bloom, Tr2PostProcess2* postProcess )
{
    if( !postProcess )
    {
        return false;
    }
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
	float exposureAdjustment = 0;
	if( postProcess )
	{
		exposureAdjustment = postProcess->m_exposureAdjustment;
	}
	if( dynamicExposure )
	{
		exposureAdjustment += dynamicExposure->m_adjustment;
	}
	m_tonemappingEffect->SetParameter( BlueSharedString( "ExposureAdjust" ), pow( 2.0f, exposureAdjustment ) );

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

void TriStepRenderPostProcess::RenderDynamicExposureDebug( Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure )
{
	if( dynamicExposure && dynamicExposure->IsActive() && dynamicExposure->m_debug )
	{
		Tr2Rect rect;
		const TriViewport& vp = renderContext.m_esm.GetViewport();
		rect.left = vp.x;
		rect.top = vp.y + int32_t( 0.7f * vp.height );
		rect.right = vp.x + vp.width;
		rect.bottom = vp.y + vp.height;

		if( !m_dynamicExposureDebugShader )
		{
			m_dynamicExposureDebugShader.CreateInstance();
			m_dynamicExposureDebugShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/ExposureDebug.fx" );
		}
		m_dynamicExposureDebugShader->SetParameter( BlueSharedString( "Exposure" ), m_exposure );
		m_dynamicExposureDebugShader->SetParameter( BlueSharedString( "Histogram" ), m_histogram );
		m_dynamicExposureDebugShader->SetParameter( BlueSharedString( "MinLuminance" ), log( dynamicExposure->m_minLuminance ) );
		m_dynamicExposureDebugShader->SetParameter( BlueSharedString( "MaxLuminance" ), log( dynamicExposure->m_maxLuminance ) );
		m_dynamicExposureDebugShader->SetParameter( BlueSharedString( "RectSize" ), Vector2( float( rect.left - rect.right ), float( rect.bottom - rect.top ) ) );

		Tr2TextureAL noTexture;
		Tr2Renderer::DrawTexture( renderContext, m_dynamicExposureDebugShader, noTexture, Vector2( 0, 0 ), Vector2( 1, 1 ), Vector2( 0, 0.7f ), Vector2( 1, 1 ) );

		const float* exposureData = nullptr;
		if( SUCCEEDED( m_exposure->GetGpuBuffer( 0 )->MapForReading( exposureData, renderContext ) ) )
		{
			Tr2Renderer::PrintfImmediate( renderContext, TRI_DBG_FONT_SMALL, rect, TRI_DFS_CENTER, 0xff00ff00, "Middle gray: %.2f", exposureData[0] );
			Tr2Renderer::PrintfImmediate( renderContext, TRI_DBG_FONT_SMALL, rect, TRI_DFS_LEFT, 0xff00ff00, "Min: %.2f", exposureData[5] );
			Tr2Renderer::PrintfImmediate( renderContext, TRI_DBG_FONT_SMALL, rect, TRI_DFS_RIGHT, 0xff00ff00, "Max: %.2f", exposureData[6] );

			m_exposure->GetGpuBuffer( 0 )->UnmapForReading( renderContext );
		}
	}
	else
	{
		m_dynamicExposureDebugShader = nullptr;
	}
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
		dispatchParameters.exposure = m_exposureTexture->GetTexture();
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

	if( dispatchRequirements & Tr2UpscalingAL::DispatchRequirements::TRANSPARENCY )
	{
		GPU_REGION( renderContext, "Transparency" );
		if( !m_transparencyMask )
		{
			m_transparencyMask.CreateInstance();
			m_transparencyMask->SetName( "TransparencyMask" );
			m_transparencyMask->Create( source->GetWidth(), source->GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM, source->GetMsaaType(), 0 );
		}

		m_transparencyMaskEffect->SetParameter( BlueSharedString( "OpaqueOnly" ), m_opaqueColorBuffer );
		m_transparencyMaskEffect->SetParameter( BlueSharedString( "OpaqueAndTransparency" ), source );
		DrawInto( *m_transparencyMask, Tr2LoadAction::DONT_CARE, m_transparencyMaskEffect, renderContext );
		dispatchParameters.transparency = m_transparencyMask ? m_transparencyMask->GetTexture() : nullptr;
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
	dispatchParameters.currentFrameIndex = Tr2Renderer::GetCurrentFrameCounter();
	dispatchParameters.reset = m_sceneDirty;

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

	dispatchParameters.upscalingDebugView = g_upscalingDebugView;
	dispatchParameters.frameGenDebugView = g_frameGenDebugView;

	m_lastFrameTime = BeOS->GetCurrentFrameTime();
	{
		GPU_REGION( renderContext, "UpscalingTechnique" );
		auto result = upscalingContext->Dispatch( dispatchParameters );
		if( result != Tr2UpscalingAL::OK && s_lastUpscalingResult != result )
		{
			Tr2UpscalingAL::LogResult(result);
		}
		s_lastUpscalingResult = result;
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

void TriStepRenderPostProcess::ProcessLut( std::vector<const Tr2PPLutEffect*> luts )
{
	bool tomeppingEffectUpdating = false;
	uint8_t lutsActive = 0;
	for( const auto& lut : luts )
	{
		// we only need to update the tonemapping buffer
		std::string influenceParam = std::string( "LUTInfluence_" ) + std::to_string( lutsActive );
		std::string textureParam = std::string( "TexLUT_" ) + std::to_string( lutsActive );

		if( !tomeppingEffectUpdating )
		{
			m_tonemappingEffect->StartUpdate();
			tomeppingEffectUpdating = true;
		}

		m_tonemappingEffect->SetParameter( BlueSharedString( influenceParam ), lut->m_influence );
		auto resource = m_tonemappingEffect->GetResourceByName( textureParam.c_str() );
		if( !resource )
		{
			m_tonemappingEffect->AddResourceTexture2D( BlueSharedString( textureParam ), lut->m_path.c_str() );
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
		lutsActive++;
	}

	if( m_lutsEnabled > lutsActive )
	{
		// need to remove all luts that may have been removed
		if( !tomeppingEffectUpdating )
		{
			m_tonemappingEffect->StartUpdate();
			tomeppingEffectUpdating = true;
		}

		std::string influenceParam = std::string( "LUTInfluence_" ) + std::to_string( lutsActive );
		m_tonemappingEffect->SetParameter( BlueSharedString( influenceParam ), 0.0f );
	}

	if( tomeppingEffectUpdating )
	{
		m_tonemappingEffect->EndUpdate();
	}
	
	if( lutsActive == 0 && m_lutsEnabled > 0 )
	{
		m_tonemappingEffect->StartUpdate();
		m_tonemappingEffect->SetOption( BlueSharedString( "LUT_TOGGLE" ), BlueSharedString( "LUT_DISABLED" ) );
		m_tonemappingEffect->EndUpdate();
	}

	m_lutsEnabled = lutsActive;
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
	auto rt1 = m_renderInfo->GetTempTexture( "Fog Color", 0.5f );
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


			BlueSharedString quality_option;
			switch(taa->m_quality)
			{
				case Tr2PPTaaEffect::Quality::TAA_LOW:
					m_taaEffect->SetOption( BlueSharedString( "QUALITY" ), BlueSharedString( "QUALITY_LOW" ) );
					break;
				case Tr2PPTaaEffect::Quality::TAA_MEDIUM:
					m_taaEffect->SetOption( BlueSharedString( "QUALITY" ), BlueSharedString( "QUALITY_MEDIUM" ) );
					break;
				case Tr2PPTaaEffect::Quality::TAA_HIGH:
					m_taaEffect->SetOption( BlueSharedString( "QUALITY" ), BlueSharedString( "QUALITY_HIGH" ) );
					break;
			}


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

void TriStepRenderPostProcess::ProcessColorCorrection( Tr2PPColorCorrectionEffect* colorCorrection )
{
	if( !colorCorrection->IsDirty() )
	{
		return;
	}

	m_tonemappingEffect->StartUpdate();

	if( colorCorrection->IsActive() )
	{
		m_tonemappingEffect->SetOption( BlueSharedString( "COLOR_CORRECTION_TOGGLE" ), BlueSharedString( "COLOR_CORRECTION_ENABLED" ) );
		static auto WhiteTemperature = BlueSharedString( "WhiteTemperature" );
		m_tonemappingEffect->SetParameter( WhiteTemperature, colorCorrection->m_whiteTemperature );
		static auto WhiteTint = BlueSharedString( "WhiteTint" );
		m_tonemappingEffect->SetParameter( WhiteTint, colorCorrection->m_whiteTint );
		static auto ColorSaturation = BlueSharedString( "ColorSaturation" );
		m_tonemappingEffect->SetParameter( ColorSaturation, colorCorrection->m_colorSaturation );
		static auto ColorContrast = BlueSharedString( "ColorContrast" );
		m_tonemappingEffect->SetParameter( ColorContrast, colorCorrection->m_colorContrast );
		static auto ColorGamma = BlueSharedString( "ColorGamma" );
		m_tonemappingEffect->SetParameter( ColorGamma, colorCorrection->m_colorGamma );
		static auto ColorGain = BlueSharedString( "ColorGain" );
		m_tonemappingEffect->SetParameter( ColorGain, colorCorrection->m_colorGain );
		static auto ColorOffset = BlueSharedString( "ColorOffset" );
		m_tonemappingEffect->SetParameter( ColorOffset, colorCorrection->m_colorOffset );
	}
	else
	{
		m_tonemappingEffect->SetOption( BlueSharedString( "COLOR_CORRECTION_TOGGLE" ), BlueSharedString( "COLOR_CORRECTION_DISABLED" ) );
	}

	m_tonemappingEffect->EndUpdate();

	colorCorrection->SetDirty( false );
}

void TriStepRenderPostProcess::ProcessTonemapping( Tr2PPTonemappingEffect* tonemapping )
{
	if ( !tonemapping->IsDirty() )
	{
		return;
	}
	
	m_tonemappingEffect->StartUpdate();

	if ( tonemapping->IsActive() )
	{
		if( tonemapping->m_method == Tr2PPTonemappingEffect::Aces )
		{
			m_tonemappingEffect->SetOption( BlueSharedString( "TONE_MAPPING_METHOD" ), BlueSharedString( "TONE_MAPPING_ACES" ) );

			if( tonemapping->m_aces.m_useSweeteners )
			{
				m_tonemappingEffect->SetOption( BlueSharedString( "SWEETENER_TOGGLE" ), BlueSharedString( "SWEETENER_ENABLED" ) );
			}
			else
			{
				m_tonemappingEffect->SetOption( BlueSharedString( "SWEETENER_TOGGLE" ), BlueSharedString( "SWEETENER_DISABLED" ) );
			}

			m_tonemappingEffect->SetParameter( BlueSharedString( "AcesSlope" ), tonemapping->m_aces.m_slope );
			m_tonemappingEffect->SetParameter( BlueSharedString( "AcesToe" ), tonemapping->m_aces.m_toe );
			m_tonemappingEffect->SetParameter( BlueSharedString( "AcesShoulder" ), tonemapping->m_aces.m_shoulder );
			m_tonemappingEffect->SetParameter( BlueSharedString( "AcesBlackClip" ), tonemapping->m_aces.m_blackClip );
			m_tonemappingEffect->SetParameter( BlueSharedString( "AcesWhiteClip" ), tonemapping->m_aces.m_whiteClip );

			// --- taken from: https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl ---
			static const Matrix ACESInputMat = Transpose( Matrix(
				0.59719f, 0.35458f, 0.04823f, 0.f, 
				0.07600f, 0.90834f, 0.01566f, 0.f, 
				0.02840f, 0.13383f, 0.83777f, 0.f, 
				0.f, 0.f, 0.f, 1.f 
			) );

			// ODT_SAT => XYZ => D60_2_D65 => sRGB
			static const Matrix ACESOutputMat = Transpose( Matrix(
				1.60475, -0.53108, -0.07367, 0.f, 
				-0.10208, 1.10813, -0.00605, 0.f, 
				-0.00327, -0.07276, 1.07602, 0.f, 
				0.f, 0.f, 0.f, 1.f 
			) );
			// --------------------------------------------------------------------------------------------

			// --- taken from https://community.acescentral.com/t/colour-artefacts-or-breakup-using-aces/520/8 ---
			const Matrix BlueCorrect = Matrix(
				0.9404372683f, -0.0183068787f, 0.0778696104f, 0.f, 
				0.0083786969f, 0.8286599939f, 0.1629613092f, 0.f, 
				0.0005471261f, -0.0008833746f, 1.0003362486f, 0.f, 
				0.f, 0.f, 0.f, 1.f 
			);
			const Matrix BlueCorrectInv = Matrix(
				1.06318f, 0.0233956f, -0.0865726f, 0.f, 
				-0.0106337f, 1.20632f, -0.19569f, 0.f, 
				-0.000590887f, 0.00105248f, 0.999538f, 0.f, 
				0.f, 0.f, 0.f, 1.f 
			);
			// ---------------------------------------------------------------------------------------------------

			Matrix blueCorrection;
			{
				Vector3 row1 = Lerp( Vector3( 1.f, 0.f, 0.f ), BlueCorrect.GetX(), tonemapping->m_aces.m_blueCorrection );
				Vector3 row2 = Lerp( Vector3( 0.f, 1.f, 0.f ), BlueCorrect.GetY(), tonemapping->m_aces.m_blueCorrection );
				Vector3 row3 = Lerp( Vector3( 0.f, 0.f, 1.f ), BlueCorrect.GetZ(), tonemapping->m_aces.m_blueCorrection );
				blueCorrection = Transpose( Matrix(
					row1.x, row1.y, row1.z, 0., 
					row2.x, row2.y, row2.z, 0., 
					row3.x, row3.y, row3.z, 0., 
					0., 0., 0., 1. 
				) );
			}
			Matrix blueCorrectionInv;
			{
				Vector3 row1 = Lerp( Vector3( 1.f, 0.f, 0.f ), BlueCorrectInv.GetX(), tonemapping->m_aces.m_blueCorrection );
				Vector3 row2 = Lerp( Vector3( 0.f, 1.f, 0.f ), BlueCorrectInv.GetY(), tonemapping->m_aces.m_blueCorrection );
				Vector3 row3 = Lerp( Vector3( 0.f, 0.f, 1.f ), BlueCorrectInv.GetZ(), tonemapping->m_aces.m_blueCorrection );
				blueCorrectionInv = Transpose( Matrix(
					row1.x, row1.y, row1.z, 0., 
					row2.x, row2.y, row2.z, 0., 
					row3.x, row3.y, row3.z, 0., 
					0., 0., 0., 1. 
				) );
			}

			Matrix scale = ScalingMatrix( Vector3( tonemapping->m_aces.m_scale, tonemapping->m_aces.m_scale, tonemapping->m_aces.m_scale ) );

			Matrix input = Transpose( ACESInputMat * blueCorrection * scale );
			m_tonemappingEffect->SetParameter( BlueSharedString( "AcesInputMat" ), input );

			Matrix output = Transpose( blueCorrectionInv * ACESOutputMat );
			m_tonemappingEffect->SetParameter( BlueSharedString( "AcesOutputMat" ), output );
		}
		else
		{
			m_tonemappingEffect->SetOption( BlueSharedString( "TONE_MAPPING_METHOD" ), BlueSharedString( "TONE_MAPPING_UNCHARTED2" ) );

			m_tonemappingEffect->SetParameter( BlueSharedString( "ShoulderStrength" ), tonemapping->m_uncharted2.m_shoulderStrength );
			m_tonemappingEffect->SetParameter( BlueSharedString( "LinearStrength" ), tonemapping->m_uncharted2.m_linearStrength );
			m_tonemappingEffect->SetParameter( BlueSharedString( "LinearAngle" ), tonemapping->m_uncharted2.m_linearAngle );
			m_tonemappingEffect->SetParameter( BlueSharedString( "ToeStrength" ), tonemapping->m_uncharted2.m_toeStrength );
			m_tonemappingEffect->SetParameter( BlueSharedString( "ToeNumerator" ), tonemapping->m_uncharted2.m_toeNumerator );
			m_tonemappingEffect->SetParameter( BlueSharedString( "ToeDenominator" ), tonemapping->m_uncharted2.m_toeDenominator );
			m_tonemappingEffect->SetParameter( BlueSharedString( "WhiteScale" ), tonemapping->m_uncharted2.m_whiteScale );
		}
	}
	else
	{
		m_tonemappingEffect->SetOption( BlueSharedString( "TONE_MAPPING_METHOD" ), BlueSharedString( "TONE_MAPPING_DISABLED" ) );
	}

	m_tonemappingEffect->EndUpdate();

	tonemapping->SetDirty( false );
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
			Tr2PostProcessRenderInfo::Texture coc;

			if( !depthOfField->m_foregroundBlurNeeded )
			{
				GPU_REGION( renderContext, "CoC" );
				coc = m_renderInfo->GetTempTexture( "CoC", depthOfField->m_cocScale, Tr2RenderContextEnum::EX_NONE, Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM );
				DrawInto( *coc, Tr2LoadAction::DONT_CARE, m_depthOfFieldCoCShader, renderContext );
			}
			else
			{
				GPU_REGION( renderContext, "CoC" );
				auto coc2 = m_renderInfo->GetTempTexture( "CoC", depthOfField->m_cocScale, Tr2RenderContextEnum::EX_NONE, Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_UNORM );

				DrawInto( *coc2, Tr2LoadAction::DONT_CARE, m_depthOfFieldCoCShader, renderContext );
				{
					GPU_REGION( renderContext, "CoC Blur" );

					auto blurContext = PostProcessBlur::CreateBlurContext( PostProcessBlur::BT_Big,
																		   PostProcessBlur::BC_r,
																		   PostProcessBlur::BP_Maximum,
																		   PostProcessBlur::BF_MaxOfAllChannels,
																		   depthOfField->m_cocScale );
					coc = m_renderInfo->GetTempTexture( "CoC Blurred", depthOfField->m_cocScale, Tr2RenderContextEnum::EX_NONE, Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM );
					Blur( *coc, *coc2, renderContext, blurContext );
				}
			}

			float adjustedScale = depthOfField->m_scale / upscalingAmount;

			auto blur = m_renderInfo->GetTempTexture( "Bokeh Blend" );

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
		if( rt != nullptr && rt->IsValid() )
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
