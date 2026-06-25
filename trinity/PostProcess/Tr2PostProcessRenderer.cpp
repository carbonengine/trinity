// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2PostProcessRenderer.h"
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

int32_t g_dynamicExposureQualityRequirement = PostProcess::Quality::MEDIUM;
TRI_REGISTER_SETTING( "dynamicExposureQualityRequirement", g_dynamicExposureQualityRequirement );

#define MEMOIZED_STRING( x ) ( []( const char* str ) { static BlueSharedString s( str ); return s; }( x "" ) )

namespace
{
const uint32_t HISTOGRAM_TILE_SIZE_X = 16;
const uint32_t HISTOGRAM_TILE_SIZE_Y = 16;
const uint32_t NUM_TILES_PER_THREAD_GROUP = 256;
const uint32_t MAX_LUTS = 4;

static Tr2UpscalingAL::Result s_lastUpscalingResult;

void DrawInto( const Tr2TextureAL& dest, Tr2LoadAction::Type loadAction, const Tr2TextureAL& src, Tr2RenderContext& renderContext )
{
	renderContext.RenderPassHint( { loadAction, Tr2StoreAction::STORE }, {} );
	renderContext.m_esm.PushRenderTarget( dest );
	Tr2Renderer::DrawTexture( renderContext, src );
	renderContext.m_esm.PopRenderTarget();
}

void DrawPartiallyInto( const Tr2TextureAL& dest, Tr2LoadAction::Type loadAction, const Tr2TextureAL& src, Tr2RenderContext& renderContext, const Vector2 tlTextureCoords = Vector2( 0, 0 ), const Vector2 brTextureCoords = Vector2( 1, 1 ) )
{
	renderContext.RenderPassHint( { loadAction, Tr2StoreAction::STORE }, {} );
	renderContext.m_esm.PushRenderTarget( dest );
	Tr2Renderer::DrawTexture( renderContext, src, tlTextureCoords, brTextureCoords, Tr2Blitter::FILTER_LINEAR );
	renderContext.m_esm.PopRenderTarget();
}

void DrawInto( const Tr2TextureAL& dest, Tr2LoadAction::Type loadAction, Tr2Effect* effect, Tr2RenderContext& renderContext )
{
	renderContext.RenderPassHint( { loadAction, Tr2StoreAction::STORE }, {} );
	renderContext.m_esm.PushRenderTarget( dest );
	Tr2Renderer::DrawScreenQuad( renderContext, effect );
	renderContext.m_esm.PopRenderTarget();
}

ImageIO::PixelFormat GetUavCompatibleFormat( ImageIO::PixelFormat format )
{
	if( format == ImageIO::PIXEL_FORMAT_B8G8R8X8_UNORM || format == ImageIO::PIXEL_FORMAT_B8G8R8A8_UNORM )
	{
		// There must be a better way to detect and handle formats that are not UAV-compatible
		format = ImageIO::PIXEL_FORMAT_R8G8B8A8_UNORM;
	}
	return format;
}

BlueSharedString GetBloomDebugShaderOptionValue( Tr2PostProcessRenderer::BloomDebugMode mode )
{
	switch( mode )
	{
	case Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_ALL:
		return MEMOIZED_STRING( "BLOOM_DEBUG_MODE_ALL" );
	case Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_STEP1:
		return MEMOIZED_STRING( "BLOOM_DEBUG_MODE_1" );
	case Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_STEP2:
		return MEMOIZED_STRING( "BLOOM_DEBUG_MODE_2" );
	case Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_STEP3:
		return MEMOIZED_STRING( "BLOOM_DEBUG_MODE_3" );
	case Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_STEP4:
		return MEMOIZED_STRING( "BLOOM_DEBUG_MODE_4" );
	case Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_STEP5:
		return MEMOIZED_STRING( "BLOOM_DEBUG_MODE_5" );
	case Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_STEP6:
		return MEMOIZED_STRING( "BLOOM_DEBUG_MODE_6" );
	default:
		return MEMOIZED_STRING( "BLOOM_DEBUG_OFF" );
	}
}

BlueSharedString GetTaaQualityShaderOptionValue( Tr2PPTaaEffect::Quality quality )
{
	switch( quality )
	{
	case Tr2PPTaaEffect::Quality::TAA_LOW:
		return MEMOIZED_STRING( "QUALITY_LOW" );
	case Tr2PPTaaEffect::Quality::TAA_MEDIUM:
		return MEMOIZED_STRING( "QUALITY_MEDIUM" );
	case Tr2PPTaaEffect::Quality::TAA_HIGH:
		return MEMOIZED_STRING( "QUALITY_HIGH" );
	default:
		return MEMOIZED_STRING( "QUALITY_LOW" );
	}
}

BlueSharedString GetTaaDebugShaderOptionValue( Tr2PPTaaEffect::Debug debug )
{
	switch( debug )
	{
	case Tr2PPTaaEffect::Debug::TAA_DEBUG_MOTION_VECTORS:
		return MEMOIZED_STRING( "DEBUG_SHOW_MOTION_VECTORS" );
	case Tr2PPTaaEffect::Debug::TAA_DEBUG_EARLY_OUT_MASK:
		return MEMOIZED_STRING( "DEBUG_SHOW_EARLY_OUT_MASK" );
	default:
		return MEMOIZED_STRING( "DEBUG_NONE" );
	}
}

template <typename T>
struct TempParameterT
{
	TempParameterT( Tr2Effect* effect, const BlueSharedString& name, const T& newValue ) :
		m_effect( effect ), m_name( name )
	{
		m_effect->SetParameter( m_name, newValue );
	}
	~TempParameterT()
	{
		m_effect->SetParameter( m_name, T{} );
	}
	Tr2Effect* m_effect;
	BlueSharedString m_name;
};

template <typename T>
[[nodiscard]] TempParameterT<T> TempParameter( Tr2Effect* effect, const BlueSharedString& name, const T& newValue )
{
	return TempParameterT<T>( effect, name, newValue );
}

#define TEMP_PARAM( effect, name, value ) auto ANONYMOUS_VARIABLE( tempParameter ) = TempParameter( effect, MEMOIZED_STRING( name ), value )

const auto RENDER_TARGET = Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE;
}

namespace PostProcessBlur
{

BlurContext CreateBlurContext( BlurType type, BlurChannel channel, BlurProcess process, BlurFinalize finalize )
{
	BlurContext context;
	context.type = type;
	context.channel = channel;
	context.process = process;
	context.finalize = finalize;

	return context;
}



BlueSharedString GetBlurChannelOptionValue( BlurChannel channel )
{
	switch( channel )
	{
	case BlurChannel::BC_rgba:
		return MEMOIZED_STRING( "BLUR_CHANNEL_RGBA" );
	case BlurChannel::BC_r:
		return MEMOIZED_STRING( "BLUR_CHANNEL_R" );
	case BlurChannel::BC_g:
		return MEMOIZED_STRING( "BLUR_CHANNEL_G" );
	case BlurChannel::BC_b:
		return MEMOIZED_STRING( "BLUR_CHANNEL_B" );
	case BlurChannel::BC_a:
		return MEMOIZED_STRING( "BLUR_CHANNEL_A" );
	default:
		return MEMOIZED_STRING( "BLUR_CHANNEL_RGBA" );
	}
}

BlueSharedString GetBlurTypeOptionValue( BlurType type )
{
	if( type == BlurType::BT_Big )
	{
		return MEMOIZED_STRING( "BLUR_TYPE_BIG" );
	}
	return MEMOIZED_STRING( "BLUR_TYPE_SMALL" );
}

BlueSharedString GetProcessTypeOptionValue( BlurProcess process )
{
	if( process == BlurProcess::BP_Maximum )
	{
		return MEMOIZED_STRING( "BLUR_PROCESS_TYPE_MAXIMUM" );
	}
	if( process == BlurProcess::BP_Minimum )
	{
		return MEMOIZED_STRING( "BLUR_PROCESS_TYPE_MINIMUM" );
	}
	return MEMOIZED_STRING( "BLUR_PROCESS_TYPE_NONE" );
}

BlueSharedString GetFinalizeTypeOptionValue( BlurFinalize finalize )
{
	if( finalize == BlurFinalize::BF_MaxOfAllChannels )
	{
		return MEMOIZED_STRING( "BLUR_FINALIZE_TYPE_MAX_OF_ALL_CHANNELS" );
	}
	return MEMOIZED_STRING( "BLUR_FINALIZE_TYPE_NONE" );
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

	std::fill( std::begin( data.weightOffset ), std::end( data.weightOffset ), Vector4( 0, 0, 0, 0 ) );

	uint32_t index = 0;

	// Fill in the data
	for( uint32_t i = 0; i < taps.size(); i += 2 )
	{
		// pack 2 weight and offset into a vector4
		float weight1 = taps[i].first / weightSum;
		float offset1 = taps[i].second;
		float weight2 = taps[i + 1].first / weightSum;
		float offset2 = taps[i + 1].second;

		data.weightOffset[index++] = Vector4( weight1, offset1, weight2, offset2 );
	}

	return data;
}
}

namespace Tonemapping
{

void ApplyColorCorrection( const Tr2PPColorCorrectionEffect* colorCorrection, Tr2Effect* tonemappingEffect )
{
	if( colorCorrection != nullptr )
	{
		tonemappingEffect->SetOption( MEMOIZED_STRING( "COLOR_CORRECTION_TOGGLE" ), MEMOIZED_STRING( "COLOR_CORRECTION_ENABLED" ) );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "WhiteTemperature" ), colorCorrection->m_whiteTemperature );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "WhiteTint" ), colorCorrection->m_whiteTint );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "ColorSaturation" ), colorCorrection->m_colorSaturation );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "ColorContrast" ), colorCorrection->m_colorContrast );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "ColorGamma" ), colorCorrection->m_colorGamma );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "ColorGain" ), colorCorrection->m_colorGain );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "ColorOffset" ), colorCorrection->m_colorOffset );
	}
	else
	{
		tonemappingEffect->SetOption( MEMOIZED_STRING( "COLOR_CORRECTION_TOGGLE" ), MEMOIZED_STRING( "COLOR_CORRECTION_DISABLED" ) );
	}
}

