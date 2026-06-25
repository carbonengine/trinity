// Copyright © 2024 CCP ehf.

#include "StdAfx.h"
#include "Tr2PostProcessAttributes.h"
#include "Tr2PostProcessEnums.h"
#include "ITr2PostProcessOwner.h"
#include "Tr2PostProcess2.h"
#include <algorithm>

using namespace PriorityBlend;

Tr2PostProcessAttributes::Tr2PostProcessAttributes( IRoot* lockobj ) :
	intensity( 0.0f ),
	priority( PostProcessEnums::Priority::MEDIUM_PRIORITY ),
	signalLossIntensity( Attribute( 0.0f ) ),
	bloomBrightness( Attribute( 0.0f ) ),
	bloomLuminanceThreshold( Attribute( 0.0f ) ),
	bloomLuminanceScale( Attribute( 0.0f ) ),
	grimeIntensity( Attribute( 0.0f ) ),
	grimePath( Attribute( BlueSharedString( "" ) ) ),
	exposureAdjustment( Attribute( 0.0f ) ),
	filmGrainIntensity( Attribute( 0.0f ) ),
	filmGrainSize( Attribute( 0.0f ) ),
	filmGrainDensity( Attribute( 0.0f ) ),
	filmGrainContrast( Attribute( 0.0f ) ),
	filmGrainBrightnessModifier( Attribute( 0.0f ) ),
	filmGrainColored( Attribute( false ) ),
	filmGrainColorAmount( Attribute( 0.0f ) ),
	saturation( Attribute( 0.0f ) ),
	fadeIntensity( Attribute( 0.0f ) ),
	fadeColor( Attribute( Color( 0.0f, 0.0f, 0.0f, 1.0f ) ) ),
	lutIntensity( Attribute( 0.0f ) ),
	lutPath( Attribute( BlueSharedString( "" ) ) ),
	prioritizedLuts( std::set<std::pair<float, BlueSharedString>>() ),
	vignetteIntensity( Attribute( 0.0f ) ),
	vignetteOpacity( Attribute( 0.0f ) ),
	vignetteColor( Attribute( Color( 1.0f, 1.0f, 1.0f, 1.0f ) ) ),
	vignetteDetail1Size( Attribute( Vector2( 16.0, 16.0 ) ) ),
	vignetteDetail1Scroll( Attribute( Vector2( 0, 0 ) ) ),
	vignetteDetail2Size( Attribute( Vector2( 16.0, 16.0 ) ) ),
	vignetteDetail2Scroll( Attribute( Vector2( 0, 0 ) ) ),
	vignetteShapePath( Attribute( BlueSharedString( "" ) ) ),
	vignetteDetailPath( Attribute( BlueSharedString( "" ) ) ),
	vignetteSineFrequency( Attribute( 0.0f ) ),
	vignetteMinSineFrequency( Attribute( 0.0f ) ),
	vignetteMaxSineFrequency( Attribute( 0.0f ) ),
	depthOfFieldScale( Attribute( 0.0f ) ),
	depthOfFieldFocalDistance( Attribute( 0.0f ) ),
	depthOfFieldFocalLength( Attribute( 0.0f ) ),
	depthOfFieldShape( Attribute( Tr2Bokeh::Disk ) ),
	depthOfFieldForegroundBlurNeeded( Attribute( false ) )
{
}

Tr2PostProcessAttributes::~Tr2PostProcessAttributes()
{
}

namespace
{

template <typename T>
const char* GetAttributeName( T Tr2PostProcessAttributes::* attribute )
{
	auto a = reinterpret_cast<uintptr_t>( &reinterpret_cast<uint8_t&>( reinterpret_cast<Tr2PostProcessAttributes*>( 0 )->*attribute ) );
	auto rtti = Tr2PostProcessAttributes::ClassType_();
	for( const Be::VarEntry* entry = rtti->mMemberTable; entry->mName; entry++ )
	{
		if( !entry->mGetProperty && entry->mOffset == a )
		{
			return entry->mName;
		}
	}
	return "unknown";
}


template <typename T, typename Accumulator = typename DefaultAccumulator<T>::Type>
typename Accumulator::ResultType Accumulate( PriorityBlend::Attribute<T> Tr2PostProcessAttributes::* attr, const std::vector<Tr2PostProcessAttributes*>& sources, AttributesDebugObserver<Tr2PostProcessAttributes>* observer, Accumulator accumulator = {} )
{
	return PriorityBlend::Accumulate( attr, sources, observer, GetAttributeName( attr ), accumulator );
}

}



