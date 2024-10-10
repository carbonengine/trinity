////////////////////////////////////////////////////////////////////////////////
//
// Created:		September 2024
// Copyright:	CCP 2024
//

#include "StdAfx.h"
#include "Tr2PostProcessAttributes.h"
#include "Tr2PostProcessEnums.h"
#include "ITr2PostProcessOwner.h"
#include "Tr2PostProcess2.h"
#include <algorithm>

using namespace PostProcess;

template<>
void Attribute<Tr2Bokeh::Shape>::Blend( const Attribute<Tr2Bokeh::Shape>& other, float intensity, float weight )
{
	if( other.enabled )
	{
		if( currentIntensity < intensity )
		{
			value = other.value;
			currentIntensity = intensity;
		}
		enabled = true;
	}
}

template <>
void Attribute<BlueSharedString>::Blend( const Attribute<BlueSharedString>& other, float intensity, float weight )
{
	if( other.enabled )
	{
		if( currentIntensity < intensity )
		{
			value = other.value;
			currentIntensity = intensity;
		}
		enabled = true;
	}
}

template <>
void Attribute<bool>::Blend( const Attribute<bool>& other, float intensity, float weight )
{
	if( other.enabled )
	{
		if( currentIntensity < intensity )
		{
			value = other.value;
			currentIntensity = intensity;
		}
		enabled = true;
	}
}

WeightRow::WeightRow() :
	type( PostProcessEnums::POST_PROCESS_ATTRIBUTE_TYPE_COUNT ),
	intensity( 0.0f ),
	totalIntensity( 0.0f )
{
}

WeightRow::WeightRow( PostProcessEnums::AttributeType type, float intensity ) :
	type( type ),
	intensity( intensity ),
	totalIntensity( 0.0f )
{
}

WeightRow::WeightRow( PostProcessEnums::AttributeType type, float intensity, float totalIntensity ) :
	type( type ),
	intensity( intensity ),
	totalIntensity( totalIntensity )
{
}

WeightRow::WeightRow( const WeightRow& other ) :
	type( other.type ),
	intensity( other.intensity ),
	totalIntensity( other.totalIntensity )
{
}

namespace
{

	BlueStructureDefinition WeightRowDef[] = {
		{ "intensity", Be::FLOAT32_1, 0 },
		{ "totalIntensity", Be::FLOAT32_1, 4 },
		{ "type", Be::UINT32_1, 8, PostProcessEnums::Tr2PostProcessAttributeTypeChooser },
		{ 0 }
	};

	const WeightRow s_defaultWeight = WeightRow();
}

Tr2PostProcessAttributes::Tr2PostProcessAttributes( IRoot* lockobj ) :
	PARENTLOCK( m_weights ),
	PARENTLOCK( m_sources ),
	intensity(0.0f),
	priority( PostProcessEnums::Priority::MEDIUM_PRIORITY ),
	signalLossIntensity( Attribute( 0.0f ) ),
	bloomBrightness( Attribute( 0.0f ) ),
	bloomLuminanceThreshold( Attribute( 0.0f ) ),
	bloomLuminanceScale( Attribute( 0.0f ) ),
	grimeIntensity( Attribute( 0.0f ) ),
	grimePath( Attribute( BlueSharedString( "" ) ) ),
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
	vignetteDetail2Scroll( Attribute(Vector2( 0, 0 ) ) ),
	vignetteShapePath( Attribute( BlueSharedString( "" ) ) ),
	vignetteDetailPath( Attribute( BlueSharedString( "" ) ) ),
	vignetteSineFrequency( Attribute( 0.0f ) ),
	vignetteMinSineFrequency( Attribute( 0.0f ) ),
	vignetteMaxSineFrequency( Attribute( 0.0f ) ),
	depthOfFieldScale( Attribute( 0.0f ) ),
	depthOfFieldFocalDistance( Attribute( 0.0f ) ),
	depthOfFieldFocalLength( Attribute( 0.0f ) ),
	depthOfFieldShape( Attribute( Tr2Bokeh::Disk ) ),
	m_owner( nullptr )
{
	m_weights.SetStructureDefinition( WeightRowDef );
	m_weights.SetDefaultValue( &s_defaultWeight );
}

Tr2PostProcessAttributes::~Tr2PostProcessAttributes()
{
}

void Tr2PostProcessAttributes::SetOwner( IRoot* owner )
{
	m_owner = owner;
}