void ApplyBloom( const Tr2PPBloomEffect* bloom, Tr2Effect* tonemappingEffect, bool newBloom, Tr2PostProcessRenderer::BloomDebugMode debugMode )
{
	if( bloom != nullptr )
	{
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "GrimeWeight" ), bloom->m_grimeWeight );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "BloomBrightness" ), newBloom ? 1.0f : bloom->m_bloomBrightness );
		tonemappingEffect->SetResourceTexture2D( MEMOIZED_STRING( "Grime" ), bloom->m_grimePath.c_str() );

		if( debugMode != Tr2PostProcessRenderer::BloomDebugMode::BLOOM_DEBUG_NONE )
		{
			// when rendering bloom debug we render it into the blit current,
			// so to not have tonemapping applied on top of it we need to set the blit original to the bloom result
			if( auto blitCurrent = tonemappingEffect->GetResourceByName( "BlitOriginal" ) )
			{
				if( TriTextureParameterPtr resource = BlueCastPtr( blitCurrent ) )
				{
					tonemappingEffect->SetParameter( MEMOIZED_STRING( "BlitOriginal" ), resource->GetResource() );
				}
			}
		}
	}
	else
	{
		tonemappingEffect->SetResourceTexture2D( MEMOIZED_STRING( "Grime" ), "res:/texture/global/black.dds" );
	}
}

void ApplyDynamicExposure( const Tr2PPDynamicExposureEffect* dynamicExposure, Tr2Effect* tonemappingEffect, float postprocessExposureAmount )
{
	if( dynamicExposure != nullptr )
	{
		tonemappingEffect->SetParameter( BlueSharedString( "ExposureMiddleValue" ), dynamicExposure->m_middleValue );
		tonemappingEffect->SetParameter( BlueSharedString( "ExposureInfluence" ), dynamicExposure->m_influence );
		tonemappingEffect->SetParameter( BlueSharedString( "MinExposure" ), dynamicExposure->m_minExposure );
		tonemappingEffect->SetParameter( BlueSharedString( "MaxExposure" ), dynamicExposure->m_maxExposure );
		tonemappingEffect->SetOption( BlueSharedString( "DYNAMIC_EXPOSURE_TOGGLE" ), BlueSharedString( "DYNAMIC_EXPOSURE_ENABLED" ) );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "ExposureAdjust" ), pow( 2.0f, postprocessExposureAmount + dynamicExposure->m_adjustment ) );
	}
	else
	{
		tonemappingEffect->SetOption( MEMOIZED_STRING( "DYNAMIC_EXPOSURE_TOGGLE" ), MEMOIZED_STRING( "DYNAMIC_EXPOSURE_DISABLED" ) );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "ExposureAdjust" ), pow( 2.0f, postprocessExposureAmount ) );
	}
}

void ApplyVignette( const Tr2PPVignetteEffect* vignette, Tr2Effect* tonemappingEffect )
{
	if( vignette != nullptr )
	{
		tonemappingEffect->SetResourceTexture2D( MEMOIZED_STRING( "VignetteShape" ), vignette->m_shapePath.c_str() );
		tonemappingEffect->SetResourceTexture2D( MEMOIZED_STRING( "VignetteDetail" ), vignette->m_detailPath.c_str() );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "VignetteDetailSize" ), Vector4( vignette->m_detail1Size[0], vignette->m_detail1Size[1], vignette->m_detail2Size[0], vignette->m_detail2Size[1] ) );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "VignetteDetailScroll" ), Vector4( vignette->m_detail1Scroll[0], vignette->m_detail1Scroll[1], vignette->m_detail2Scroll[0], vignette->m_detail2Scroll[1] ) );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "VignetteColor" ), Vector4( vignette->m_color ) );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "VignetteIntensity" ), Vector2( vignette->m_intensity, vignette->m_opacity ) );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "VignetteSineFrequency" ), vignette->m_sineFrequency );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "VignetteSineRange" ), Vector2( vignette->m_sineMinimum, vignette->m_sineMaximum ) );

		tonemappingEffect->SetOption( MEMOIZED_STRING( "VIGNETTE_TOGGLE" ), MEMOIZED_STRING( "VIGNETTE_ENABLED" ) );
	}
	else
	{
		tonemappingEffect->SetOption( MEMOIZED_STRING( "VIGNETTE_TOGGLE" ), MEMOIZED_STRING( "VIGNETTE_DISABLED" ) );
	}
}

void ApplyDesatureate( const Tr2PPDesaturateEffect* desaturate, Tr2Effect* tonemappingEffect )
{
	if( desaturate != nullptr )
	{
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "SaturationFactor" ), desaturate->m_intensity );
		tonemappingEffect->SetOption( MEMOIZED_STRING( "DESATURATE_TOGGLE" ), MEMOIZED_STRING( "DESATURATE_ENABLED" ) );
	}
	else
	{
		tonemappingEffect->SetOption( MEMOIZED_STRING( "DESATURATE_TOGGLE" ), MEMOIZED_STRING( "DESATURATE_DISABLED" ) );
	}
}

void ApplyFade( const Tr2PPFadeEffect* fade, Tr2Effect* tonemappingEffect )
{
	if( fade != nullptr )
	{
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "FadeColor" ), Vector4( fade->m_color ) );
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "FadeAmount" ), fade->m_intensity );
	}
	else
	{
		tonemappingEffect->SetParameter( MEMOIZED_STRING( "FadeAmount" ), 0.0f );
	}
}

void ApplyLuts( std::vector<const Tr2PPLutEffect*>& luts, Tr2Effect* tonemappingEffect )
{
	if( luts.size() > 0 )
	{
		tonemappingEffect->SetOption( MEMOIZED_STRING( "LUT_TOGGLE" ), MEMOIZED_STRING( "LUT_ENABLED" ) );

		for( uint32_t i = 0; i < MAX_LUTS; i++ )
		{
			tonemappingEffect->SetParameter( BlueSharedString( "LUTInfluence_" + std::to_string( i ) ), i < luts.size() ? luts[i]->m_influence : 0.0f );
			tonemappingEffect->SetResourceTexture2D( BlueSharedString( "TexLUT_" + std::to_string( i ) ), i < luts.size() ? luts[i]->m_path.c_str() : "" );
		}
	}
	else
	{
		tonemappingEffect->SetOption( BlueSharedString( "LUT_TOGGLE" ), BlueSharedString( "LUT_DISABLED" ) );
	}
}

void ApplyAcesTonemappingMethod( const Tr2PPTonemappingEffect* tonemapping, Tr2Effect* tonemappingEffect )
{
	tonemappingEffect->SetOption( MEMOIZED_STRING( "TONE_MAPPING_METHOD" ), MEMOIZED_STRING( "TONE_MAPPING_ACES" ) );

	static BlueSharedString blueOption = tonemapping->m_aces.m_useSweeteners ? MEMOIZED_STRING( "SWEETENER_ENABLED" ) : MEMOIZED_STRING( "SWEETENER_DISABLED" );
	tonemappingEffect->SetOption( MEMOIZED_STRING( "SWEETENER_TOGGLE" ), blueOption );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "AcesSlope" ), tonemapping->m_aces.m_slope );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "AcesToe" ), tonemapping->m_aces.m_toe );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "AcesShoulder" ), tonemapping->m_aces.m_shoulder );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "AcesBlackClip" ), tonemapping->m_aces.m_blackClip );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "AcesWhiteClip" ), tonemapping->m_aces.m_whiteClip );

	// --- taken from: https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl ---
	static const Matrix ACESInputMat = Transpose( Matrix(
		0.59719f, 0.35458f, 0.04823f, 0.f, 0.07600f, 0.90834f, 0.01566f, 0.f, 0.02840f, 0.13383f, 0.83777f, 0.f, 0.f, 0.f, 0.f, 1.f ) );

	// ODT_SAT => XYZ => D60_2_D65 => sRGB
	static const Matrix ACESOutputMat = Transpose( Matrix(
		1.60475, -0.53108, -0.07367, 0.f, -0.10208, 1.10813, -0.00605, 0.f, -0.00327, -0.07276, 1.07602, 0.f, 0.f, 0.f, 0.f, 1.f ) );
	// --------------------------------------------------------------------------------------------

	// --- taken from https://community.acescentral.com/t/colour-artefacts-or-breakup-using-aces/520/8 ---
	const Matrix BlueCorrect = Matrix(
		0.9404372683f, -0.0183068787f, 0.0778696104f, 0.f, 0.0083786969f, 0.8286599939f, 0.1629613092f, 0.f, 0.0005471261f, -0.0008833746f, 1.0003362486f, 0.f, 0.f, 0.f, 0.f, 1.f );
	const Matrix BlueCorrectInv = Matrix(
		1.06318f, 0.0233956f, -0.0865726f, 0.f, -0.0106337f, 1.20632f, -0.19569f, 0.f, -0.000590887f, 0.00105248f, 0.999538f, 0.f, 0.f, 0.f, 0.f, 1.f );
	// ---------------------------------------------------------------------------------------------------

	Matrix blueCorrection;
	{
		Vector3 row1 = Lerp( Vector3( 1.f, 0.f, 0.f ), BlueCorrect.GetX(), tonemapping->m_aces.m_blueCorrection );
		Vector3 row2 = Lerp( Vector3( 0.f, 1.f, 0.f ), BlueCorrect.GetY(), tonemapping->m_aces.m_blueCorrection );
		Vector3 row3 = Lerp( Vector3( 0.f, 0.f, 1.f ), BlueCorrect.GetZ(), tonemapping->m_aces.m_blueCorrection );
		blueCorrection = Transpose( Matrix(
			row1.x, row1.y, row1.z, 0., row2.x, row2.y, row2.z, 0., row3.x, row3.y, row3.z, 0., 0., 0., 0., 1. ) );
	}
	Matrix blueCorrectionInv;
	{
		Vector3 row1 = Lerp( Vector3( 1.f, 0.f, 0.f ), BlueCorrectInv.GetX(), tonemapping->m_aces.m_blueCorrection );
		Vector3 row2 = Lerp( Vector3( 0.f, 1.f, 0.f ), BlueCorrectInv.GetY(), tonemapping->m_aces.m_blueCorrection );
		Vector3 row3 = Lerp( Vector3( 0.f, 0.f, 1.f ), BlueCorrectInv.GetZ(), tonemapping->m_aces.m_blueCorrection );
		blueCorrectionInv = Transpose( Matrix(
			row1.x, row1.y, row1.z, 0., row2.x, row2.y, row2.z, 0., row3.x, row3.y, row3.z, 0., 0., 0., 0., 1. ) );
	}

	Matrix scale = ScalingMatrix( Vector3( tonemapping->m_aces.m_scale, tonemapping->m_aces.m_scale, tonemapping->m_aces.m_scale ) );

	Matrix input = Transpose( ACESInputMat * blueCorrection * scale );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "AcesInputMat" ), input );

	Matrix output = Transpose( blueCorrectionInv * ACESOutputMat );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "AcesOutputMat" ), output );
}

void ApplyAgxTonemappingMethod( Tr2Effect* tonemappingEffect )
{
	tonemappingEffect->SetOption( MEMOIZED_STRING( "TONE_MAPPING_METHOD" ), MEMOIZED_STRING( "TONE_MAPPING_AGX" ) );
}