void Tr2PostProcessAttributes::MergeInto( Tr2PostProcess2& postprocess, std::vector<Tr2PostProcessAttributes*>& sources, AttributesDebugObserver<Tr2PostProcessAttributes>* debugObserver )
{
	auto signalLossIntensity = Accumulate( &Tr2PostProcessAttributes::signalLossIntensity, sources, debugObserver );
	auto bloomBrightness = Accumulate( &Tr2PostProcessAttributes::bloomBrightness, sources, debugObserver );
	auto bloomLuminanceThreshold = Accumulate( &Tr2PostProcessAttributes::bloomLuminanceThreshold, sources, debugObserver );
	auto bloomLuminanceScale = Accumulate( &Tr2PostProcessAttributes::bloomLuminanceScale, sources, debugObserver );
	auto bloomSizeScale = Accumulate( &Tr2PostProcessAttributes::bloomSizeScale, sources, debugObserver );
	auto bloomDirectionalWeight = Accumulate( &Tr2PostProcessAttributes::bloomDirectionalWeight, sources, debugObserver );
	auto bloomStepSize1 = Accumulate( &Tr2PostProcessAttributes::bloomStepSize1, sources, debugObserver );
	auto bloomStepSize2 = Accumulate( &Tr2PostProcessAttributes::bloomStepSize2, sources, debugObserver );
	auto bloomStepSize3 = Accumulate( &Tr2PostProcessAttributes::bloomStepSize3, sources, debugObserver );
	auto bloomStepSize4 = Accumulate( &Tr2PostProcessAttributes::bloomStepSize4, sources, debugObserver );
	auto bloomStepSize5 = Accumulate( &Tr2PostProcessAttributes::bloomStepSize5, sources, debugObserver );
	auto bloomStepSize6 = Accumulate( &Tr2PostProcessAttributes::bloomStepSize6, sources, debugObserver );
	auto bloomStepTint1 = Accumulate( &Tr2PostProcessAttributes::bloomStepTint1, sources, debugObserver );
	auto bloomStepTint2 = Accumulate( &Tr2PostProcessAttributes::bloomStepTint2, sources, debugObserver );
	auto bloomStepTint3 = Accumulate( &Tr2PostProcessAttributes::bloomStepTint3, sources, debugObserver );
	auto bloomStepTint4 = Accumulate( &Tr2PostProcessAttributes::bloomStepTint4, sources, debugObserver );
	auto bloomStepTint5 = Accumulate( &Tr2PostProcessAttributes::bloomStepTint5, sources, debugObserver );
	auto bloomStepTint6 = Accumulate( &Tr2PostProcessAttributes::bloomStepTint6, sources, debugObserver );

	auto grimeIntensity = Accumulate( &Tr2PostProcessAttributes::grimeIntensity, sources, debugObserver );
	auto grimePath = Accumulate( &Tr2PostProcessAttributes::grimePath, sources, debugObserver );
	auto exposureAdjustment = Accumulate( &Tr2PostProcessAttributes::exposureAdjustment, sources, debugObserver );
	auto filmGrainColored = Accumulate( &Tr2PostProcessAttributes::filmGrainColored, sources, debugObserver );
	auto filmGrainColorAmount = Accumulate( &Tr2PostProcessAttributes::filmGrainColorAmount, sources, debugObserver );
	auto filmGrainIntensity = Accumulate( &Tr2PostProcessAttributes::filmGrainIntensity, sources, debugObserver );
	auto filmGrainSize = Accumulate( &Tr2PostProcessAttributes::filmGrainSize, sources, debugObserver );
	auto filmGrainDensity = Accumulate( &Tr2PostProcessAttributes::filmGrainDensity, sources, debugObserver );
	auto filmGrainContrast = Accumulate( &Tr2PostProcessAttributes::filmGrainContrast, sources, debugObserver );
	auto filmGrainBrightnessModifier = Accumulate( &Tr2PostProcessAttributes::filmGrainBrightnessModifier, sources, debugObserver );
	auto saturation = Accumulate( &Tr2PostProcessAttributes::saturation, sources, debugObserver );
	auto fadeIntensity = Accumulate( &Tr2PostProcessAttributes::fadeIntensity, sources, debugObserver );
	auto fadeColor = Accumulate( &Tr2PostProcessAttributes::fadeColor, sources, debugObserver );
	auto lutIntensity = Accumulate( &Tr2PostProcessAttributes::lutIntensity, sources, debugObserver );
	auto lutPath = Accumulate( &Tr2PostProcessAttributes::lutPath, sources, debugObserver, MaxNWeightsAccumulator<BlueSharedString, 4>() );
	auto vignetteIntensity = Accumulate( &Tr2PostProcessAttributes::vignetteIntensity, sources, debugObserver );
	auto vignetteOpacity = Accumulate( &Tr2PostProcessAttributes::vignetteOpacity, sources, debugObserver );
	auto vignetteColor = Accumulate( &Tr2PostProcessAttributes::vignetteColor, sources, debugObserver );
	auto vignetteDetail1Size = Accumulate( &Tr2PostProcessAttributes::vignetteDetail1Size, sources, debugObserver );
	auto vignetteDetail1Scroll = Accumulate( &Tr2PostProcessAttributes::vignetteDetail1Scroll, sources, debugObserver );
	auto vignetteDetail2Size = Accumulate( &Tr2PostProcessAttributes::vignetteDetail2Size, sources, debugObserver );
	auto vignetteDetail2Scroll = Accumulate( &Tr2PostProcessAttributes::vignetteDetail2Scroll, sources, debugObserver );
	auto vignetteShapePath = Accumulate( &Tr2PostProcessAttributes::vignetteShapePath, sources, debugObserver );
	auto vignetteDetailPath = Accumulate( &Tr2PostProcessAttributes::vignetteDetailPath, sources, debugObserver );
	auto vignetteSineFrequency = Accumulate( &Tr2PostProcessAttributes::vignetteSineFrequency, sources, debugObserver );
	auto vignetteMinSineFrequency = Accumulate( &Tr2PostProcessAttributes::vignetteMinSineFrequency, sources, debugObserver );
	auto vignetteMaxSineFrequency = Accumulate( &Tr2PostProcessAttributes::vignetteMaxSineFrequency, sources, debugObserver );
	auto depthOfFieldScale = Accumulate( &Tr2PostProcessAttributes::depthOfFieldScale, sources, debugObserver );
	auto depthOfFieldFocalDistance = Accumulate( &Tr2PostProcessAttributes::depthOfFieldFocalDistance, sources, debugObserver );
	auto depthOfFieldFocalLength = Accumulate( &Tr2PostProcessAttributes::depthOfFieldFocalLength, sources, debugObserver );
	auto depthOfFieldShape = Accumulate( &Tr2PostProcessAttributes::depthOfFieldShape, sources, debugObserver, MaxWeightAccumulator<Tr2Bokeh::Shape>() );
	auto depthOfFieldForegroundBlurNeeded = Accumulate( &Tr2PostProcessAttributes::depthOfFieldForegroundBlurNeeded, sources, debugObserver );
	auto whiteTemperature = Accumulate( &Tr2PostProcessAttributes::whiteTemperature, sources, debugObserver );
	auto whiteTint = Accumulate( &Tr2PostProcessAttributes::whiteTint, sources, debugObserver );
	auto colorSaturation = Accumulate( &Tr2PostProcessAttributes::colorSaturation, sources, debugObserver );
	auto colorContrast = Accumulate( &Tr2PostProcessAttributes::colorContrast, sources, debugObserver );
	auto colorGamma = Accumulate( &Tr2PostProcessAttributes::colorGamma, sources, debugObserver );
	auto colorGain = Accumulate( &Tr2PostProcessAttributes::colorGain, sources, debugObserver );
	auto colorOffset = Accumulate( &Tr2PostProcessAttributes::colorOffset, sources, debugObserver );

	postprocess.SetBloom( nullptr );
	postprocess.SetDesaturate( nullptr );
	postprocess.SetFade( nullptr );
	postprocess.SetFilmGrain( nullptr );
	postprocess.SetSignalLoss( nullptr );
	postprocess.SetVignette( nullptr );
	postprocess.SetDepthOfField( nullptr );
	postprocess.SetColorCorrection( nullptr );
	postprocess.ClearLuts();

	if( signalLossIntensity > 0 )
	{
		Tr2PPSignalLossEffectPtr signalLossEffect;
		signalLossEffect.CreateInstance();
		signalLossEffect->m_strength = signalLossIntensity;
		postprocess.SetSignalLoss( signalLossEffect );
	}
	if( bloomBrightness > 0 )
	{
		Tr2PPBloomEffectPtr bloomEffect;
		bloomEffect.CreateInstance();
		bloomEffect->m_bloomBrightness = bloomBrightness;
		bloomEffect->m_luminanceThreshold = bloomLuminanceThreshold;
		bloomEffect->m_luminanceScale = bloomLuminanceScale;

		bloomEffect->m_sizeScale = bloomSizeScale;
		bloomEffect->m_directionalWeight = bloomDirectionalWeight;
		bloomEffect->m_stepSizes = { bloomStepSize1, bloomStepSize2, bloomStepSize3, bloomStepSize4, bloomStepSize5, bloomStepSize6 };
		bloomEffect->m_stepTints = { bloomStepTint1, bloomStepTint2, bloomStepTint3, bloomStepTint4, bloomStepTint5, bloomStepTint6 };

		postprocess.SetBloom( bloomEffect );
	}
	if( grimeIntensity > 0 )
	{
		Tr2PPBloomEffectPtr bloomEffect;
		if( postprocess.GetBloomIfAvailable() == nullptr )
		{
			bloomEffect.CreateInstance();
		}
		else
		{
			bloomEffect = postprocess.GetBloomIfAvailable();
		}
		bloomEffect->m_grimeWeight = grimeIntensity;
		bloomEffect->m_grimePath = grimePath;
		postprocess.SetBloom( bloomEffect );
	}
	if( filmGrainIntensity > 0 )
	{
		Tr2PPFilmGrainEffectPtr filmGrainEffect;
		filmGrainEffect.CreateInstance();
		filmGrainEffect->m_intensity = filmGrainIntensity;
		filmGrainEffect->m_grainSize = filmGrainSize;
		filmGrainEffect->m_grainDensity = filmGrainDensity;
		filmGrainEffect->m_grainContrast = filmGrainContrast;
		filmGrainEffect->m_brightnessModifier = filmGrainBrightnessModifier;
		filmGrainEffect->m_colored = filmGrainColored;
		filmGrainEffect->m_colorAmount = filmGrainColorAmount;
		postprocess.SetFilmGrain( filmGrainEffect );
	}

	if( saturation != 0 )
	{
		Tr2PPDesaturateEffectPtr desaturationEffect;
		desaturationEffect.CreateInstance();
		desaturationEffect->m_intensity = saturation + 1.0f;
		postprocess.SetDesaturate( desaturationEffect );
	}

	if( fadeIntensity > 0 )
	{
		Tr2PPFadeEffectPtr fadeEffect;
		fadeEffect.CreateInstance();
		fadeEffect->m_intensity = fadeIntensity;
		fadeEffect->m_color = fadeColor;
		postprocess.SetFade( fadeEffect );
	}

	size_t count = 0;
	for( const auto& lut : lutPath.values )
	{
		if( count == lutPath.count )
		{
			break;
		}
		Tr2PPLutEffectPtr lutEffect;
		lutEffect.CreateInstance();
		lutEffect->m_influence = lut.weight * lutIntensity;
		lutEffect->m_path = lut.value;

		postprocess.AddLut( lutEffect );
		++count;
	}

	if( vignetteIntensity > 0 )
	{
		Tr2PPVignetteEffectPtr vignetteEffect;
		vignetteEffect.CreateInstance();
		vignetteEffect->m_intensity = vignetteIntensity;
		vignetteEffect->m_opacity = vignetteOpacity;
		vignetteEffect->m_color = vignetteColor;
		vignetteEffect->m_detail1Size = vignetteDetail1Size;
		vignetteEffect->m_detail1Scroll = vignetteDetail1Scroll;
		vignetteEffect->m_detail2Size = vignetteDetail2Size;
		vignetteEffect->m_detail2Scroll = vignetteDetail2Scroll;
		vignetteEffect->m_shapePath = vignetteShapePath;
		vignetteEffect->m_detailPath = vignetteDetailPath;
		vignetteEffect->m_sineFrequency = vignetteSineFrequency;
		vignetteEffect->m_sineMinimum = vignetteMinSineFrequency;
		vignetteEffect->m_sineMaximum = vignetteMaxSineFrequency;

		postprocess.SetVignette( vignetteEffect );
	}

	if( depthOfFieldScale > 0 )
	{
		Tr2PPDepthOfFieldEffectPtr dofEffect;
		dofEffect.CreateInstance();
		dofEffect->m_scale = depthOfFieldScale;
		dofEffect->m_cocScale = 1.0f;
		dofEffect->m_focalDistance = depthOfFieldFocalDistance;
		dofEffect->m_focalLength = depthOfFieldFocalLength;
		dofEffect->m_bokehShape = depthOfFieldShape;
		dofEffect->m_foregroundBlurNeeded = depthOfFieldForegroundBlurNeeded;
		postprocess.SetDepthOfField( dofEffect );
	}

	postprocess.m_exposureAdjustment = exposureAdjustment;

	// All color correction attributes are enabled because of weird interpolation issues when entering the post processing volumes.
	// Due to that, this check is always going to be true. So we cannot _really_ turn color correction off...
	if( whiteTemperature > 0 )
	{
		Tr2PPColorCorrectionEffectPtr colorCorrectionEffect;
		colorCorrectionEffect.CreateInstance();
		colorCorrectionEffect->m_whiteTemperature = whiteTemperature;
		colorCorrectionEffect->m_whiteTint = whiteTint;
		colorCorrectionEffect->m_colorSaturation = colorSaturation;
		colorCorrectionEffect->m_colorContrast = colorContrast;
		colorCorrectionEffect->m_colorGamma = colorGamma;
		colorCorrectionEffect->m_colorGain = colorGain;
		colorCorrectionEffect->m_colorOffset = colorOffset;
		postprocess.SetColorCorrection( colorCorrectionEffect );
	}
}

