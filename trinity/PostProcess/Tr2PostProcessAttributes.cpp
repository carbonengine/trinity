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
		default:
			break;
		}
	}
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
}