void ApplyUncharted2TonemappingMethod( const Tr2PPTonemappingEffect* tonemapping, Tr2Effect* tonemappingEffect )
{
	tonemappingEffect->SetOption( MEMOIZED_STRING( "TONE_MAPPING_METHOD" ), MEMOIZED_STRING( "TONE_MAPPING_UNCHARTED2" ) );

	tonemappingEffect->SetParameter( MEMOIZED_STRING( "ShoulderStrength" ), tonemapping->m_uncharted2.m_shoulderStrength );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "LinearStrength" ), tonemapping->m_uncharted2.m_linearStrength );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "LinearAngle" ), tonemapping->m_uncharted2.m_linearAngle );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "ToeStrength" ), tonemapping->m_uncharted2.m_toeStrength );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "ToeNumerator" ), tonemapping->m_uncharted2.m_toeNumerator );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "ToeDenominator" ), tonemapping->m_uncharted2.m_toeDenominator );
	tonemappingEffect->SetParameter( MEMOIZED_STRING( "WhiteScale" ), tonemapping->m_uncharted2.m_whiteScale );
}

void ApplyNoTonemappingMethod( Tr2Effect* tonemappingEffect )
{
	tonemappingEffect->SetOption( MEMOIZED_STRING( "TONE_MAPPING_METHOD" ), MEMOIZED_STRING( "TONE_MAPPING_DISABLED" ) );
}
}

namespace AMDSharpening
{
Vector4 AsVector( uintfloat4 v )
{
	return Vector4( v.f[0], v.f[1], v.f[2], v.f[3] );
}
}

Tr2PostProcessRenderer::Tr2PostProcessRenderer( IRoot* lockobj ) :
	m_quality( PostProcess::HIGH ),
	m_lutsEnabled( 0 ),
	m_vignetteEnabled( false ),
	m_lastFrameTime( std::numeric_limits<long long>().max() ),
	m_bloomDebugMode( BloomDebugMode::BLOOM_DEBUG_NONE ),
	m_useNewBloom( g_newBloom ),
	m_bokehFrameCounter( 0 ),
	m_taaFrameCounter( 0 )
{
	// tonemapping shader
	m_tonemappingEffect.CreateInstance();
	m_tonemappingEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/ToneMapping.fx" );

	// upscaling shaders
	m_reactiveMaskEffect.CreateInstance();
	m_reactiveMaskEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/ReactiveMask.fx" );

	m_transparencyMaskEffect.CreateInstance();
	m_transparencyMaskEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/TransparencyMask.fx" );

	// bloom shaders
	m_bloomHighPassFilter.CreateInstance();
	m_bloomHighPassFilter->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/HighPassFilter.fx" );

	m_downSamplerLuminancePreserve.CreateInstance();
	m_downSamplerLuminancePreserve->StartUpdate();
	m_downSamplerLuminancePreserve->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Downsample.fx" );
	m_downSamplerLuminancePreserve->SetOption( MEMOIZED_STRING( "LUNINANCE_PRESERVE" ), MEMOIZED_STRING( "LUNINANCE_PRESERVE_ON" ) );
	m_downSamplerLuminancePreserve->EndUpdate();

	m_downSampler.CreateInstance();
	m_downSampler->StartUpdate();
	m_downSampler->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Downsample.fx" );
	m_downSampler->SetOption( MEMOIZED_STRING( "LUNINANCE_PRESERVE" ), MEMOIZED_STRING( "LUNINANCE_PRESERVE_OFF" ) );
	m_downSampler->EndUpdate();

	m_upsamplerHorizontal.CreateInstance();
	m_upsamplerHorizontal->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Upsample.fx" );

	m_upsamplerVertical.CreateInstance();
	m_upsamplerVertical->StartUpdate();
	m_upsamplerVertical->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Upsample.fx" );
	m_upsamplerVertical->SetOption( MEMOIZED_STRING( "UPSAMPLING_STEP" ), MEMOIZED_STRING( "UPSAMPLING_STEP_SECOND" ) );
	m_upsamplerVertical->EndUpdate();

	m_bloomConstantBuffer = Tr2ConstantBufferAL();

	// dynamic exposure shaders
	m_dynamicExposureToTextureShader.CreateInstance();
	m_dynamicExposureToTextureShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/ExposureToTexture.fx" );

	m_dynamicExposureCreateHistogramShader.CreateInstance();
	m_dynamicExposureCreateHistogramShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/CreateHistograms.fx" );

	m_dynamicExposureMergeHistogramShader.CreateInstance();
	m_dynamicExposureMergeHistogramShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/MergeHistograms.fx" );

	m_dynamicExposureMeasureExposureShader.CreateInstance();
	m_dynamicExposureMeasureExposureShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/MeasureExposure.fx" );

	// AMD CAS shader
	m_fidelityFxCasShader.CreateInstance();
	m_fidelityFxCasShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/CAS.fx" );

	// downsample depth shader
	m_downsampleDepthEffect.CreateInstance();
	m_downsampleDepthEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/DownsampleDepth.fx" );

	// fog shaders
	m_fogColorEffect.CreateInstance();
	m_fogColorEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/EnvironmentFogColor.fx" );

	m_fogCompositeEffect.CreateInstance();
	m_fogCompositeEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/EnvironmentFogComposit.fx" );

	// depth of field shaders
	m_depthOfFieldBokehBlurShader.CreateInstance();
	m_depthOfFieldBokehBlurShader->StartUpdate();
	m_depthOfFieldBokehBlurShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Bokeh.fx" );
	m_depthOfFieldBokehBlurShader->SetOption( MEMOIZED_STRING( "BOKEH_PIXEL_METHOD" ), MEMOIZED_STRING( "BOKEH_PIXEL_AVERAGE" ) );
	m_depthOfFieldBokehBlurShader->EndUpdate();

	m_depthOfFieldBokehFillShader.CreateInstance();
	m_depthOfFieldBokehFillShader->StartUpdate();
	m_depthOfFieldBokehFillShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Bokeh.fx" );
	m_depthOfFieldBokehFillShader->SetOption( MEMOIZED_STRING( "BOKEH_PIXEL_METHOD" ), MEMOIZED_STRING( "BOKEH_PIXEL_MAX" ) );
	m_depthOfFieldBokehFillShader->EndUpdate();

	m_depthOfFieldBokehTAAShader.CreateInstance();
	m_depthOfFieldBokehTAAShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/BokehTAA.fx" );

	m_depthOfFieldCoCShader.CreateInstance();
	m_depthOfFieldCoCShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/CircleOfConfusion.fx" );

	// god ray shader
	m_godrayEffect.CreateInstance();
	m_godrayEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Godrays.fx" );

	// signal loss shader
	m_signalLossEffect.CreateInstance();
	m_signalLossEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/SignalLoss.fx" );

	// film grain shader
	m_grainShader.CreateInstance();
	m_grainShader->StartUpdate();
	m_grainShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/FilmGrain.fx" );
	m_grainShader->AddResourceTexture2D( MEMOIZED_STRING( "NoiseTexture" ), "res:/texture/global/film_grain_noise.png" );
	m_grainShader->EndUpdate();

	// taa shaders
	m_taaEffect.CreateInstance();
	m_taaEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/TAA.fx" );

	m_taaCopyEffect.CreateInstance();
	m_taaCopyEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/TAACopy.fx" );
}

PostProcess::Quality Tr2PostProcessRenderer::GetPostProcessingQuality() const
{
	return m_quality;
}

void Tr2PostProcessRenderer::SetPostProcessingQuality( PostProcess::Quality quality )
{
	m_quality = quality;
}