void Tr2PostProcessAttributes::Reset()
{
	intensity = 0.0f;
	priority = PostProcessEnums::Priority::MEDIUM_PRIORITY;
	signalLossIntensity = Attribute( 0.0f );
	bloomBrightness = Attribute( 0.0f );
	bloomLuminanceThreshold = Attribute( 0.0f );
	bloomLuminanceScale = Attribute( 0.0f );
	bloomSizeScale = Bloom::DEFAULT_BLOOM_SIZE_SCALE;
	bloomDirectionalWeight = Bloom::DEFAULT_BLOOM_DIRECTIONAL_WEIGHT;
	bloomStepSize1 = Bloom::DEFAULT_BLOOM_STEP_1_SIZE;
	bloomStepSize2 = Bloom::DEFAULT_BLOOM_STEP_2_SIZE;
	bloomStepSize3 = Bloom::DEFAULT_BLOOM_STEP_3_SIZE;
	bloomStepSize4 = Bloom::DEFAULT_BLOOM_STEP_4_SIZE;
	bloomStepSize5 = Bloom::DEFAULT_BLOOM_STEP_5_SIZE;
	bloomStepSize6 = Bloom::DEFAULT_BLOOM_STEP_6_SIZE;
	bloomStepTint1 = Bloom::DEFAULT_BLOOM_STEP_1_TINT;
	bloomStepTint2 = Bloom::DEFAULT_BLOOM_STEP_2_TINT;
	bloomStepTint3 = Bloom::DEFAULT_BLOOM_STEP_3_TINT;
	bloomStepTint4 = Bloom::DEFAULT_BLOOM_STEP_4_TINT;
	bloomStepTint5 = Bloom::DEFAULT_BLOOM_STEP_5_TINT;
	bloomStepTint6 = Bloom::DEFAULT_BLOOM_STEP_6_TINT;

	grimeIntensity = Attribute( 0.0f );
	grimePath = Attribute( BlueSharedString( "" ) );
	exposureAdjustment = Attribute( 0.0f );
	filmGrainIntensity = Attribute( 0.0f );
	filmGrainSize = Attribute( 0.0f );
	filmGrainDensity = Attribute( 0.0f );
	filmGrainContrast = Attribute( 0.0f );
	filmGrainBrightnessModifier = Attribute( 0.0f );
	filmGrainColored = Attribute( false );
	filmGrainColorAmount = Attribute( 0.0f );
	saturation = Attribute( 0.0f );
	fadeIntensity = Attribute( 0.0f );
	fadeColor = Attribute( Color( 0.0f, 0.0f, 0.0f, 1.0f ) );
	lutIntensity = Attribute( 0.0f );
	lutPath = Attribute( BlueSharedString( "" ) );
	prioritizedLuts.clear();
	vignetteIntensity = Attribute( 0.0f );
	vignetteOpacity = Attribute( 0.0f );
	vignetteColor = Attribute( Color( 1.0f, 1.0f, 1.0f, 1.0f ) );
	vignetteDetail1Size = Attribute( Vector2( 16.0, 16.0 ) );
	vignetteDetail1Scroll = Attribute( Vector2( 0, 0 ) );
	vignetteDetail2Size = Attribute( Vector2( 16.0, 16.0 ) );
	vignetteDetail2Scroll = Attribute( Vector2( 0, 0 ) );
	vignetteShapePath = Attribute( BlueSharedString( "" ) );
	vignetteDetailPath = Attribute( BlueSharedString( "" ) );
	vignetteSineFrequency = Attribute( 0.0f );
	vignetteMinSineFrequency = Attribute( 0.0f );
	vignetteMaxSineFrequency = Attribute( 0.0f );
	depthOfFieldScale = Attribute( 0.0f );
	depthOfFieldFocalDistance = Attribute( 0.0f );
	depthOfFieldFocalLength = Attribute( 0.0f );
	depthOfFieldShape = Attribute( Tr2Bokeh::Disk );

	// We enable all color correction options because otherwise it would interpolate from zero to whatever the
	// postprocess volume has set, giving us weird results when entering the volume.
	whiteTemperature = Attribute( 6500.0f, true );
	whiteTint = Attribute( 0.0f, true );
	colorSaturation = Attribute( 1.0f, true );
	colorContrast = Attribute( 1.0f, true );
	colorGamma = Attribute( 1.0f, true );
	colorGain = Attribute( Vector3( 1.0f, 1.0f, 1.0f ), true );
	colorOffset = Attribute( Vector3( 0.0f, 0.0f, 0.0f ), true );
}