void Tr2PostProcessAttributes::Reset()
{
	intensity = 0.0f;
	priority = PostProcessEnums::Priority::MEDIUM_PRIORITY;
	signalLossIntensity = Attribute( 0.0f );
	bloomBrightness = Attribute( 0.0f );
	bloomLuminanceThreshold = Attribute( 0.0f );
	bloomLuminanceScale = Attribute( 0.0f );
	grimeIntensity = Attribute( 0.0f );
	grimePath = Attribute( BlueSharedString( "" ) );
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

	whiteTemperature = 0;
	whiteTint = 0;
	colorSaturation = 0;
	colorContrast = 0;
	colorGamma = 0;
	colorGain = Vector3( 0.0f, 0.0f, 0.0f );
	colorOffset = Vector3( 0.0f, 0.0f, 0.0f );
}

void Tr2PostProcessAttributes::FromPostProcess( Tr2PostProcess2* postProcess, PostProcessEnums::Priority inPriority, float inIntensity )
{
	Reset();
	
	intensity = inIntensity;
	priority = inPriority;

	if( !postProcess )
	{
		return;
	}

	if( auto signalLoss = postProcess->GetSignalLoss() )
	{
		signalLossIntensity = Attribute( signalLoss->m_strength, true );
	}
	if( auto bloom = postProcess->GetBloom() )
	{
		bloomBrightness = Attribute( bloom->m_bloomBrightness, true );
		bloomLuminanceScale = Attribute( bloom->m_luminanceScale, true );
		bloomLuminanceThreshold = Attribute( bloom->m_luminanceThreshold, true );
		grimeIntensity = Attribute( bloom->m_grimeWeight, true );
		grimePath = Attribute( bloom->m_grimePath, true );
	}
	if( auto filmGrain = postProcess->GetFilmGrain() )
	{
		filmGrainIntensity = Attribute( filmGrain->m_intensity, true );
		filmGrainSize = Attribute( filmGrain->m_grainSize, true );
		filmGrainDensity = Attribute( filmGrain->m_grainDensity, true );
		filmGrainContrast = Attribute( filmGrain->m_grainContrast, true );
		filmGrainBrightnessModifier = Attribute( filmGrain->m_brightnessModifier, true );
		filmGrainColored = Attribute( filmGrain->m_colored, true );
		filmGrainColorAmount = Attribute( filmGrain->m_colorAmount, true );
	}
	if( auto desaturate = postProcess->GetDesaturate() )
	{
		// negative is desaturation, positive is saturation, so move the zero point to 0.0 from 1.0
		saturation = Attribute( desaturate->m_intensity - 1.0f, true );
	}
	if( auto fade = postProcess->GetFade() )
	{
		fadeIntensity = Attribute( fade->m_intensity, true );
		fadeColor = Attribute( fade->m_color, true );
	}
	if( auto vignette = postProcess->GetVignette() )
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
	if( auto depthOfField = postProcess->GetDepthOfField() )
	{
		depthOfFieldScale = Attribute( depthOfField->m_scale, true );
		depthOfFieldFocalDistance = Attribute( depthOfField->m_focalDistance, true );
		depthOfFieldFocalLength = Attribute( depthOfField->m_focalLength, true );
		depthOfFieldShape = Attribute( depthOfField->m_bokehShape, true );
	}

	auto luts = std::vector<const Tr2PPLutEffect*>();
	postProcess->GetLuts(luts);
	std::sort( luts.begin(), luts.end(), []( const Tr2PPLutEffect* a, const Tr2PPLutEffect* b ) { return a->m_influence < b->m_influence; } );
	for( auto lut : luts )
	{
		// just get the maximum lut intensity
		lutIntensity = Attribute(lut->m_influence, true);
		lutPath = Attribute(lut->m_path, true);
		break;
	}

	whiteTemperature = Attribute( postProcess->m_whiteTemperature, true );
	whiteTint = Attribute( postProcess->m_whiteTint, true );
	colorSaturation = Attribute( postProcess->m_colorSaturation, true );
	colorContrast = Attribute( postProcess->m_colorContrast, true );
	colorGamma = Attribute( postProcess->m_colorGamma, true );
	colorGain = Attribute( postProcess->m_colorGain, true );
	colorOffset = Attribute( postProcess->m_colorOffset, true );
}