void Tr2PostProcessRenderer::Execute(
	const Tr2TextureAL& destination,
	Tr2GpuResourcePool::Texture sourceBuffer,
	Tr2GpuResourcePool::Texture depthMap,
	Tr2GpuResourcePool::Texture velocity,
	Tr2GpuResourcePool::Texture opaqueColor,
	EveSpaceScene* scene,
	Tr2UpscalingContextAL* upscalingContext,
	Tr2GpuResourcePool& gpuResourcePool,
	Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !sourceBuffer.IsValid() )
	{
		CCP_LOGERR( "Tr2PostProcessRenderer::Execute: Source buffer is invalid!" );
		return;
	}
	const auto renderSize = TextureSize2D{ sourceBuffer->GetWidth(), sourceBuffer->GetHeight() };
	auto displaySize = renderSize;

	GPU_REGION( renderContext, "Post-processing" );

	Tr2PostProcess2Ptr postProcess = scene->GetPostProcess();

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	renderContext.m_esm.PushRenderTarget();
	renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );

	const auto upscalingInfo = renderContext.GetPrimaryRenderContext().GetUpscalingInfo( upscalingContext ? upscalingContext->GetID() : Tr2UpscalingAL::INVALID_CONTEXT_ID );

	auto upscalingEnabled = upscalingInfo.technique != Tr2UpscalingAL::NONE;
	bool sharpeningRequired = !upscalingInfo.hasSharpening;

	Tr2GpuResourcePool::Texture output;
	if( upscalingEnabled )
	{
		displaySize = { upscalingInfo.displayWidth, upscalingInfo.displayHeight };

		output = gpuResourcePool.GetTempTexture( "Final Result", displaySize, destination.GetFormat(), RENDER_TARGET );
	}
	else
	{
		output = gpuResourcePool.GetTempTexture( "Final Result", displaySize, destination.GetFormat(), RENDER_TARGET );
	}

	// Always copy
	auto nonMsaaSource = gpuResourcePool.GetTempTexture( "Pre-upscaling Composite", renderSize, sourceBuffer->GetFormat(), RENDER_TARGET );
	sourceBuffer->Resolve( nonMsaaSource, renderContext );

	Tr2PPDynamicExposureEffect* dynamicExposure = nullptr;
	Tr2GpuResourcePool::Buffer histogramBuffer;

	if( postProcess != nullptr )
	{
		if( auto genericEffect = postProcess->GetGenericEffectIfAvailable( m_quality ) )
		{
			RenderGenericEffect( nonMsaaSource, sourceBuffer, renderContext, genericEffect );
		}

		if( auto fog = postProcess->GetFogIfAvailable( m_quality ) )
		{
			RenderFog( nonMsaaSource, sourceBuffer, gpuResourcePool, renderContext, fog );
		}
		sourceBuffer = {};

		if( auto godrays = postProcess->GetGodRaysIfAvailable( m_quality ) )
		{
			RenderGodRays( nonMsaaSource, depthMap, gpuResourcePool, renderContext, godrays );
		}

		if( auto dof = postProcess->GetDepthOfFieldIfAvailable( m_quality ) )
		{
			bool temporal = upscalingInfo.temporal || postProcess->GetTaaIfAvailable( m_quality ) != nullptr;
			RenderDepthOfField( nonMsaaSource, gpuResourcePool, renderContext, dof, temporal, upscalingInfo.upscalingAmount );
		}

		dynamicExposure = postProcess->GetDynamicExposureIfAvailable( m_quality );
		auto taa = postProcess->GetTaaIfAvailable( m_quality );

		if( taa != nullptr && !upscalingInfo.temporal )
		{
			RenderTaa( nonMsaaSource, velocity, opaqueColor, gpuResourcePool, renderContext, taa, dynamicExposure );
			if( !upscalingContext )
			{
				velocity = {};
				opaqueColor = {};
			}
		}
		else
		{
			m_taaFrameCounter = 0;
		}

		if( dynamicExposure )
		{
			histogramBuffer = RenderDynamicExposure( nonMsaaSource, gpuResourcePool, renderContext, dynamicExposure );
			if( !dynamicExposure->m_debug )
			{
				histogramBuffer = {};
			}
		}
	}

	Tr2GpuResourcePool::Texture upscaledSource;

	if( upscalingContext && upscalingInfo.temporal )
	{
		// need to set the hudless texture as it is needed for frame generation (and needs to be set before the upscaling call)
		upscalingContext->SetHudLessTexture( &output.Get() );

		upscaledSource = RenderUpscaling( nonMsaaSource, depthMap, velocity, opaqueColor, scene->GetReprojectionMatrix(), gpuResourcePool, renderContext, upscalingContext, dynamicExposure );
		depthMap = {};
		velocity = {};
		opaqueColor = {};

		// need to reset the perframedata so we have the correct viewport size etc
		scene->ApplyUpscalingToPerFrameData( displaySize.width, displaySize.height, renderContext );
	}
	else
	{
		upscaledSource = nonMsaaSource;
		if( !upscalingContext )
		{
			depthMap = {};
			velocity = {};
			opaqueColor = {};
		}
	}
	nonMsaaSource = {};

	// this needs to be after dynamic exposure, since bloom can be exposure dependent
	Tr2GpuResourcePool::Texture bloomTexture = GetBlackTexture( gpuResourcePool );
	if( postProcess != nullptr )
	{
		if( auto bloom = postProcess->GetBloomIfAvailable( m_quality ) )
		{
			bloomTexture = RenderBloom( upscaledSource, gpuResourcePool, renderContext, bloom, dynamicExposure );
		}
	}

	upscaledSource = RenderSharpening( sharpeningRequired, upscaledSource, gpuResourcePool, renderContext );

	TEMP_PARAM( m_tonemappingEffect, "BlitCurrent", bloomTexture );
	TEMP_PARAM( m_tonemappingEffect, "BlitOriginal", upscaledSource );
	TEMP_PARAM( m_tonemappingEffect, "Exposure", GetExposureBuffer( gpuResourcePool ) );
	TEMP_PARAM( m_tonemappingEffect, "Histogram", histogramBuffer );

	Tr2PPFilmGrainEffect* filmGrain = postProcess != nullptr ? postProcess->GetFilmGrainIfAvailable( m_quality ) : nullptr;

	if( !upscalingInfo.temporal || filmGrain != nullptr )
	{
		GPU_REGION( renderContext, "Tonemapping" );
		if( upscalingContext && !upscalingInfo.temporal )
		{
			auto tonemappedOutput = gpuResourcePool.GetTempTexture( "Tonemapping Result", renderSize, destination.GetFormat(), RENDER_TARGET );

			RenderTonemapping( tonemappedOutput, postProcess, renderContext );

			output = RenderUpscaling( tonemappedOutput, depthMap, velocity, opaqueColor, scene->GetReprojectionMatrix(), gpuResourcePool, renderContext, upscalingContext, dynamicExposure );
			depthMap = {};
			velocity = {};
			opaqueColor = {};

			// need to reset the perframedata so we have the correct viewport size etc
			scene->ApplyUpscalingToPerFrameData( displaySize.width, displaySize.height, renderContext );
		}
		else
		{
			RenderTonemapping( output, postProcess, renderContext );
		}

		renderContext.m_esm.SetRenderTarget( 0, destination );
		if( filmGrain != nullptr )
		{
			RenderFilmGrain( output, renderContext, filmGrain );
		}
		else
		{
			Tr2Renderer::DrawTexture( renderContext, output );
		}
	}
	else
	{
		RenderTonemapping( output, postProcess, renderContext );
		Tr2Renderer::DrawTexture( renderContext, output );
	}

	if( postProcess != nullptr )
	{
		if( auto signalLoss = postProcess->GetSignalLossIfAvailable( m_quality ) )
		{
			RenderSignalLoss( output, renderContext, signalLoss );
		}
	}

	RenderDynamicExposureDebug( gpuResourcePool, renderContext, dynamicExposure, histogramBuffer );

	renderContext.m_esm.PopDepthStencilBuffer();
	renderContext.m_esm.PopRenderTarget();
}

void Tr2PostProcessRenderer::SetupExposureConversion( bool enable, float middleValue )
{
	if( enable )
	{
		m_dynamicExposureToTextureShader->SetParameter( MEMOIZED_STRING( "ExposureMiddleValue" ), middleValue );
	}
}

Tr2GpuResourcePool::Texture Tr2PostProcessRenderer::RenderSharpening( bool enable, Tr2GpuResourcePool::Texture& input, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext )
{
	if( !enable )
	{
		return input;
	}
	GPU_REGION( renderContext, "CAS Sharpening" );

	static const uint32_t CAS_THREAD_GROUP_WORK_REGION_DIM = 16;
	auto format = GetUavCompatibleFormat( input->GetFormat() );
	auto output = gpuResourcePool.GetTempTexture( "Sharpening Output", input->GetWidth(), input->GetHeight(), format, RENDER_TARGET | Tr2GpuUsage::UNORDERED_ACCESS );

	auto renderWidth = output->GetWidth();
	auto renderHeight = output->GetHeight();
	AF1 outWidth = static_cast<AF1>( renderWidth );
	AF1 outHeight = static_cast<AF1>( renderHeight );
	float casIntensity = 0.5f;

	AMDSharpening::CASConstants casConst;

	CasSetup( casConst.const0.u, casConst.const1.u, casIntensity, outWidth, outHeight, outWidth, outHeight );

	m_fidelityFxCasShader->SetParameter( MEMOIZED_STRING( "const0" ), AMDSharpening::AsVector( casConst.const0 ) );
	m_fidelityFxCasShader->SetParameter( MEMOIZED_STRING( "const1" ), AMDSharpening::AsVector( casConst.const1 ) );
	TEMP_PARAM( m_fidelityFxCasShader, "InputTexture", input );
	TEMP_PARAM( m_fidelityFxCasShader, "OutputTexture", output );

	auto dispatchX = ( renderWidth + ( CAS_THREAD_GROUP_WORK_REGION_DIM - 1 ) ) / CAS_THREAD_GROUP_WORK_REGION_DIM;
	auto dispatchY = ( renderHeight + ( CAS_THREAD_GROUP_WORK_REGION_DIM - 1 ) ) / CAS_THREAD_GROUP_WORK_REGION_DIM;
	Tr2Renderer::RunComputeShader( m_fidelityFxCasShader, dispatchX, dispatchY, 1, renderContext );
	return output;
}

// Helper function to blur certain channel of a source render target to a destination render target with a blur type (Big/Small)
Tr2GpuResourcePool::Texture Tr2PostProcessRenderer::Blur( Tr2GpuResourcePool::Texture src, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, const PostProcessBlur::BlurContext& blurContext )
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
		effects.second->SetParameter( MEMOIZED_STRING( "Direction" ), Vector2( 0, 1 ) );

		BlueSharedString blurTypeOption = PostProcessBlur::GetBlurTypeOptionValue( blurContext.type );
		effects.first->SetOption( MEMOIZED_STRING( "BLUR_TYPE" ), blurTypeOption );
		effects.second->SetOption( MEMOIZED_STRING( "BLUR_TYPE" ), blurTypeOption );

		BlueSharedString blurChannelOption = PostProcessBlur::GetBlurChannelOptionValue( blurContext.channel );
		effects.first->SetOption( MEMOIZED_STRING( "BLUR_CHANNEL" ), blurChannelOption );
		effects.second->SetOption( MEMOIZED_STRING( "BLUR_CHANNEL" ), blurChannelOption );

		BlueSharedString blurChannelProcessOption = PostProcessBlur::GetProcessTypeOptionValue( blurContext.process );
		effects.first->SetOption( MEMOIZED_STRING( "BLUR_PROCESS_TYPE" ), blurChannelProcessOption );
		effects.second->SetOption( MEMOIZED_STRING( "BLUR_PROCESS_TYPE" ), blurChannelProcessOption );

		effects.first->SetOption( MEMOIZED_STRING( "BLUR_FINALIZE_TYPE" ), PostProcessBlur::GetFinalizeTypeOptionValue( PostProcessBlur::BF_None ) );
		effects.second->SetOption( MEMOIZED_STRING( "BLUR_FINALIZE_TYPE" ), PostProcessBlur::GetFinalizeTypeOptionValue( blurContext.finalize ) );

		effects.first->EndUpdate();
		effects.second->EndUpdate();
		m_blurEffects.insert( std::pair( blurContext.Hash(), effects ) );
	}
	else
	{
		effects = lookup->second;
	}

	auto rt2 = gpuResourcePool.GetTempTexture( "Blur Temp 1", src->GetWidth(), src->GetHeight(), src->GetFormat(), RENDER_TARGET );
	TEMP_PARAM( effects.first, "BlitCurrent", src );
	DrawInto( rt2, Tr2LoadAction::DONT_CARE, effects.first, renderContext );
	src = {};

	auto rt1 = gpuResourcePool.GetTempTexture( "Blur Temp 2", rt2->GetWidth(), rt2->GetHeight(), rt2->GetFormat(), RENDER_TARGET );
	TEMP_PARAM( effects.second, "BlitCurrent", rt2 );
	DrawInto( rt1, Tr2LoadAction::DONT_CARE, effects.second, renderContext );
	return rt1;
}