void Tr2PostProcessAttributes::FromPostProcess( Tr2PostProcess2* postProcess, PostProcessEnums::Priority inPriority, float inIntensity )
{
	// important to nuke any old values
	Reset();

	intensity = inIntensity;
	priority = inPriority;

	if( !postProcess )
	{
		return;
	}

	if( auto signalLoss = postProcess->GetSignalLossIfAvailable() )
	{
		signalLossIntensity = Attribute( signalLoss->m_strength, true );
	}

	if( auto bloom = postProcess->GetBloomIfAvailable() )
	{
		bloomBrightness = Attribute( bloom->m_bloomBrightness, true );
		bloomLuminanceScale = Attribute( bloom->m_luminanceScale, true );
		bloomLuminanceThreshold = Attribute( bloom->m_luminanceThreshold, true );
		grimeIntensity = Attribute( bloom->m_grimeWeight, true );
		grimePath = Attribute( bloom->m_grimePath, true );

		bloomSizeScale = Attribute( bloom->m_sizeScale, true );
		bloomDirectionalWeight = Attribute( bloom->m_directionalWeight, true );
		bloomStepSize1 = Attribute( bloom->m_stepSizes[0], true );
		bloomStepSize2 = Attribute( bloom->m_stepSizes[1], true );
		bloomStepSize3 = Attribute( bloom->m_stepSizes[2], true );
		bloomStepSize4 = Attribute( bloom->m_stepSizes[3], true );
		bloomStepSize5 = Attribute( bloom->m_stepSizes[4], true );
		bloomStepSize6 = Attribute( bloom->m_stepSizes[5], true );
		bloomStepTint1 = Attribute( bloom->m_stepTints[0], true );
		bloomStepTint2 = Attribute( bloom->m_stepTints[1], true );
		bloomStepTint3 = Attribute( bloom->m_stepTints[2], true );
		bloomStepTint4 = Attribute( bloom->m_stepTints[3], true );
		bloomStepTint5 = Attribute( bloom->m_stepTints[4], true );
		bloomStepTint6 = Attribute( bloom->m_stepTints[5], true );
	}
	if( auto filmGrain = postProcess->GetFilmGrainIfAvailable() )
	{
		filmGrainIntensity = Attribute( filmGrain->m_intensity, true );
		filmGrainSize = Attribute( filmGrain->m_grainSize, true );
		filmGrainDensity = Attribute( filmGrain->m_grainDensity, true );
		filmGrainContrast = Attribute( filmGrain->m_grainContrast, true );
		filmGrainBrightnessModifier = Attribute( filmGrain->m_brightnessModifier, true );
		filmGrainColored = Attribute( filmGrain->m_colored, true );
		filmGrainColorAmount = Attribute( filmGrain->m_colorAmount, true );
	}
	if( auto desaturate = postProcess->GetDesaturateIfAvailable() )
	{
		// negative is desaturation, positive is saturation, so move the zero point to 0.0 from 1.0
		saturation = Attribute( desaturate->m_intensity - 1.0f, true );
	}
	if( auto fade = postProcess->GetFadeIfAvailable() )
	{
		fadeIntensity = Attribute( fade->m_intensity, true );
		fadeColor = Attribute( fade->m_color, true );
	}
	if( auto vignette = postProcess->GetVignetteIfAvailable() )
	{
		vignetteIntensity = Attribute( vignette->m_intensity, true );
		vignetteOpacity = Attribute( vignette->m_opacity, true );
		vignetteColor = Attribute( vignette->m_color, true );
		vignetteDetail1Size = Attribute( vignette->m_detail1Size, true );
		vignetteDetail1Scroll = Attribute( vignette->m_detail1Scroll, true );
		vignetteDetail2Size = Attribute( vignette->m_detail2Size, true );
		vignetteDetail2Scroll = Attribute( vignette->m_detail2Scroll, true );
		vignetteShapePath = Attribute( vignette->m_shapePath, true );
		vignetteDetailPath = Attribute( vignette->m_detailPath, true );
		vignetteSineFrequency = Attribute( vignette->m_sineFrequency, true );
		vignetteMinSineFrequency = Attribute( vignette->m_sineMinimum, true );
		vignetteMaxSineFrequency = Attribute( vignette->m_sineMaximum, true );
	}
	if( auto depthOfField = postProcess->GetDepthOfFieldIfAvailable() )
	{
		depthOfFieldScale = Attribute( depthOfField->m_scale, true );
		depthOfFieldFocalDistance = Attribute( depthOfField->m_focalDistance, true );
		depthOfFieldFocalLength = Attribute( depthOfField->m_focalLength, true );
		depthOfFieldShape = Attribute( depthOfField->m_bokehShape, true );
	}

	auto luts = std::vector<const Tr2PPLutEffect*>();
	postProcess->GetAvilableSortedLuts( luts );
	for( auto lut : luts )
	{
		// just get the maximum lut intensity
		lutIntensity = Attribute( lut->m_influence, true );
		lutPath = Attribute( lut->m_path, true );
		break;
	}

	if( auto colorCorrection = postProcess->GetColorCorrectionIfAvailable() )
	{
		whiteTemperature = Attribute( colorCorrection->m_whiteTemperature, true );
		whiteTint = Attribute( colorCorrection->m_whiteTint, true );
		colorSaturation = Attribute( colorCorrection->m_colorSaturation, true );
		colorContrast = Attribute( colorCorrection->m_colorContrast, true );
		colorGamma = Attribute( colorCorrection->m_colorGamma, true );
		colorGain = Attribute( colorCorrection->m_colorGain, true );
		colorOffset = Attribute( colorCorrection->m_colorOffset, true );
	}
}