void Tr2PostProcessAttributes::ResetWeights()
{
	m_weights.clear();

	if( intensity == 0.0f )
	{
		return;
	}

	if( signalLossIntensity.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::SIGNAL_LOSS_INTENSITY, intensity );
		m_weights.Append( &r );
	}
	if( bloomBrightness.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::BLOOM_BRIGHTNESS, intensity );
		m_weights.Append( &r );
	}
	if( bloomLuminanceThreshold.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::BLOOM_LUMINANCE_THRESHOLD, intensity );
		m_weights.Append( &r );
	}
	if( bloomLuminanceScale.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::BLOOM_LUMINANCE_SCALE, intensity );
		m_weights.Append( &r );
	}
	if( grimeIntensity.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::GRIME_INTENSITY, intensity );
		m_weights.Append( &r );
	}
	if( grimePath.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::GRIME_PATH, intensity );
		m_weights.Append( &r );
	}
	if( filmGrainIntensity.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::FILM_GRAIN_INTENSITY, intensity );
		m_weights.Append( &r );
	}
	if( filmGrainSize.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::FILM_GRAIN_SIZE, intensity );
		m_weights.Append( &r );
	}
	if( filmGrainDensity.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::FILM_GRAIN_DENSITY, intensity );
		m_weights.Append( &r );
	}
	if( filmGrainContrast.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::FILM_GRAIN_CONTRAST, intensity );
		m_weights.Append( &r );
	}
	if( filmGrainBrightnessModifier.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::FILM_GRAIN_BRIGHTNESS_MODIFIER, intensity );
		m_weights.Append( &r );
	}
	if( filmGrainColored.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::FILM_GRAIN_COLORED, intensity );
		m_weights.Append( &r );
	}
	if( filmGrainColorAmount.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::FILM_GRAIN_COLOR_AMOUNT, intensity );
		m_weights.Append( &r );
	}
	if( saturation.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::SATURATION, intensity );
		m_weights.Append( &r );
	}
	if( fadeIntensity.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::FADE_INTENSITY, intensity );
		m_weights.Append( &r );
	}
	if( fadeColor.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::FADE_COLOR, intensity );
		m_weights.Append( &r );
	}
	if( lutIntensity.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::LUT_INTENSITY, intensity );
		m_weights.Append( &r );
	}
	if( lutPath.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::LUT_PATH, intensity );
		m_weights.Append( &r );
	}
	if( vignetteIntensity.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::VIGNETTE_INTENSITY, intensity );
		m_weights.Append( &r );
	}
	if( vignetteOpacity.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::VIGNETTE_OPACITY, intensity );
		m_weights.Append( &r );
	}
	if( vignetteColor.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::VIGNETTE_COLOR, intensity );
		m_weights.Append( &r );
	}
	if( vignetteDetail1Size.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::VIGNETTE_DETAIL1_SIZE, intensity );
		m_weights.Append( &r );
	}
	if( vignetteDetail1Scroll.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::VIGNETTE_DETAIL1_SCROLL, intensity );
		m_weights.Append( &r );
	}
	if( vignetteDetail2Size.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::VIGNETTE_DETAIL2_SIZE, intensity );
		m_weights.Append( &r );
	}
	if( vignetteDetail2Scroll.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::VIGNETTE_DETAIL2_SCROLL, intensity );
		m_weights.Append( &r );
	}
	if( vignetteShapePath.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::VIGNETTE_SHAPE_PATH, intensity );
		m_weights.Append( &r );
	}
	if( vignetteDetailPath.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::VIGNETTE_DETAIL_PATH, intensity );
		m_weights.Append( &r );
	}
	if( vignetteSineFrequency.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::VIGNETTE_SINE_FREQUENCY, intensity );
		m_weights.Append( &r );
	}
	if( vignetteMinSineFrequency.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::VIGNETTE_MIN_SINE_FREQUENCY, intensity );
		m_weights.Append( &r );
	}
	if( vignetteMaxSineFrequency.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::VIGNETTE_MAX_SINE_FREQUENCY, intensity );
		m_weights.Append( &r );
	}
	if( depthOfFieldScale.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::DEPTH_OF_FIELD_SCALE, intensity );
		m_weights.Append( &r );
	}
	if( depthOfFieldFocalDistance.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::DEPTH_OF_FIELD_FOCAL_DISTANCE, intensity );
		m_weights.Append( &r );
	}
	if( depthOfFieldFocalLength.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::DEPTH_OF_FIELD_FOCAL_LENGTH, intensity );
		m_weights.Append( &r );
	}
	if( depthOfFieldShape.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::DEPTH_OF_FIELD_SHAPE, intensity );
		m_weights.Append( &r );
	}
	if( whiteTemperature.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::WHITE_TEMPERATURE, intensity );
		m_weights.Append( &r );
	}
	if( whiteTint.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::WHITE_TINT, intensity );
		m_weights.Append( &r );
	}
	if( colorSaturation.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::COLOR_SATURATION, intensity );
		m_weights.Append( &r );
	}
	if( colorContrast.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::COLOR_CONTRAST, intensity );
		m_weights.Append( &r );
	}
	if( colorGamma.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::COLOR_GAMMA, intensity );
		m_weights.Append( &r );
	}
	if( colorGain.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::COLOR_GAIN, intensity );
		m_weights.Append( &r );
	}
	if( colorOffset.enabled )
	{
		auto r = WeightRow( PostProcessEnums::AttributeType::COLOR_OFFSET, intensity );
		m_weights.Append( &r );
	}
}