Tr2GpuResourcePool::Texture Tr2PostProcessRenderer::DownSampleDepth( const Tr2TextureAL& depth, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext )
{
	TEMP_PARAM( m_downsampleDepthEffect, "DepthMap", depth );
	auto destination = gpuResourcePool.GetTempTexture( "Down-sampled Depth", TextureSize2D( depth.GetDesc() ) * 0.5f, ImageIO::PIXEL_FORMAT_R32_FLOAT, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
	DrawInto( destination, Tr2LoadAction::DONT_CARE, m_downsampleDepthEffect, renderContext );
	return destination;
}


Tr2GpuResourcePool::Texture Tr2PostProcessRenderer::RenderBloom( Tr2GpuResourcePool::Texture& dest, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPBloomEffect* bloom, Tr2PPDynamicExposureEffect* dynamicExposure )
{
	GPU_REGION( renderContext, "Bloom" );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	bool hasDynamicExposure = dynamicExposure != nullptr;
	bool exposureDependant = bloom->m_exposureDependency && hasDynamicExposure;

	if( !m_useNewBloom )
	{
		m_bloomHighPassFilter->SetParameter( MEMOIZED_STRING( "LuminanceThreshold" ), std::max( 0.0f, bloom->m_luminanceThreshold ) );
		m_bloomHighPassFilter->SetParameter( MEMOIZED_STRING( "LuminanceScale" ), bloom->m_luminanceScale );
		m_bloomHighPassFilter->SetParameter( MEMOIZED_STRING( "ExposureDependency" ), exposureDependant ? 1.0f : 0.0f );
		auto rt1 = gpuResourcePool.GetTempTexture( "Bloom", TextureSize2D( dest->GetDesc() ) * 0.5f, dest->GetFormat(), RENDER_TARGET );
		TEMP_PARAM( m_bloomHighPassFilter, "BlitCurrent", dest );
		TEMP_PARAM( m_bloomHighPassFilter, "Exposure", GetExposureBuffer( gpuResourcePool ) );
		DrawInto( rt1, Tr2LoadAction::DONT_CARE, m_bloomHighPassFilter, renderContext );

		return Blur( std::move( rt1 ), gpuResourcePool, renderContext, {} );
	}

	int depth = 0;
	auto black = GetBlackTexture( gpuResourcePool );

	float currentSize = 0.5f;
	uint32_t minDim = std::min( dest->GetHeight(), dest->GetWidth() );

	m_downSamplerLuminancePreserve->SetOption( MEMOIZED_STRING( "EXPOSURE_DEPENDANCE" ), hasDynamicExposure ? MEMOIZED_STRING( "EXPOSURE_DEPENDANCE_ON" ) : MEMOIZED_STRING( "EXPOSURE_DEPENDANCE_OFF" ) );
	m_downSamplerLuminancePreserve->SetParameter( MEMOIZED_STRING( "LuminanceThreshold" ), bloom->m_luminanceThreshold );

	std::array<Tr2GpuResourcePool::Texture, Bloom::MAX_BLOOM_STEPS> downsampleTexture;
	std::array<Tr2GpuResourcePool::Texture, Bloom::MAX_BLOOM_STEPS> upsampleHorizontalTexture;
	std::array<Tr2GpuResourcePool::Texture, Bloom::MAX_BLOOM_STEPS> upsampleTexture;

	for( int i = 0; i < Bloom::MAX_BLOOM_STEPS; ++i )
	{
		if( (uint32_t)( (float)minDim * currentSize ) == 0 )
		{
			break;
		}

		auto size = TextureSize2D( dest->GetDesc() ) * currentSize;

		auto name = "Downsample_" + std::to_string( i );
		downsampleTexture[i] = gpuResourcePool.GetTempTexture( name.c_str(), size, dest->GetFormat(), RENDER_TARGET );

		name = "Upsample_" + std::to_string( i );
		upsampleTexture[i] = gpuResourcePool.GetTempTexture( name.c_str(), size, dest->GetFormat(), RENDER_TARGET );

		if( m_bloomDebugMode != BloomDebugMode::BLOOM_DEBUG_NONE )
		{
			name = "Upsample_Horizontal_" + std::to_string( i );
			upsampleHorizontalTexture[i] = gpuResourcePool.GetTempTexture( name.c_str(), size, dest->GetFormat(), RENDER_TARGET );
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
			auto invTexelSize = Vector2( 1.0f / (float)lastRt->GetWidth(), 1.0f / (float)lastRt->GetHeight() );

			TEMP_PARAM( effect, "BlitCurrent", lastRt );
			TEMP_PARAM( effect, "Exposure", GetExposureBuffer( gpuResourcePool ) );

			auto downsampleInfo = DownsampleData{
				invTexelSize
			};
			FillAndSetConstants( m_bloomConstantBuffer, &downsampleInfo, sizeof( downsampleInfo ), Tr2RenderContextEnum::PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );

			DrawInto( rt, Tr2LoadAction::DONT_CARE, effect, renderContext );

			lastRt = rt;
		}
	}

	{
		// do a two pass downsampling with gaussian blur on the downsampled texture
		// then the last upsampled texture will be added on top

		GPU_REGION( renderContext, "Upsample" );

		float tintScale = ( 1.0f / Bloom::MAX_BLOOM_STEPS ) * bloom->m_bloomBrightness;

		Vector2 directionalWeight = Vector2( std::max( bloom->m_directionalWeight, 0.0f ), std::fabsf( bloom->m_directionalWeight ) );
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

			m_upsamplerHorizontal->SetParameter( MEMOIZED_STRING( "BlitCurrent" ), currentMip );
			auto gaussianOutput = GaussianDistribution::CalculateGaussianPassParameters( radiusInPixels, directionalWeight.x, invTexelSize.x, Vector3( 1, 1, 1 ), Vector2( 1, 0 ) );
			FillAndSetConstants( m_bloomConstantBuffer, &gaussianOutput, sizeof( gaussianOutput ), Tr2RenderContextEnum::PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
			DrawInto( currentUpsampled, Tr2LoadAction::DONT_CARE, m_upsamplerHorizontal, renderContext );
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
			m_upsamplerVertical->SetParameter( MEMOIZED_STRING( "BlitCurrent" ), currentUpsampled );
			m_upsamplerVertical->SetParameter( MEMOIZED_STRING( "LastMip" ), lastRt );

			Vector4 tint = bloom->m_stepTints[i] * tintScale;
			auto gaussianOutput = GaussianDistribution::CalculateGaussianPassParameters( radiusInPixels, directionalWeight.y, invTexelSize.y, tint.GetXYZ(), Vector2( 0, 1 ) );
			FillAndSetConstants( m_bloomConstantBuffer, &gaussianOutput, sizeof( gaussianOutput ), Tr2RenderContextEnum::PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
			// draw into the downsample texture, because they will not be used again
			DrawInto( currentMip, Tr2LoadAction::DONT_CARE, m_upsamplerVertical, renderContext );

			lastRt = currentMip;
		}
	}

	if( m_bloomDebugMode != BloomDebugMode::BLOOM_DEBUG_NONE )
	{
		return RenderBloomDebug( downsampleTexture, upsampleTexture, dest, gpuResourcePool, renderContext );
	}

	return lastRt;
}

Tr2GpuResourcePool::Texture Tr2PostProcessRenderer::RenderBloomDebug(
	std::array<Tr2GpuResourcePool::Texture, Bloom::MAX_BLOOM_STEPS>& downsample,
	std::array<Tr2GpuResourcePool::Texture, Bloom::MAX_BLOOM_STEPS>& upsample,
	Tr2GpuResourcePool::Texture& blitCurrent,
	Tr2GpuResourcePool& gpuResourcePool,
	Tr2RenderContext& renderContext )
{
	if( m_bloomDebugShader == nullptr )
	{
		m_bloomDebugShader.CreateInstance();
		m_bloomDebugShader->StartUpdate();
		m_bloomDebugShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/BloomDebug.fx" );
		m_bloomDebugShader->EndUpdate();
	}
	GPU_REGION( renderContext, "Bloom Debug" );

	m_bloomDebugShader->SetOption( MEMOIZED_STRING( "BLOOM_DEBUG_MODE" ), GetBloomDebugShaderOptionValue( m_bloomDebugMode ) );
	m_bloomDebugShader->SetParameter( MEMOIZED_STRING( "BlitCurrent" ), blitCurrent );
	for( int i = 0; i < Bloom::MAX_BLOOM_STEPS; ++i )
	{
		m_bloomDebugShader->SetParameter( BlueSharedString( "Downsample" + std::to_string( i + 1 ) ), downsample[i] );
		m_bloomDebugShader->SetParameter( BlueSharedString( "Upsample" + std::to_string( i + 1 ) ), upsample[i] );
	}

	auto debugView = gpuResourcePool.GetTempTexture( "Bloom Debug", blitCurrent->GetWidth(), blitCurrent->GetHeight(), blitCurrent->GetFormat(), RENDER_TARGET );

	DrawInto( debugView, Tr2LoadAction::DONT_CARE, m_bloomDebugShader, renderContext );

	return debugView;
}

void Tr2PostProcessRenderer::RenderGodRays( const Tr2TextureAL& dest, const Tr2TextureAL& depth, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPGodRaysEffect* godrays )
{
	GPU_REGION( renderContext, "Godrays" );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	// Downsample depth
	auto rt1 = DownSampleDepth( depth, gpuResourcePool, renderContext );

	// God rays
	auto rt2 = gpuResourcePool.GetTempTexture( "God rays", TextureSize2D( dest.GetDesc() ) * 0.5f, dest.GetFormat(), RENDER_TARGET );
	renderContext.m_esm.PushRenderTarget( rt2 );
	renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 0 ); // clear is needed because godray vertex shader can opt out of rendering

	m_godrayEffect->SetParameter( MEMOIZED_STRING( "Color" ), Vector4( godrays->m_godRayColor ) );
	m_godrayEffect->SetParameter( MEMOIZED_STRING( "Intensity" ), Vector4( godrays->m_intensity, 0.0f, 1.0f, 1.0f ) );
	m_godrayEffect->SetParameter( MEMOIZED_STRING( "grFactors" ), godrays->grFactors );
	m_godrayEffect->SetResourceTexture2D( MEMOIZED_STRING( "NoiseTexMap" ), godrays->m_noiseTexturePath.c_str() );
	TEMP_PARAM( m_godrayEffect, "DepthMap", rt1 );

	Tr2Renderer::DrawScreenQuad( renderContext, m_godrayEffect );
	renderContext.m_esm.PopRenderTarget();

	// Blit
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
	DrawInto( dest, Tr2LoadAction::LOAD, rt2, renderContext );
}

void Tr2PostProcessRenderer::RenderSignalLoss( const Tr2TextureAL& dest, Tr2RenderContext& renderContext, Tr2PPSignalLossEffect* signalLoss )
{
	GPU_REGION( renderContext, "Signal Loss" );
	m_signalLossEffect->SetParameter( MEMOIZED_STRING( "NoiseStrength" ), signalLoss->m_strength );

	Tr2Renderer::DrawTexture( renderContext, m_signalLossEffect, dest, Vector2( 0, 0 ), Vector2( 1, 1 ) );
}


Tr2GpuResourcePool::Buffer Tr2PostProcessRenderer::RenderDynamicExposure( const Tr2TextureAL& source, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure )
{
	GPU_REGION( renderContext, "Exposure" );

	uint32_t tilesX = source.GetWidth() / HISTOGRAM_TILE_SIZE_X + 1;
	uint32_t tilesY = source.GetHeight() / HISTOGRAM_TILE_SIZE_Y + 1;

	uint32_t localHistogramCount = tilesX * tilesY * 16;

	auto localHistograms = gpuResourcePool.GetTempBuffer(
		"LocalHistograms",
		Tr2BufferDescriptionAL( Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_UINT, localHistogramCount, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::UNORDERED_ACCESS, Tr2CpuUsage::NONE ) );


	auto histogram = gpuResourcePool.GetTempBuffer(
		"Histogram",
		Tr2BufferDescriptionAL( Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, 65, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::UNORDERED_ACCESS, Tr2CpuUsage::NONE ) );


	// we also need to update the tonemapping buffer
	m_dynamicExposureCreateHistogramShader->SetParameter( MEMOIZED_STRING( "ScreenTilesX" ), float( tilesX ) );
	TEMP_PARAM( m_dynamicExposureCreateHistogramShader, "BlitOriginal", source );
	TEMP_PARAM( m_dynamicExposureCreateHistogramShader, "LocalHistograms", localHistograms );
	m_dynamicExposureMergeHistogramShader->SetParameter( MEMOIZED_STRING( "ScreenTilesX" ), float( tilesX ) );
	m_dynamicExposureMergeHistogramShader->SetParameter( MEMOIZED_STRING( "ScreenTilesY" ), float( tilesY ) );
	TEMP_PARAM( m_dynamicExposureMergeHistogramShader, "LocalHistograms", localHistograms );
	TEMP_PARAM( m_dynamicExposureMergeHistogramShader, "Histogram", histogram );

	uint32_t m_uintValue[4] = { 0, 0, 0, 0 };
	// Clear local histograms
	renderContext.ClearUav( localHistograms, m_uintValue );

	// Clear histograms
	renderContext.ClearUav( histogram, m_uintValue );

	// Create histograms
	m_dynamicExposureCreateHistogramShader->SetParameter( MEMOIZED_STRING( "MinLuminance" ), log( dynamicExposure->m_minLuminance ) );
	m_dynamicExposureCreateHistogramShader->SetParameter( MEMOIZED_STRING( "MaxLuminance" ), log( dynamicExposure->m_maxLuminance ) );
	m_dynamicExposureCreateHistogramShader->SetParameter( MEMOIZED_STRING( "MinBrightness" ), dynamicExposure->m_minBrightness );
	m_dynamicExposureCreateHistogramShader->SetParameter( MEMOIZED_STRING( "MaxBrightness" ), dynamicExposure->m_maxBrightness );
	Tr2Renderer::RunComputeShader( m_dynamicExposureCreateHistogramShader, tilesX, tilesY, 1, renderContext );

	// Merge histogram
	uint32_t mergeHistogramXDim = tilesX * tilesY / NUM_TILES_PER_THREAD_GROUP + 1;
	Tr2Renderer::RunComputeShader( m_dynamicExposureMergeHistogramShader, mergeHistogramXDim, 1, 1, renderContext );

	// Measure histogram
	m_dynamicExposureMeasureExposureShader->SetParameter( MEMOIZED_STRING( "MinLuminance" ), log( dynamicExposure->m_minLuminance ) );
	m_dynamicExposureMeasureExposureShader->SetParameter( MEMOIZED_STRING( "MaxLuminance" ), log( dynamicExposure->m_maxLuminance ) );
	m_dynamicExposureMeasureExposureShader->SetParameter( MEMOIZED_STRING( "MinBrightness" ), dynamicExposure->m_minBrightness );
	m_dynamicExposureMeasureExposureShader->SetParameter( MEMOIZED_STRING( "MaxBrightness" ), dynamicExposure->m_maxBrightness );
	m_dynamicExposureMeasureExposureShader->SetParameter( MEMOIZED_STRING( "IncreaseSpeed" ), dynamicExposure->m_increaseSpeed );
	m_dynamicExposureMeasureExposureShader->SetParameter( MEMOIZED_STRING( "DecreaseSpeed" ), dynamicExposure->m_decreaseSpeed );
	m_dynamicExposureMeasureExposureShader->SetParameter( MEMOIZED_STRING( "MinExposure" ), dynamicExposure->m_minExposure );
	m_dynamicExposureMeasureExposureShader->SetParameter( MEMOIZED_STRING( "MaxExposure" ), dynamicExposure->m_maxExposure );
	TEMP_PARAM( m_dynamicExposureMeasureExposureShader, "Histogram", histogram );
	TEMP_PARAM( m_dynamicExposureMeasureExposureShader, "Exposure", GetExposureBuffer( gpuResourcePool ) );
	Tr2Renderer::RunComputeShader( m_dynamicExposureMeasureExposureShader, 1, 1, 1, renderContext );
	return histogram;
}

void Tr2PostProcessRenderer::RenderDynamicExposureDebug( Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPDynamicExposureEffect* dynamicExposure, const Tr2BufferAL& histogramBuffer )
{
	if( dynamicExposure && dynamicExposure->m_debug )
	{
		GPU_REGION( renderContext, "Dynamic Exposure Debug" );

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
		auto exposure = GetExposureBuffer( gpuResourcePool );
		m_dynamicExposureDebugShader->SetParameter( MEMOIZED_STRING( "Exposure" ), exposure );
		m_dynamicExposureDebugShader->SetParameter( MEMOIZED_STRING( "Histogram" ), histogramBuffer );
		m_dynamicExposureDebugShader->SetParameter( MEMOIZED_STRING( "MinLuminance" ), log( dynamicExposure->m_minLuminance ) );
		m_dynamicExposureDebugShader->SetParameter( MEMOIZED_STRING( "MaxLuminance" ), log( dynamicExposure->m_maxLuminance ) );
		m_dynamicExposureDebugShader->SetParameter( MEMOIZED_STRING( "RectSize" ), Vector2( float( rect.left - rect.right ), float( rect.bottom - rect.top ) ) );

		Tr2TextureAL noTexture;
		Tr2Renderer::DrawTexture( renderContext, m_dynamicExposureDebugShader, noTexture, Vector2( 0, 0 ), Vector2( 1, 1 ), Vector2( 0, 0.7f ), Vector2( 1, 1 ) );

		const float* exposureData = nullptr;
		if( SUCCEEDED( exposure->MapForReading( exposureData, renderContext ) ) )
		{
			Tr2Renderer::PrintfImmediate( renderContext, TRI_DBG_FONT_SMALL, rect, TRI_DFS_CENTER, 0xff00ff00, "Middle gray: %.2f", exposureData[0] );
			Tr2Renderer::PrintfImmediate( renderContext, TRI_DBG_FONT_SMALL, rect, TRI_DFS_LEFT, 0xff00ff00, "Min: %.2f", exposureData[5] );
			Tr2Renderer::PrintfImmediate( renderContext, TRI_DBG_FONT_SMALL, rect, TRI_DFS_RIGHT, 0xff00ff00, "Max: %.2f", exposureData[6] );

			exposure->UnmapForReading( renderContext );
		}
	}
	else
	{
		m_dynamicExposureDebugShader = nullptr;
	}
}

Tr2GpuResourcePool::Texture Tr2PostProcessRenderer::RenderUpscaling(
	const Tr2TextureAL& source,
	const Tr2TextureAL& depth,
	const Tr2TextureAL& velocity,
	const Tr2TextureAL& opaqueColor,
	const Matrix& reprojection,
	Tr2GpuResourcePool& gpuResourcePool,
	Tr2RenderContext& renderContext,
	Tr2UpscalingContextAL* upscalingContext,
	Tr2PPDynamicExposureEffect* dynamicExposure )
{
	GPU_REGION( renderContext, "Upscaling" );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
	uint32_t w, h;
	upscalingContext->GetDisplayDimensions( w, h );

	Tr2UpscalingAL::DispatchParameters dispatchParameters = {};
	auto dispatchRequirements = upscalingContext->GetDispatchRequirements();
	auto dest = gpuResourcePool.GetTempTexture( "Upscaled Output", w, h, GetUavCompatibleFormat( source.GetFormat() ), RENDER_TARGET | Tr2GpuUsage::UNORDERED_ACCESS );
	dispatchParameters.output = &dest.Get();

	bool wantsExposure = dispatchRequirements & Tr2UpscalingAL::DispatchRequirements::OPTIONAL_EXPOSURE;
	bool canHaveExposure = dynamicExposure != nullptr;
	float middleValue = dynamicExposure != nullptr ? dynamicExposure->m_middleValue : 0.0f;
	SetupExposureConversion( wantsExposure && canHaveExposure, middleValue );

	Tr2GpuResourcePool::Texture exposureTexture;
	if( wantsExposure && canHaveExposure )
	{
		GPU_REGION( renderContext, "ExposureToTexture" );
		TEMP_PARAM( m_dynamicExposureToTextureShader, "ExposureBuffer", GetExposureBuffer( gpuResourcePool ) );
		exposureTexture = gpuResourcePool.GetTempTexture( "Exposure", 1, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::UNORDERED_ACCESS );
		TEMP_PARAM( m_dynamicExposureToTextureShader, "ExposureTexture", exposureTexture );
		Tr2Renderer::RunComputeShader( m_dynamicExposureToTextureShader, 1, 1, 1, renderContext );
		m_dynamicExposureToTextureShader->SetParameter( MEMOIZED_STRING( "ExposureTexture" ), Tr2TextureAL{} );
		dispatchParameters.exposure = &exposureTexture.Get();
	}

	Tr2GpuResourcePool::Texture reactiveMask;
	if( dispatchRequirements & Tr2UpscalingAL::DispatchRequirements::REACTIVE )
	{
		GPU_REGION( renderContext, "ReactiveMask" );
		reactiveMask = gpuResourcePool.GetTempTexture( "ReactiveMask", source.GetWidth(), source.GetHeight(), Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM, RENDER_TARGET );
		TEMP_PARAM( m_reactiveMaskEffect, "OpaqueOnly", opaqueColor );
		TEMP_PARAM( m_reactiveMaskEffect, "OpaqueAndTransparency", source );
		DrawInto( reactiveMask, Tr2LoadAction::DONT_CARE, m_reactiveMaskEffect, renderContext );
		dispatchParameters.reactive = &reactiveMask.Get();
	}

	Tr2GpuResourcePool::Texture transparencyMask;
	if( dispatchRequirements & Tr2UpscalingAL::DispatchRequirements::TRANSPARENCY )
	{
		GPU_REGION( renderContext, "Transparency" );
		transparencyMask = gpuResourcePool.GetTempTexture( "TransparencyMask", source.GetWidth(), source.GetHeight(), Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM, RENDER_TARGET );
		TEMP_PARAM( m_transparencyMaskEffect, "OpaqueOnly", opaqueColor );
		TEMP_PARAM( m_transparencyMaskEffect, "OpaqueAndTransparency", source );
		DrawInto( transparencyMask, Tr2LoadAction::DONT_CARE, m_transparencyMaskEffect, renderContext );
		dispatchParameters.transparency = &transparencyMask.Get();
	}

	auto view = Tr2Renderer::GetViewTransform();
	auto projection = Tr2Renderer::GetProjectionTransform();
	auto inverseProjection = Inverse( projection );
	auto invReprojection = Inverse( reprojection );

	dispatchParameters.aspectRatio = Tr2Renderer::GetAspectRatio();
	dispatchParameters.backClip = Tr2Renderer::GetBackClip();
	dispatchParameters.frontClip = Tr2Renderer::GetFrontClip();
	dispatchParameters.fieldOfView = Tr2Renderer::GetFieldOfView();
	dispatchParameters.frameTimeDelta = TimeAsFloat( BeOS->GetCurrentFrameTime() - m_lastFrameTime ) * 1000.0f;
	dispatchParameters.preExposure = 0.4f;
	dispatchParameters.currentFrameIndex = Tr2Renderer::GetCurrentFrameCounter();
	dispatchParameters.reset = false;

	memcpy( dispatchParameters.cameraPos, &Tr2Renderer::GetViewPosition(), 3 * sizeof( float ) );
	memcpy( dispatchParameters.cameraForward, &view.GetZ(), 3 * sizeof( float ) );
	memcpy( dispatchParameters.cameraUp, &view.GetY(), 3 * sizeof( float ) );
	memcpy( dispatchParameters.cameraRight, &view.GetX(), 3 * sizeof( float ) );
	memcpy( dispatchParameters.projection, &projection, 16 * sizeof( float ) );
	memcpy( dispatchParameters.invProjection, &inverseProjection, 16 * sizeof( float ) );
	memcpy( dispatchParameters.clipToPrevClip, &reprojection, 16 * sizeof( float ) );
	memcpy( dispatchParameters.prevClipToClip, &invReprojection, 16 * sizeof( float ) );

	dispatchParameters.depth = &depth;
	dispatchParameters.input = &source;
	dispatchParameters.opaqueOnly = &opaqueColor;
	dispatchParameters.velocity = &velocity;

	dispatchParameters.upscalingDebugView = g_upscalingDebugView;
	dispatchParameters.frameGenDebugView = g_frameGenDebugView;

	m_lastFrameTime = BeOS->GetCurrentFrameTime();
	{
		GPU_REGION( renderContext, "UpscalingTechnique" );
		auto result = upscalingContext->Dispatch( dispatchParameters );
		if( result != Tr2UpscalingAL::OK && s_lastUpscalingResult != result )
		{
			Tr2UpscalingAL::LogResult( result );
		}
		s_lastUpscalingResult = result;
	}
	return dest;
}

void Tr2PostProcessRenderer::RenderFilmGrain( const Tr2TextureAL& dest, Tr2RenderContext& renderContext, Tr2PPFilmGrainEffect* filmGrain )
{
	GPU_REGION( renderContext, "Film grain" );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
	m_grainShader->SetParameter( MEMOIZED_STRING( "GrainColorAmount" ), filmGrain->m_colored ? filmGrain->m_colorAmount : 0.0f );
	m_grainShader->SetParameter( MEMOIZED_STRING( "GrainSize" ), filmGrain->m_grainSize );
	m_grainShader->SetParameter( MEMOIZED_STRING( "GrainIntensity" ), filmGrain->m_intensity );
	m_grainShader->SetParameter( MEMOIZED_STRING( "GrainThreshold" ), 1.0f - filmGrain->m_grainDensity );
	m_grainShader->SetParameter( MEMOIZED_STRING( "GrainEdge" ), 1.0f / ( filmGrain->m_grainContrast * filmGrain->m_grainSize ) );
	m_grainShader->SetParameter( MEMOIZED_STRING( "BrightnessModifier" ), filmGrain->m_brightnessModifier );
	m_grainShader->SetParameter( MEMOIZED_STRING( "InputTexture" ), dest );
	Tr2Renderer::DrawScreenQuad( renderContext, m_grainShader );
}

void Tr2PostProcessRenderer::RenderFog( const Tr2TextureAL& dest, const Tr2TextureAL& source, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPFogEffect* fog )
{
	GPU_REGION( renderContext, "Fog" );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );


	// render fog color
	auto rt1 = gpuResourcePool.GetTempTexture( "Fog Color", TextureSize2D( dest.GetDesc() ) * 0.5f, dest.GetFormat(), RENDER_TARGET );
	TEMP_PARAM( m_fogColorEffect, "BlitCurrent", dest );
	m_fogColorEffect->SetParameter( MEMOIZED_STRING( "Params" ), Vector4( fog->m_nebulaInfluence, fog->m_nebulaBlur, fog->m_originalBrightenOnly, fog->m_colorInfluence ) );
	m_fogColorEffect->SetParameter( MEMOIZED_STRING( "Color" ), Vector4( fog->m_color ) );
	DrawInto( rt1, Tr2LoadAction::DONT_CARE, m_fogColorEffect, renderContext );

	// blur
	auto blurred = Blur( std::move( rt1 ), gpuResourcePool, renderContext, {} );

	// final composite
	m_fogCompositeEffect->SetParameter( MEMOIZED_STRING( "FogParameters" ), Vector4( fog->m_totalAmount, fog->m_totalPower, fog->m_backgroundOcclusion, fog->m_intensity ) );
	m_fogCompositeEffect->SetParameter( MEMOIZED_STRING( "BrightnessAdjustment" ), Vector4( fog->m_brightnessThreshold0, fog->m_brightnessThreshold1, fog->m_brightnessAdjustmentAmount, 0.0f ) );
	m_fogCompositeEffect->SetParameter( MEMOIZED_STRING( "BlendFunction0" ), Vector4( fog->m_blendDistance0, fog->m_blendBias0, fog->m_blendAmount0, fog->m_blendPower0 ) );
	m_fogCompositeEffect->SetParameter( MEMOIZED_STRING( "BlendFunction1" ), Vector4( fog->m_blendDistance1, fog->m_blendBias1, fog->m_blendAmount1, fog->m_blendPower1 ) );
	m_fogCompositeEffect->SetParameter( MEMOIZED_STRING( "BlendFunction2" ), Vector4( fog->m_blendDistance2, fog->m_blendBias2, fog->m_blendAmount2, fog->m_blendPower2 ) );
	m_fogCompositeEffect->SetParameter( MEMOIZED_STRING( "AreaSize" ), Vector4( fog->m_areaSize, fog->m_areaScale.x ) );
	m_fogCompositeEffect->SetParameter( MEMOIZED_STRING( "AreaCenter" ), Vector4( fog->m_areaCenter, fog->m_areaScale.y ) );
	TEMP_PARAM( m_fogCompositeEffect, "BlitCurrent", blurred );
	TEMP_PARAM( m_fogCompositeEffect, "BlitOriginal", source );
	DrawInto( dest, Tr2LoadAction::DONT_CARE, m_fogCompositeEffect, renderContext );
}

void Tr2PostProcessRenderer::RenderTaa( const Tr2TextureAL& dest, const Tr2TextureAL& velocity, const Tr2TextureAL& opaqueColor, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPTaaEffect* taa, Tr2PPDynamicExposureEffect* dynamicExposure )
{
	GPU_REGION( renderContext, "TAA" );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	auto Clear = []( const Tr2TextureAL& tex, Tr2RenderContextAL& renderContext ) {
		renderContext.SetRenderTarget( tex, 0 );
		renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 1.0f );
	};
	auto ClearUav = []( const Tr2TextureAL& tex, Tr2RenderContextAL& renderContext ) {
		uint32_t zeroes[4] = {};
		renderContext.ClearUav( tex, 0, zeroes );
	};

	auto accumulationBuffer0 = gpuResourcePool.GetPersistentTexture(
		"TAA Accumulation 0",
		dest.GetWidth(),
		dest.GetHeight(),
		Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_UNORM,
		Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE,
		Clear );
	auto accumulationBuffer1 = gpuResourcePool.GetPersistentTexture(
		"TAA Accumulation 1",
		dest.GetWidth(),
		dest.GetHeight(),
		Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_UNORM,
		Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE,
		Clear );

	auto cooldownBuffer = gpuResourcePool.GetPersistentTexture(
		"TAA Cooldown",
		dest.GetWidth(),
		dest.GetHeight(),
		Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT,
		Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE,
		ClearUav );


	Tr2TextureAL input;
	Tr2TextureAL output;

	uint32_t frame_count = m_taaFrameCounter++;
	if( ( frame_count & 1 ) == 0 )
	{
		input = accumulationBuffer0;
		output = accumulationBuffer1;
	}
	else
	{
		input = accumulationBuffer1;
		output = accumulationBuffer0;
	}


	Tr2EffectPtr effects[] = { m_taaEffect, m_taaCopyEffect };
	for( auto& effect : effects )
	{
		if( dynamicExposure )
		{
			effect->SetParameter( MEMOIZED_STRING( "ExposureAdjust" ), pow( 2.0f, dynamicExposure->m_adjustment ) );
			effect->SetParameter( MEMOIZED_STRING( "ExposureMiddleValue" ), dynamicExposure->m_middleValue );
			effect->SetParameter( MEMOIZED_STRING( "ExposureInfluence" ), dynamicExposure->m_influence );

			effect->SetOption( MEMOIZED_STRING( "DYNAMIC_EXPOSURE_TOGGLE" ), MEMOIZED_STRING( "DYNAMIC_EXPOSURE_ENABLED" ) );
		}
		else
		{
			effect->SetOption( MEMOIZED_STRING( "DYNAMIC_EXPOSURE_TOGGLE" ), MEMOIZED_STRING( "DYNAMIC_EXPOSURE_DISABLED" ) );
		}
	}

	m_taaEffect->SetParameter( MEMOIZED_STRING( "FrameIndex" ), frame_count );

	m_taaEffect->SetParameter( MEMOIZED_STRING( "EarlyOutThreshold" ), taa->m_earlyOutThreshold );
	m_taaEffect->SetOption( MEMOIZED_STRING( "QUALITY" ), GetTaaQualityShaderOptionValue( taa->m_quality ) );
	m_taaEffect->SetOption( MEMOIZED_STRING( "DEBUG" ), GetTaaDebugShaderOptionValue( taa->m_debugMode ) );
	TEMP_PARAM( m_taaEffect, "CurrentFrame", dest );
	TEMP_PARAM( m_taaEffect, "CurrentFrameOpaque", opaqueColor );
	TEMP_PARAM( m_taaEffect, "AccumulationBuffer", input );
	TEMP_PARAM( m_taaEffect, "CooldownMap", cooldownBuffer );
	TEMP_PARAM( m_taaEffect, "VelocityMap", velocity );
	TEMP_PARAM( m_taaEffect, "Exposure", dynamicExposure ? GetExposureBuffer( gpuResourcePool ) : Tr2BufferAL{} );

	float max_weight = 0.96f;
	float weight = min( (float)frame_count / ( (float)frame_count + 1.0f ), max_weight );
	m_taaEffect->SetParameter( MEMOIZED_STRING( "BlendWeight" ), weight );

	DrawInto( output, Tr2LoadAction::DONT_CARE, m_taaEffect, renderContext );


	TEMP_PARAM( m_taaCopyEffect, "AccumulationBuffer", output );
	TEMP_PARAM( m_taaCopyEffect, "Exposure", dynamicExposure ? GetExposureBuffer( gpuResourcePool ) : Tr2BufferAL{} );

	DrawInto( dest, Tr2LoadAction::DONT_CARE, m_taaCopyEffect, renderContext );
}