void Tr2PostProcessAttributes::UpdateWeightIntensity( std::vector<WeightRow>& currentPriorityWeights, std::vector<WeightRow>& higherPriorityWeights )
{
	// update the total intensity on the current priority weights
	for( const auto& row : currentPriorityWeights )
	{
		if( row.intensity == 0.0f )
		{
			continue;
		}
		for( auto& myRow : m_weights )
		{
			if( row.type == myRow.type )
			{
				myRow.totalIntensity += row.intensity;
			}
		}
	}

	// Now remove the weights of higher priority attributes from the current priority attributes.
	// This will cause lower priority attributes to be less visible when higher priority attributes are present.
	for( const auto& row : higherPriorityWeights )
	{
		if( row.intensity == 0.0f )
		{
			continue;
		}
		for( auto& myRow : m_weights )
		{
			if( row.type == myRow.type )
			{
				myRow.intensity = std::max(0.0f, myRow.intensity - row.intensity);
			}
		}
	}
}

PWeightRowStructureList Tr2PostProcessAttributes::GetWeightTable() const
{
	return m_weights;
}

void Tr2PostProcessAttributes::Merge( const Tr2PostProcessAttributes* other )
{
	for( const auto& row : other->GetWeightTable() )
	{
		if( row.totalIntensity == 0.0f )
		{
			continue;
		}
		switch ( row.type )
		{
		case PostProcessEnums::AttributeType::SIGNAL_LOSS_INTENSITY:
			signalLossIntensity.Blend( other->signalLossIntensity, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::BLOOM_BRIGHTNESS:
			bloomBrightness.Blend( other->bloomBrightness, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::BLOOM_LUMINANCE_THRESHOLD:
			bloomLuminanceThreshold.Blend( other->bloomLuminanceThreshold, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::BLOOM_LUMINANCE_SCALE:
			bloomLuminanceScale.Blend( other->bloomLuminanceScale, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::GRIME_INTENSITY:
			grimeIntensity.Blend( other->grimeIntensity, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::GRIME_PATH:
			grimePath.Blend( other->grimePath, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::FILM_GRAIN_INTENSITY:
			filmGrainIntensity.Blend( other->filmGrainIntensity, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::FILM_GRAIN_SIZE:
			filmGrainSize.Blend( other->filmGrainSize, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::FILM_GRAIN_DENSITY:
			filmGrainDensity.Blend( other->filmGrainDensity, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::FILM_GRAIN_CONTRAST:
			filmGrainContrast.Blend( other->filmGrainContrast, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::FILM_GRAIN_BRIGHTNESS_MODIFIER:
			filmGrainBrightnessModifier.Blend( other->filmGrainBrightnessModifier, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::FILM_GRAIN_COLORED:
			filmGrainColored.Blend( other->filmGrainColored, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::FILM_GRAIN_COLOR_AMOUNT:
			filmGrainColorAmount.Blend( other->filmGrainColorAmount, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::SATURATION:
			saturation.Blend( other->saturation, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::FADE_INTENSITY:
			fadeIntensity.Blend( other->fadeIntensity, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::FADE_COLOR:
			fadeColor.Blend( other->fadeColor, row.intensity, row.totalIntensity );
			break;	
		case PostProcessEnums::AttributeType::LUT_INTENSITY:
		case PostProcessEnums::AttributeType::LUT_PATH:
			prioritizedLuts.insert( { other->lutIntensity.enabled ? other->lutIntensity.value * row.intensity * row.intensity / row.totalIntensity : -1.0f, 
							other->lutPath.enabled ? other->lutPath.value : BlueSharedString( "" ) 
			} );
		break;
		case PostProcessEnums::AttributeType::VIGNETTE_INTENSITY:
			vignetteIntensity.Blend( other->vignetteIntensity, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::VIGNETTE_OPACITY:
			vignetteOpacity.Blend( other->vignetteOpacity, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::VIGNETTE_COLOR:
			vignetteColor.Blend( other->vignetteColor, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::VIGNETTE_DETAIL1_SIZE:
			vignetteDetail1Size.Blend( other->vignetteDetail1Size, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::VIGNETTE_DETAIL1_SCROLL:
			vignetteDetail1Scroll.Blend( other->vignetteDetail1Scroll, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::VIGNETTE_DETAIL2_SIZE:
			vignetteDetail2Size.Blend( other->vignetteDetail2Size, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::VIGNETTE_DETAIL2_SCROLL:
			vignetteDetail2Scroll.Blend( other->vignetteDetail2Scroll, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::VIGNETTE_SHAPE_PATH:
			vignetteShapePath.Blend( other->vignetteShapePath, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::VIGNETTE_DETAIL_PATH:
			vignetteDetailPath.Blend( other->vignetteDetailPath, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::VIGNETTE_SINE_FREQUENCY:
			vignetteSineFrequency.Blend( other->vignetteSineFrequency, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::VIGNETTE_MIN_SINE_FREQUENCY:
			vignetteMinSineFrequency.Blend( other->vignetteMinSineFrequency, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::VIGNETTE_MAX_SINE_FREQUENCY:
			vignetteMaxSineFrequency.Blend( other->vignetteMaxSineFrequency, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::DEPTH_OF_FIELD_SCALE:
			depthOfFieldScale.Blend( other->depthOfFieldScale, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::DEPTH_OF_FIELD_FOCAL_DISTANCE:
			depthOfFieldFocalDistance.Blend( other->depthOfFieldFocalDistance, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::DEPTH_OF_FIELD_FOCAL_LENGTH:
			depthOfFieldFocalLength.Blend( other->depthOfFieldFocalLength, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::DEPTH_OF_FIELD_SHAPE:
			depthOfFieldShape.Blend( other->depthOfFieldShape, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::WHITE_TEMPERATURE:
			whiteTemperature.Blend( other->whiteTemperature, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::WHITE_TINT:
			whiteTint.Blend( other->whiteTint, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::COLOR_SATURATION:
			colorSaturation.Blend( other->colorSaturation, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::COLOR_CONTRAST:
			colorContrast.Blend( other->colorContrast, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::COLOR_GAMMA:
			colorGamma.Blend( other->colorGamma, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::COLOR_GAIN:
			colorGain.Blend( other->colorGain, row.intensity, row.totalIntensity );
			break;
		case PostProcessEnums::AttributeType::COLOR_OFFSET:
			colorOffset.Blend( other->colorOffset, row.intensity, row.totalIntensity );
			break;
		default:
			break;
		}
	}
}

namespace
{

template <typename T>
T Zero( T )
{
	return {};
}

inline Vector2 Zero( Vector2 )
{
	return Vector2( 0, 0 );
}

inline Vector3 Zero( Vector3 )
{
	return Vector3( 0, 0, 0 );
}

inline Vector4 Zero( Vector4 )
{
	return Vector4( 0, 0, 0, 0 );
}

inline Color Zero( Color )
{
	return Color( 0, 0, 0, 0 );
}

template <typename T>
class SumAccumulator
{
public:
	using ResultType = T;

	void Add( const T& value, float weight )
	{
		m_result += value * weight;
	}

	ResultType GetResult() const
	{
		return m_result;
	}

private:
	ResultType m_result = Zero( T() );
};


template <typename T>
class MaxWeightAccumulator
{
public:
	using ResultType = T;

	void Add( const T& value, float weight )
	{
		if( weight > m_weight )
		{
			m_weight = weight;
			m_result = value;
		}
	}

	ResultType GetResult() const
	{
		return m_result;
	}

private:
	ResultType m_result = Zero( T() );
	float m_weight = 0.0f;
};

template <typename T, size_t N>
struct MaxNWeightsAccumulator
{
public:
	struct WeightedValue
	{
		T value = Zero( T() );
		float weight = 0;
	};
	struct ResultType
	{
		std::array<WeightedValue, N> values;
		size_t count = 0;
	};

	void Add( const T& value, float weight )
	{
		for( size_t i = 0; i < m_result.count; ++i )
		{
			if( weight > m_result.values[i].weight )
			{
				for( size_t j = N - 1; j > i; --j )
				{
					m_result.values[j] = m_result.values[j - 1];
				}
				m_result.values[i].value = value;
				m_result.values[i].weight = weight;
				if( m_result.count < N )
				{
					++m_result.count;
				}
				return;
			}
		}
	}

	ResultType GetResult() const
	{
		ResultType result = m_result;
		float totalWeight = 0;
		for( auto& value : result.values )
		{
			totalWeight += value.weight;
		}
		if( totalWeight > 0 )
		{
			for( auto& value : result.values )
			{
				value.weight /= totalWeight;
			}
		}
		return result;
	}

private:
	ResultType m_result;
};


template <typename T>
struct DefaultAccumulator
{
	using Type = SumAccumulator<T>;
};

template <>
struct DefaultAccumulator<BlueSharedString>
{
	using Type = MaxWeightAccumulator<BlueSharedString>;
};

template <>
struct DefaultAccumulator<bool>
{
	using Type = MaxWeightAccumulator<bool>;
};

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

template <typename T>
void EndAttribute( Tr2PostProcessAttributesDebugObserver& observer, const T& value )
{
	auto pyValue = BlueWrapReturnValue( {}, value );
	observer.EndAttribute( pyValue );
	Py_DECREF( pyValue );
}

void EndAttribute( Tr2PostProcessAttributesDebugObserver& observer, const typename MaxNWeightsAccumulator<BlueSharedString, 4>::ResultType& value )
{
	auto pyList = PyList_New( 0 );
	for (size_t i = 0; i < value.count; ++i)
	{
		auto element = PyDict_New();
		auto pyValue = BlueWrapReturnValue( {}, value.values[i].value );
		PyDict_SetItemString( element, "value", pyValue );
		Py_DECREF( pyValue );

		auto pyWeight = BlueWrapReturnValue( {}, value.values[i].weight );
		PyDict_SetItemString( element, "weight", pyWeight );
		Py_DECREF( pyWeight );

		PyList_Append( pyList, element );
		Py_DECREF( element );
	}
	observer.EndAttribute( pyList );
	Py_DECREF( pyList );
}




template <typename T, typename Accumulator = typename DefaultAccumulator<T>::Type>
typename Accumulator::ResultType Accumulate( PostProcess::Attribute<T> Tr2PostProcessAttributes::*attr, const std::vector<Tr2PostProcessAttributes*>& sources, Tr2PostProcessAttributesDebugObserver* observer, Accumulator accumulator = {} )
{
	// sources are assumed to be sorted by priority: high to low

	float remainingWeight = 1.0f;

	if( observer )
	{
		observer->BeginAttribute( GetAttributeName( attr ) );
	}

	for( auto it = begin( sources ); it != end( sources ); )
	{
		// figure out the range of sources with the same priority
		auto jt = it;
		while( jt != end( sources ) && ( *jt )->priority == ( *it )->priority )
		{
			++jt;
		}

		float totalPriorityIntensity = 0.0f;
		for( auto kt = it; kt != jt; ++kt )
		{
			if( ( ( **kt ).*attr ).enabled )
			{
				totalPriorityIntensity += ( **kt ).intensity;
			}
		}
		if( totalPriorityIntensity == 0.0f )
		{
			it = jt;
			continue;
		}

		float normalizationFactor = 1.f / std::max( totalPriorityIntensity, 1.0f ) * remainingWeight;

		for( auto kt = it; kt != jt; ++kt )
		{
			if( ( ( **kt ).*attr ).enabled )
			{
				float weight = ( **kt ).intensity * normalizationFactor;

				accumulator.Add( ( ( **kt ).*attr ).value, weight );
				if( observer )
				{
					observer->Influence( *kt, weight );
				}
			}
		}

		remainingWeight -= totalPriorityIntensity;
		it = jt;
		if( remainingWeight <= 0 )
		{
			break;
		}
	}
	if( observer )
	{
		EndAttribute( *observer, accumulator.GetResult() );
	}
	return accumulator.GetResult();
}

}

void Tr2PostProcessAttributes::MergeInto( Tr2PostProcess2& postprocess, std::vector<Tr2PostProcessAttributes*>& sources, Tr2PostProcessAttributesDebugObserver* debugObserver )
{
	auto signalLossIntensity = Accumulate( &Tr2PostProcessAttributes::signalLossIntensity, sources, debugObserver );
	auto bloomBrightness = Accumulate( &Tr2PostProcessAttributes::bloomBrightness, sources, debugObserver );
	auto bloomLuminanceThreshold = Accumulate( &Tr2PostProcessAttributes::bloomLuminanceThreshold, sources, debugObserver );
	auto bloomLuminanceScale = Accumulate( &Tr2PostProcessAttributes::bloomLuminanceScale, sources, debugObserver );
	auto grimeIntensity = Accumulate( &Tr2PostProcessAttributes::grimeIntensity, sources, debugObserver );
	auto grimePath = Accumulate( &Tr2PostProcessAttributes::grimePath, sources, debugObserver );
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
		postprocess.SetBloom( bloomEffect );
	}
	if( grimeIntensity > 0 )
	{
		Tr2PPBloomEffectPtr bloomEffect;
		if( postprocess.GetBloom() == nullptr )
		{
			bloomEffect.CreateInstance();
		}
		else
		{
			bloomEffect = postprocess.GetBloom();
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
		lutEffect->m_influence = lut.weight;
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
		postprocess.SetDepthOfField( dofEffect );
	}

	postprocess.m_whiteTemperature = whiteTemperature;
	postprocess.m_whiteTint = whiteTint;
	postprocess.m_colorSaturation = colorSaturation;
	postprocess.m_colorContrast = colorContrast;
	postprocess.m_colorGamma = colorGamma;
	postprocess.m_colorGain = colorGain;
	postprocess.m_colorOffset = colorOffset;
}

void Tr2PostProcessAttributes::Blend( std::vector<Tr2PostProcessAttributes*> sources )
{
	m_sources.Clear();
	m_weights.Clear();

	if( sources.size() == 0 )
	{
		return;
	}

	std::vector<std::vector<Tr2PostProcessAttributes*>> priorityList;
	priorityList.resize( PostProcessEnums::Priority::PRIORITY_COUNT );

	std::vector<std::vector<WeightRow>> priorityAccumulatedWeights;
	priorityAccumulatedWeights.resize( PostProcessEnums::Priority::PRIORITY_COUNT );

	// gather all the attributes grouped by priority
	for( auto& source : sources )
	{
		source->ResetWeights();
		m_sources.Append( source );

		if( source->intensity == 0 )
		{
			continue;
		}

		priorityList[source->priority].push_back( source );
		for( auto& row : source->GetWeightTable() )
		{
			priorityAccumulatedWeights[source->priority].push_back( row );
		}
	}		

	std::vector<WeightRow> higherPriorityWeights;

	// modify the weights of the attributes
	for( int priority = PostProcessEnums::Priority::PRIORITY_COUNT - 1; priority >= 0; --priority )
	{
		// go through all the sources in this priority and update the total intensity
		for( auto& source : priorityList[priority] )
		{
			source->UpdateWeightIntensity( priorityAccumulatedWeights[source->priority], higherPriorityWeights );
		}
		higherPriorityWeights.insert(higherPriorityWeights.end(), priorityAccumulatedWeights[priority].begin(), priorityAccumulatedWeights[priority].end());
	}

	for( auto& source : sources )
	{
		// blend the attributes
		Merge( source );
	}
}

void Tr2PostProcessAttributes::ToPostProcess( Tr2PostProcess2Ptr postprocess ) const
{
	postprocess->SetBloom( nullptr );
	postprocess->SetDesaturate( nullptr );
	postprocess->SetFade( nullptr );
	postprocess->SetFilmGrain( nullptr );
	postprocess->SetSignalLoss( nullptr );
	postprocess->SetVignette( nullptr );
	postprocess->SetDepthOfField( nullptr );
	postprocess->ClearLuts();

	if( signalLossIntensity.enabled )
	{
		Tr2PPSignalLossEffectPtr signalLossEffect;
		signalLossEffect.CreateInstance();
		signalLossEffect->m_strength = signalLossIntensity.value;
		postprocess->SetSignalLoss( signalLossEffect );
	}

	if( bloomBrightness.enabled )
	{
		Tr2PPBloomEffectPtr bloomEffect;
		bloomEffect.CreateInstance();
		bloomEffect->m_bloomBrightness = bloomBrightness.value;
		bloomEffect->m_luminanceThreshold = bloomLuminanceThreshold.value;
		bloomEffect->m_luminanceScale = bloomLuminanceScale.value;
		postprocess->SetBloom( bloomEffect );
	}
	if( grimeIntensity.enabled )
	{
		Tr2PPBloomEffectPtr bloomEffect;
		if( postprocess->GetBloom() == nullptr )
		{
			bloomEffect.CreateInstance();
		}
		else
		{
			bloomEffect = postprocess->GetBloom();
		}
		bloomEffect->m_grimeWeight = grimeIntensity.value;
		bloomEffect->m_grimePath = grimePath.value;
		postprocess->SetBloom( bloomEffect );
	}
	if( filmGrainIntensity.enabled )
	{
		Tr2PPFilmGrainEffectPtr filmGrainEffect;
		filmGrainEffect.CreateInstance();
		filmGrainEffect->m_intensity = filmGrainIntensity.value;
		filmGrainEffect->m_grainSize = filmGrainSize.value;
		filmGrainEffect->m_grainDensity = filmGrainDensity.value;
		filmGrainEffect->m_grainContrast = filmGrainContrast.value;
		filmGrainEffect->m_brightnessModifier = filmGrainBrightnessModifier.value;
		filmGrainEffect->m_colored = filmGrainColored.value;
		filmGrainEffect->m_colorAmount = filmGrainColorAmount.value;
		postprocess->SetFilmGrain( filmGrainEffect );
	}

	if( saturation.enabled )
	{
		Tr2PPDesaturateEffectPtr desaturationEffect;
		desaturationEffect.CreateInstance();
		desaturationEffect->m_intensity = saturation.value + 1.0f;
		postprocess->SetDesaturate( desaturationEffect );
	}

	if( fadeIntensity.enabled )
	{
		Tr2PPFadeEffectPtr fadeEffect;
		fadeEffect.CreateInstance();
		fadeEffect->m_intensity = fadeIntensity.value;
		fadeEffect->m_color = fadeColor.value;
		postprocess->SetFade( fadeEffect );
	}

	float lastInfluence = 0.0f;
	BlueSharedString lastPath = BlueSharedString( "" );
	uint16_t count = 0;
	for( const auto& lut : prioritizedLuts )
	{
		if( count == 4 )
		{
			// we only support 4 simultanious luts
			break;
		}
		Tr2PPLutEffectPtr lutEffect;
		lutEffect.CreateInstance();
		lutEffect->m_influence = max( lastInfluence, lut.first );
		lutEffect->m_path = lut.second.empty() ? lastPath : lut.second;

		lastInfluence = lutEffect->m_influence;
		lastPath = lutEffect->m_path;

		postprocess->AddLut( lutEffect );
		++count;
	}

	if( vignetteIntensity.enabled )
	{
		Tr2PPVignetteEffectPtr vignetteEffect;
		vignetteEffect.CreateInstance();
		vignetteEffect->m_intensity = vignetteIntensity.value;
		vignetteEffect->m_opacity = vignetteOpacity.value;
		vignetteEffect->m_color = vignetteColor.value;
		vignetteEffect->m_detail1Size = vignetteDetail1Size.value;
		vignetteEffect->m_detail1Scroll = vignetteDetail1Scroll.value;
		vignetteEffect->m_detail2Size = vignetteDetail2Size.value;
		vignetteEffect->m_detail2Scroll = vignetteDetail2Scroll.value;
		vignetteEffect->m_shapePath = vignetteShapePath.value;
		vignetteEffect->m_detailPath = vignetteDetailPath.value;

		postprocess->SetVignette( vignetteEffect );
	}

	if( depthOfFieldScale.enabled )
	{
		Tr2PPDepthOfFieldEffectPtr dofEffect;
		dofEffect.CreateInstance();
		dofEffect->m_scale = depthOfFieldScale.value;
		dofEffect->m_cocScale = 1.0f;
		dofEffect->m_focalDistance = depthOfFieldFocalDistance.value;
		dofEffect->m_focalLength = depthOfFieldFocalLength.value;
		dofEffect->m_bokehShape = depthOfFieldShape.value;
		postprocess->SetDepthOfField( dofEffect );
	}

	postprocess->m_whiteTemperature = whiteTemperature.value;
	postprocess->m_whiteTint = whiteTint.value;
	postprocess->m_colorSaturation = colorSaturation.value;
	postprocess->m_colorContrast = colorContrast.value;
	postprocess->m_colorGamma = colorGamma.value;
	postprocess->m_colorGain = colorGain.value;
	postprocess->m_colorOffset = colorOffset.value;
}



Tr2PostProcessAttributesDebugObserver::Tr2PostProcessAttributesDebugObserver()
{
	m_debugObject = PyDict_New();
}

Tr2PostProcessAttributesDebugObserver::~Tr2PostProcessAttributesDebugObserver()
{
	Py_XDECREF( m_debugObject );
}

void Tr2PostProcessAttributesDebugObserver::BeginAttribute(const char* name)
{
	m_currentAttribute = PyDict_New();

	auto pyName = BlueWrapReturnValue( {}, name );
	PyDict_SetItemString( m_currentAttribute, "name", pyName );
	Py_XDECREF( pyName );

	m_currentInfluencers = PyList_New( 0 );
	PyDict_SetItemString( m_currentAttribute, "influencers", m_currentInfluencers );
	Py_XDECREF( m_currentInfluencers );


	PyDict_SetItemString( m_debugObject, name, m_currentAttribute );
	Py_XDECREF( m_currentAttribute );
}

void Tr2PostProcessAttributesDebugObserver::Influence(Tr2PostProcessAttributes* attributes, float weight)
{
	auto rec = PyDict_New();

	auto pyAttributes = BlueWrapReturnValue( {}, attributes );
	PyDict_SetItemString( rec, "attributes", pyAttributes );
	Py_XDECREF( pyAttributes );

	auto pyWeight = BlueWrapReturnValue( {}, weight );
	PyDict_SetItemString( rec, "weight", pyWeight );
	Py_XDECREF( pyWeight );

	PyList_Append( m_currentInfluencers, rec );
	Py_XDECREF( rec );
}

void Tr2PostProcessAttributesDebugObserver::EndAttribute( PyObject* value )
{
	if (value && m_currentAttribute)
	{
		PyDict_SetItemString( m_currentAttribute, "value", value );
	}
	m_currentAttribute = nullptr;
	m_currentInfluencers = nullptr;
}

BluePy Tr2PostProcessAttributesDebugObserver::GetDict() const
{
	return BluePy( m_debugObject, true );
}