void Tr2PostProcessRenderer::RenderTonemapping(
	const Tr2TextureAL& dest,
	Tr2PostProcess2* postprocess,
	Tr2RenderContext& renderContext )
{
	GPU_REGION( renderContext, "Tonemapping" );

	m_tonemappingEffect->SetParameter( MEMOIZED_STRING( "OutputGamma" ), g_eveSpaceSceneGammaBrightness );

	Tonemapping::ApplyColorCorrection( postprocess ? postprocess->GetColorCorrectionIfAvailable( m_quality ) : nullptr, m_tonemappingEffect );
	Tonemapping::ApplyBloom( postprocess ? postprocess->GetBloomIfAvailable( m_quality ) : nullptr, m_tonemappingEffect, m_useNewBloom, m_bloomDebugMode );
	Tonemapping::ApplyDynamicExposure( postprocess ? postprocess->GetDynamicExposureIfAvailable( m_quality ) : nullptr, m_tonemappingEffect, postprocess ? postprocess->m_exposureAdjustment : 0.0f );
	Tonemapping::ApplyVignette( postprocess ? postprocess->GetVignetteIfAvailable( m_quality ) : nullptr, m_tonemappingEffect );
	Tonemapping::ApplyDesatureate( postprocess ? postprocess->GetDesaturateIfAvailable( m_quality ) : nullptr, m_tonemappingEffect );
	Tonemapping::ApplyFade( postprocess ? postprocess->GetFadeIfAvailable( m_quality ) : nullptr, m_tonemappingEffect );

	std::vector<const Tr2PPLutEffect*> luts{};
	if( postprocess )
	{
		postprocess->GetAvilableSortedLuts( luts );
	}
	Tonemapping::ApplyLuts( luts, m_tonemappingEffect );

	if( postprocess )
	{
		if( auto tonemapping = postprocess->GetTonemappingIfAvailable() )
		{
			if( tonemapping->m_method == Tr2PPTonemappingEffect::Aces )
			{
				Tonemapping::ApplyAcesTonemappingMethod( tonemapping, m_tonemappingEffect );
			}
			else if( tonemapping->m_method == Tr2PPTonemappingEffect::AgX )
			{
				Tonemapping::ApplyAgxTonemappingMethod( m_tonemappingEffect );
			}
			else
			{
				Tonemapping::ApplyUncharted2TonemappingMethod( tonemapping, m_tonemappingEffect );
			}
		}
		else
		{
			Tonemapping::ApplyNoTonemappingMethod( m_tonemappingEffect );
		}
	}
	else
	{
		Tonemapping::ApplyNoTonemappingMethod( m_tonemappingEffect );
	}

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
	DrawInto( dest, Tr2LoadAction::DONT_CARE, m_tonemappingEffect, renderContext );
}

void Tr2PostProcessRenderer::RenderGenericEffect( const Tr2TextureAL& dest, const Tr2TextureAL& src, Tr2RenderContext& renderContext, Tr2PPGenericEffectPtr genericEffect )
{
	Tr2EffectPtr effect = genericEffect->GetEffect();
	if( effect != nullptr )
	{
		GPU_REGION( renderContext, "GenericEffect" );
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

		TEMP_PARAM( effect, "Blit", src );
		DrawInto( dest, Tr2LoadAction::DONT_CARE, effect, renderContext );
	}
}

void Tr2PostProcessRenderer::RenderDepthOfField( const Tr2TextureAL& dest, Tr2GpuResourcePool& gpuResourcePool, Tr2RenderContext& renderContext, Tr2PPDepthOfFieldEffect* depthOfField, bool temporal, float upscalingAmount )
{
	GPU_REGION( renderContext, "DepthOfField" );
	{
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );


		BlueSharedString shape = depthOfField->GetBokehShapeString();
		{
			Tr2GpuResourcePool::Texture coc;

			auto cocSize = TextureSize2D( dest.GetDesc() ) * depthOfField->m_cocScale;
			m_depthOfFieldCoCShader->SetParameter( MEMOIZED_STRING( "FocalInfo" ), Vector4( depthOfField->m_focalDistance, depthOfField->m_focalLength, depthOfField->m_scale, 0.0 ) );
			m_depthOfFieldCoCShader->SetOption( MEMOIZED_STRING( "COC_OUTPUT_CHANNEL_COUNT" ), depthOfField->m_foregroundBlurNeeded ? MEMOIZED_STRING( "COC_OUTPUT_CHANNEL_COUNT_2" ) : MEMOIZED_STRING( "COC_OUTPUT_CHANNEL_COUNT_1" ) );

			if( !depthOfField->m_foregroundBlurNeeded )
			{
				GPU_REGION( renderContext, "CoC" );

				coc = gpuResourcePool.GetTempTexture( "CoC", cocSize, Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM, RENDER_TARGET );
				DrawInto( coc, Tr2LoadAction::DONT_CARE, m_depthOfFieldCoCShader, renderContext );
			}
			else
			{
				GPU_REGION( renderContext, "CoC" );
				auto coc2 = gpuResourcePool.GetTempTexture( "CoC", cocSize, Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_UNORM, RENDER_TARGET );

				DrawInto( coc2, Tr2LoadAction::DONT_CARE, m_depthOfFieldCoCShader, renderContext );
				{
					GPU_REGION( renderContext, "CoC Blur" );

					auto blurContext = PostProcessBlur::CreateBlurContext( PostProcessBlur::BT_Big,
																		   PostProcessBlur::BC_r,
																		   PostProcessBlur::BP_Maximum,
																		   PostProcessBlur::BF_MaxOfAllChannels );
					coc = Blur( coc2, gpuResourcePool, renderContext, blurContext );
				}
			}

			float adjustedScale = depthOfField->m_scale / upscalingAmount;

			auto blur = gpuResourcePool.GetTempTexture( "Bokeh Blend", dest.GetWidth(), dest.GetHeight(), dest.GetFormat(), RENDER_TARGET );

			if( depthOfField->m_useTAAFriendlyBokeh )
			{

				float GOLDEN_ANGLE = TRI_PI * ( 3.0f - sqrt( 5.0f ) );
				float angle = 0;
				float samplesPerPixel = 2.0 / 5.0;

				if( temporal )
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
					m_depthOfFieldBokehTAAShader->SetOption( MEMOIZED_STRING( "BOKEH_SHAPE" ), shape );
					TEMP_PARAM( m_depthOfFieldBokehTAAShader, "BlitCurrent", dest );
					TEMP_PARAM( m_depthOfFieldBokehTAAShader, "CoCMap", coc );
					m_depthOfFieldBokehTAAShader->SetParameter( MEMOIZED_STRING( "BokehInfo" ), Vector4( adjustedScale, angle, samplesPerPixel, 0.0f ) );
					DrawInto( blur, Tr2LoadAction::DONT_CARE, m_depthOfFieldBokehTAAShader, renderContext );
				}
				{

					GPU_REGION( renderContext, "Copy back" );
					DrawInto( dest, Tr2LoadAction::DONT_CARE, blur, renderContext );
				}
			}
			else
			{
				{
					GPU_REGION( renderContext, "Bokeh Blend" );
					auto blitCurrent = TempParameter( m_depthOfFieldBokehBlurShader, MEMOIZED_STRING( "BlitCurrent" ), dest );
					auto cocMap = TempParameter( m_depthOfFieldBokehBlurShader, MEMOIZED_STRING( "CoCMap" ), coc );
					m_depthOfFieldBokehBlurShader->SetParameter( MEMOIZED_STRING( "BokehInfo" ), Vector4( adjustedScale, 0.0f, 0.0f, 0.0f ) );
					m_depthOfFieldBokehBlurShader->SetOption( MEMOIZED_STRING( "BOKEH_SHAPE" ), shape );
					DrawInto( blur, Tr2LoadAction::DONT_CARE, m_depthOfFieldBokehBlurShader, renderContext );
				}
				{
					GPU_REGION( renderContext, "Bokeh Fill" );
					auto blitCurrent = TempParameter( m_depthOfFieldBokehFillShader, MEMOIZED_STRING( "BlitCurrent" ), blur );
					auto cocMap = TempParameter( m_depthOfFieldBokehFillShader, MEMOIZED_STRING( "CoCMap" ), coc );
					m_depthOfFieldBokehFillShader->SetParameter( MEMOIZED_STRING( "BokehInfo" ), Vector4( adjustedScale, 0.0f, 0.0f, 0.0f ) );
					m_depthOfFieldBokehFillShader->SetOption( MEMOIZED_STRING( "BOKEH_SHAPE" ), shape );
					DrawInto( dest, Tr2LoadAction::DONT_CARE, m_depthOfFieldBokehFillShader, renderContext );
				}
			}
		}
	}
}

Tr2GpuResourcePool::Buffer Tr2PostProcessRenderer::GetExposureBuffer( Tr2GpuResourcePool& gpuResourcePool ) const
{
	const float zeroes[8] = {};
	return gpuResourcePool.GetPersistentBuffer(
		"Exposure Buffer",
		Tr2BufferDescriptionAL( Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT, 8, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ ),
		zeroes );
}

Tr2GpuResourcePool::Texture Tr2PostProcessRenderer::GetBlackTexture( Tr2GpuResourcePool& gpuResourcePool ) const
{
	const uint32_t blackColor[4 * 4] = {};
	Tr2SubresourceData initData = { blackColor, 4 * sizeof( uint32_t ), 4 * 4 * sizeof( uint32_t ) };
	return gpuResourcePool.GetPersistentTexture( "Black", 4, 4, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2GpuUsage::SHADER_RESOURCE, &initData );
}
