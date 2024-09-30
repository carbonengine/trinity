////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PostProcessUtils.h"
#include "ITr2PostProcessOwner.h"
#include <algorithm>

namespace PostProcess
{
template<>
void PostProcessAttribute<Tr2Bokeh::Shape>::Blend( const PostProcessAttribute<Tr2Bokeh::Shape>& other, float intensity, float weight )
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
void PostProcessAttribute<BlueSharedString>::Blend( const PostProcessAttribute<BlueSharedString>& other, float intensity, float weight )
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
void PostProcessAttribute<bool>::Blend( const PostProcessAttribute<bool>& other, float intensity, float weight )
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

Attributes::Attributes() :
	intensity(0.0f),
	priority(Priority::MEDIUM_PRIORITY),
	signalLossIntensity( PostProcessAttribute( 0.0f ) ),
	bloomBrightness( PostProcessAttribute( 0.0f ) ),
	bloomLuminanceThreshold( PostProcessAttribute( 0.0f ) ),
	bloomLuminanceScale( PostProcessAttribute( 0.0f ) ),
	grimeIntensity( PostProcessAttribute( 0.0f ) ),
	grimePath( PostProcessAttribute( BlueSharedString( "" ) ) ),
	filmGrainIntensity( PostProcessAttribute( 0.0f ) ),
	filmGrainSize( PostProcessAttribute( 0.0f ) ),
	filmGrainDensity( PostProcessAttribute( 0.0f ) ),
	filmGrainContrast( PostProcessAttribute( 0.0f ) ),
	filmGrainBrightnessModifier( PostProcessAttribute( 0.0f ) ),
	filmGrainColored( PostProcessAttribute( false ) ),
	filmGrainColorAmount( PostProcessAttribute( 0.0f ) ),
	saturation( PostProcessAttribute( 0.0f ) ),
	fadeIntensity( PostProcessAttribute( 0.0f ) ),
	fadeColor( PostProcessAttribute( Color( 0.0f, 0.0f, 0.0f, 1.0f ) ) ),
	lutIntensity( PostProcessAttribute( 0.0f ) ),
	lutPath( PostProcessAttribute( BlueSharedString( "" ) ) ),
	prioritizedLuts( std::set<std::pair<float, BlueSharedString>>() ),
	vignetteIntensity( PostProcessAttribute( 0.0f ) ),
	vignetteOpacity( PostProcessAttribute( 0.0f ) ),
	vignetteColor( PostProcessAttribute( Color( 1.0f, 1.0f, 1.0f, 1.0f ) ) ),
	vignetteDetail1Size( PostProcessAttribute( Vector2( 16.0, 16.0 ) ) ),
	vignetteDetail1Scroll( PostProcessAttribute( Vector2( 0, 0 ) ) ),
	vignetteDetail2Size( PostProcessAttribute( Vector2( 16.0, 16.0 ) ) ),
	vignetteDetail2Scroll( PostProcessAttribute(Vector2( 0, 0 ) ) ),
	vignetteShapePath( PostProcessAttribute( BlueSharedString( "" ) ) ),
	vignetteDetailPath( PostProcessAttribute( BlueSharedString( "" ) ) ),
	vignetteSineFrequency( PostProcessAttribute( 0.0f ) ),
	vignetteMinSineFrequency( PostProcessAttribute( 0.0f ) ),
	vignetteMaxSineFrequency( PostProcessAttribute( 0.0f ) ),
	depthOfFieldScale( PostProcessAttribute( 0.0f ) ),
	depthOfFieldFocalDistance( PostProcessAttribute( 0.0f ) ),
	depthOfFieldFocalLength( PostProcessAttribute( 0.0f ) ),
	depthOfFieldShape( PostProcessAttribute( Tr2Bokeh::Disk ) )
{
}

WeightTable Attributes::GetWeightTable( size_t index, Priority priority )
{
	WeightTable map = WeightTable();

	if( intensity == 0.0f )
	{
		return map;
	}

	if( signalLossIntensity.enabled ){
		map.push_back( { PostProcessAttributeType::SIGNAL_LOSS_INTENSITY, intensity, 0.0f, index, priority } );
	}
	if( bloomBrightness.enabled ){
		map.push_back( { PostProcessAttributeType::BLOOM_BRIGHTNESS, intensity, 0.0f, index, priority } );	
	}
	if( bloomLuminanceThreshold.enabled ){
		map.push_back( { PostProcessAttributeType::BLOOM_LUMINANCE_THRESHOLD, intensity, 0.0f, index, priority } );	
	}
	if( bloomLuminanceScale.enabled ){
		map.push_back( { PostProcessAttributeType::BLOOM_LUMINANCE_SCALE, intensity, 0.0f, index, priority } );	
	}
	if( grimeIntensity.enabled ){
		map.push_back( { PostProcessAttributeType::GRIME_INTENSITY, intensity, 0.0f, index, priority } );	
	}
	if( grimePath.enabled ){
		map.push_back( { PostProcessAttributeType::GRIME_PATH, intensity, 0.0f, index, priority } );	
	}
	if( filmGrainIntensity.enabled ){
		map.push_back( { PostProcessAttributeType::FILM_GRAIN_INTENSITY, intensity, 0.0f, index, priority } );	
	}
	if( filmGrainSize.enabled ){
		map.push_back( { PostProcessAttributeType::FILM_GRAIN_SIZE, intensity, 0.0f, index, priority } );	
	}
	if( filmGrainDensity.enabled ){
		map.push_back( { PostProcessAttributeType::FILM_GRAIN_DENSITY, intensity, 0.0f, index, priority } );
	}
	if( filmGrainContrast.enabled ){
		map.push_back( { PostProcessAttributeType::FILM_GRAIN_CONTRAST, intensity, 0.0f, index, priority } );
	}
	if( filmGrainBrightnessModifier.enabled ){
		map.push_back( { PostProcessAttributeType::FILM_GRAIN_BRIGHTNESS_MODIFIER, intensity, 0.0f, index, priority } );
	}
	if( filmGrainColored.enabled ){
		map.push_back( { PostProcessAttributeType::FILM_GRAIN_COLORED, intensity, 0.0f, index, priority } );
	}
	if( filmGrainColorAmount.enabled ){
		map.push_back( { PostProcessAttributeType::FILM_GRAIN_COLOR_AMOUNT, intensity, 0.0f, index, priority } );
	}
	if( saturation.enabled ){
		map.push_back( { PostProcessAttributeType::SATURATION, intensity, 0.0f, index, priority } );
	}
	if( fadeIntensity.enabled ){
		map.push_back( { PostProcessAttributeType::FADE_INTENSITY, intensity, 0.0f, index, priority } );
	}
	if( fadeColor.enabled ){
		map.push_back( { PostProcessAttributeType::FADE_COLOR, intensity, 0.0f, index, priority } );
	}
	if( lutIntensity.enabled ){
		map.push_back( { PostProcessAttributeType::LUT_INTENSITY, intensity, 0.0f, index, priority } );
	}
	if( lutPath.enabled ){
		map.push_back( { PostProcessAttributeType::LUT_PATH, intensity, 0.0f, index, priority } );
	}
	if( vignetteIntensity.enabled ){
		map.push_back( { PostProcessAttributeType::VIGNETTE_INTENSITY, intensity, 0.0f, index, priority } );
	}
	if( vignetteOpacity.enabled ){
		map.push_back( { PostProcessAttributeType::VIGNETTE_OPACITY, intensity, 0.0f, index, priority } );
	}
	if( vignetteColor.enabled ){
		map.push_back( { PostProcessAttributeType::VIGNETTE_COLOR, intensity, 0.0f, index, priority } );
	}
	if( vignetteDetail1Size.enabled ){
		map.push_back( { PostProcessAttributeType::VIGNETTE_DETAIL1_SIZE, intensity, 0.0f, index, priority } );
	}
	if( vignetteDetail1Scroll.enabled ){
		map.push_back( { PostProcessAttributeType::VIGNETTE_DETAIL1_SCROLL, intensity, 0.0f, index, priority } );
	}
	if( vignetteDetail2Size.enabled ){
		map.push_back( { PostProcessAttributeType::VIGNETTE_DETAIL2_SIZE, intensity, 0.0f, index, priority } );
	}
	if( vignetteDetail2Scroll.enabled ){
		map.push_back( { PostProcessAttributeType::VIGNETTE_DETAIL2_SCROLL, intensity, 0.0f, index, priority } );
	}
	if( vignetteShapePath.enabled ){
		map.push_back( { PostProcessAttributeType::VIGNETTE_SHAPE_PATH, intensity, 0.0f, index, priority } );
	}
	if( vignetteDetailPath.enabled ){
		map.push_back( { PostProcessAttributeType::VIGNETTE_DETAIL_PATH, intensity, 0.0f, index, priority } );
	}
	if( vignetteSineFrequency.enabled ){
		map.push_back( { PostProcessAttributeType::VIGNETTE_SINE_FREQUENCY, intensity, 0.0f, index, priority } );
	}
	if( vignetteMinSineFrequency.enabled ){
		map.push_back( { PostProcessAttributeType::VIGNETTE_MIN_SINE_FREQUENCY, intensity, 0.0f, index, priority } );
	}
	if( vignetteMaxSineFrequency.enabled ){
		map.push_back( { PostProcessAttributeType::VIGNETTE_MAX_SINE_FREQUENCY, intensity, 0.0f, index, priority } );
	}
	if( depthOfFieldScale.enabled ){
		map.push_back( { PostProcessAttributeType::DEPTH_OF_FIELD_SCALE, intensity, 0.0f, index, priority } );
	}
	if( depthOfFieldFocalDistance.enabled ){
		map.push_back( { PostProcessAttributeType::DEPTH_OF_FIELD_FOCAL_DISTANCE, intensity, 0.0f, index, priority } );
	}
	if( depthOfFieldFocalLength.enabled ){
		map.push_back( { PostProcessAttributeType::DEPTH_OF_FIELD_FOCAL_LENGTH, intensity, 0.0f, index, priority } );
	}
	if( depthOfFieldShape.enabled ){
		map.push_back( { PostProcessAttributeType::DEPTH_OF_FIELD_SHAPE, intensity, 0.0f, index, priority } );
	}
	return map;
}

void Attributes::Merge( const Attributes& other, const WeightTable& table )
{
	for( auto& row : table )
	{
		if( row.totalIntensity == 0.0f )
		{
			continue;
		}
		switch ( row.type )
		{
		case PostProcessAttributeType::SIGNAL_LOSS_INTENSITY:
			signalLossIntensity.Blend( other.signalLossIntensity, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::BLOOM_BRIGHTNESS:
			bloomBrightness.Blend( other.bloomBrightness, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::BLOOM_LUMINANCE_THRESHOLD:
			bloomLuminanceThreshold.Blend( other.bloomLuminanceThreshold, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::BLOOM_LUMINANCE_SCALE:
			bloomLuminanceScale.Blend( other.bloomLuminanceScale, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::GRIME_INTENSITY:
			grimeIntensity.Blend( other.grimeIntensity, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::GRIME_PATH:
			grimePath.Blend( other.grimePath, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::FILM_GRAIN_INTENSITY:
			filmGrainIntensity.Blend( other.filmGrainIntensity, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::FILM_GRAIN_SIZE:
			filmGrainSize.Blend( other.filmGrainSize, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::FILM_GRAIN_DENSITY:
			filmGrainDensity.Blend( other.filmGrainDensity, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::FILM_GRAIN_CONTRAST:
			filmGrainContrast.Blend( other.filmGrainContrast, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::FILM_GRAIN_BRIGHTNESS_MODIFIER:
			filmGrainBrightnessModifier.Blend( other.filmGrainBrightnessModifier, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::FILM_GRAIN_COLORED:
			filmGrainColored.Blend( other.filmGrainColored, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::FILM_GRAIN_COLOR_AMOUNT:
			filmGrainColorAmount.Blend( other.filmGrainColorAmount, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::SATURATION:
			saturation.Blend( other.saturation, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::FADE_INTENSITY:
			fadeIntensity.Blend( other.fadeIntensity, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::FADE_COLOR:
			fadeColor.Blend( other.fadeColor, row.intensity, row.totalIntensity );
			break;	
		case PostProcessAttributeType::LUT_INTENSITY:
		case PostProcessAttributeType::LUT_PATH:
			prioritizedLuts.insert( { other.lutIntensity.enabled ? other.lutIntensity.value * row.intensity * row.intensity / row.totalIntensity : -1.0f, 
							other.lutPath.enabled ? other.lutPath.value : BlueSharedString( "" ) 
			} );
		break;
		case PostProcessAttributeType::VIGNETTE_INTENSITY:
			vignetteIntensity.Blend( other.vignetteIntensity, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::VIGNETTE_OPACITY:
			vignetteOpacity.Blend( other.vignetteOpacity, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::VIGNETTE_COLOR:
			vignetteColor.Blend( other.vignetteColor, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::VIGNETTE_DETAIL1_SIZE:
			vignetteDetail1Size.Blend( other.vignetteDetail1Size, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::VIGNETTE_DETAIL1_SCROLL:
			vignetteDetail1Scroll.Blend( other.vignetteDetail1Scroll, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::VIGNETTE_DETAIL2_SIZE:
			vignetteDetail2Size.Blend( other.vignetteDetail2Size, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::VIGNETTE_DETAIL2_SCROLL:
			vignetteDetail2Scroll.Blend( other.vignetteDetail2Scroll, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::VIGNETTE_SHAPE_PATH:
			vignetteShapePath.Blend( other.vignetteShapePath, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::VIGNETTE_DETAIL_PATH:
			vignetteDetailPath.Blend( other.vignetteDetailPath, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::VIGNETTE_SINE_FREQUENCY:
			vignetteSineFrequency.Blend( other.vignetteSineFrequency, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::VIGNETTE_MIN_SINE_FREQUENCY:
			vignetteMinSineFrequency.Blend( other.vignetteMinSineFrequency, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::VIGNETTE_MAX_SINE_FREQUENCY:
			vignetteMaxSineFrequency.Blend( other.vignetteMaxSineFrequency, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::DEPTH_OF_FIELD_SCALE:
			depthOfFieldScale.Blend( other.depthOfFieldScale, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::DEPTH_OF_FIELD_FOCAL_DISTANCE:
			depthOfFieldFocalDistance.Blend( other.depthOfFieldFocalDistance, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::DEPTH_OF_FIELD_FOCAL_LENGTH:
			depthOfFieldFocalLength.Blend( other.depthOfFieldFocalLength, row.intensity, row.totalIntensity );
			break;
		case PostProcessAttributeType::DEPTH_OF_FIELD_SHAPE:
			depthOfFieldShape.Blend( other.depthOfFieldShape, row.intensity, row.totalIntensity );
			break;
		default:
			break;
		}
	}
}

Tr2PostProcess2Ptr Gather( EveComponentRegistry& registry )
{
	auto postprocessOwnerCount = registry.ComponentCount<ITr2PostProcessOwner>();
	if( postprocessOwnerCount == 0 )
	{
		return nullptr;
	}

	// blend postprocesses
	std::vector<std::vector<PostProcess::Attributes>> priorityList;
	std::vector<WeightTable> priorityWeights;

	WeightTable combination  = WeightTable();

	for( int priority = 0; priority < PostProcess::Priority::PRIORITY_COUNT; ++priority )
	{
		priorityList.push_back( std::vector<PostProcess::Attributes>() );
		priorityWeights.push_back( WeightTable() );
	}

	registry.ProcessComponents<ITr2PostProcessOwner>( [&]( ITr2PostProcessOwner* owner ) {
		auto attrs = owner->GetPostProcessAttributes();
		priorityList[attrs.priority].push_back( attrs );
		auto table = attrs.GetWeightTable( priorityList[attrs.priority].size() - 1, attrs.priority );
		priorityWeights[attrs.priority].insert( 
			priorityWeights[attrs.priority].end(), table.begin(), table.end()
		);
	} );		


	// accumulate the intensity weights
	WeightTable lastTable = WeightTable();

	for( int priority = PostProcess::Priority::PRIORITY_COUNT - 1; priority >= 0; --priority )
	{
		auto& weights = priorityWeights[priority];
		for( uint32_t i = 0; i < weights.size(); ++i )
		{
			// accumulate the intensity weights
			WeightRow& current = weights[i];

			// divide by the last weights
			std::vector<PostProcessAttributeType> processedTypes;
			for( auto& last : lastTable )
			{
				if( current.type == last.type && std::find( processedTypes.begin(), processedTypes.end(), last.type ) == processedTypes.end() )
				{
					current.intensity = std::max( current.intensity - last.intensity, 0.0f );
					current.totalIntensity = std::max( current.totalIntensity - last.totalIntensity, 0.0f );
					processedTypes.push_back( last.type );
				}
			}

			current.totalIntensity += current.intensity;

			for( uint32_t j = i+1; j < weights.size(); ++j )
			{
				WeightRow& rowJ = weights[j];
				if( current.type == rowJ.type )
				{
					rowJ.totalIntensity += current.intensity;
					current.totalIntensity += rowJ.intensity;
				}
			}
		}

		lastTable = priorityWeights[priority];
		// add it to the combined weight table
		combination.insert( combination.end(), lastTable.begin(), lastTable.end() );
	}

	Attributes blendedAttributes = Attributes();
	// blend priorities
	for( int priority = PostProcess::Priority::PRIORITY_COUNT - 1; priority >= 0; --priority )
	{
		auto& currentPriorityWeights = priorityWeights[priority];
		for( uint32_t i = 0; i < priorityList[priority].size(); ++i )
		{
			auto checker = [=]( WeightRow r ) -> bool { return r.index == i && r.priority == priority; };

			// get the table for these attributes
			WeightTable table = WeightTable();
			std::copy_if( currentPriorityWeights.begin(), currentPriorityWeights.end(), std::back_inserter(table), checker );
			blendedAttributes.Merge( priorityList[priority][i], table );
		}
	}
	
	Tr2PostProcess2Ptr pp;
	pp.CreateInstance();

	// combine into a Tr2PostPRocess, useless but just getting things to work

	if( blendedAttributes.signalLossIntensity.enabled )
	{
		Tr2PPSignalLossEffectPtr signalLossEffect;
		signalLossEffect.CreateInstance();
		signalLossEffect->m_strength = blendedAttributes.signalLossIntensity.value;
		pp->SetSignalLoss( signalLossEffect );
	}

	if( blendedAttributes.bloomBrightness.enabled )
	{
		Tr2PPBloomEffectPtr bloomEffect;
		bloomEffect.CreateInstance();
		bloomEffect->m_bloomBrightness = blendedAttributes.bloomBrightness.value;
		bloomEffect->m_luminanceThreshold = blendedAttributes.bloomLuminanceThreshold.value;
		bloomEffect->m_luminanceScale = blendedAttributes.bloomLuminanceScale.value;
		pp->SetBloom( bloomEffect );
	}
	if( blendedAttributes.grimeIntensity.enabled )
	{
		Tr2PPBloomEffectPtr bloomEffect;
		if( pp->GetBloom() == nullptr )
		{
			bloomEffect.CreateInstance();
		}
		else
		{
			bloomEffect = pp->GetBloom();
		}
		bloomEffect->m_grimeWeight = blendedAttributes.grimeIntensity.value;
		bloomEffect->m_grimePath = blendedAttributes.grimePath.value;
		pp->SetBloom( bloomEffect );
	}
	if( blendedAttributes.filmGrainIntensity.enabled )
	{
		Tr2PPFilmGrainEffectPtr filmGrainEffect;
		filmGrainEffect.CreateInstance();
		filmGrainEffect->m_intensity = blendedAttributes.filmGrainIntensity.value;
		filmGrainEffect->m_grainSize = blendedAttributes.filmGrainSize.value;
		filmGrainEffect->m_grainDensity = blendedAttributes.filmGrainDensity.value;
		filmGrainEffect->m_grainContrast = blendedAttributes.filmGrainContrast.value;
		filmGrainEffect->m_brightnessModifier = blendedAttributes.filmGrainBrightnessModifier.value;
		filmGrainEffect->m_colored = blendedAttributes.filmGrainColored.value;
		filmGrainEffect->m_colorAmount = blendedAttributes.filmGrainColorAmount.value;
		pp->SetFilmGrain( filmGrainEffect );
	}

	if( blendedAttributes.saturation.enabled )
	{
		Tr2PPDesaturateEffectPtr desaturationEffect;
		desaturationEffect.CreateInstance();
		desaturationEffect->m_intensity = blendedAttributes.saturation.value + 1.0f;
		pp->SetDesaturate( desaturationEffect );
	}

	if( blendedAttributes.fadeIntensity.enabled )
	{
		Tr2PPFadeEffectPtr fadeEffect;
		fadeEffect.CreateInstance();
		fadeEffect->m_intensity = blendedAttributes.fadeIntensity.value;
		fadeEffect->m_color = blendedAttributes.fadeColor.value;
		pp->SetFade( fadeEffect );
	}

	float lastInfluence = 0.0f;
	BlueSharedString lastPath = BlueSharedString("");
	uint16_t count = 0;
	for( const auto& lut: blendedAttributes.prioritizedLuts )
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

		pp->AddLut( lutEffect );
		++count;
	}

	if( blendedAttributes.vignetteIntensity.enabled )
	{
		Tr2PPVignetteEffectPtr vignetteEffect;
		vignetteEffect.CreateInstance();
		vignetteEffect->m_intensity = blendedAttributes.vignetteIntensity.value;
		vignetteEffect->m_opacity = blendedAttributes.vignetteOpacity.value;
		vignetteEffect->m_color = blendedAttributes.vignetteColor.value;
		vignetteEffect->m_detail1Size = blendedAttributes.vignetteDetail1Size.value;
		vignetteEffect->m_detail1Scroll = blendedAttributes.vignetteDetail1Scroll.value;
		vignetteEffect->m_detail2Size = blendedAttributes.vignetteDetail2Size.value;
		vignetteEffect->m_detail2Scroll = blendedAttributes.vignetteDetail2Scroll.value;
		vignetteEffect->m_shapePath = blendedAttributes.vignetteShapePath.value;
		vignetteEffect->m_detailPath = blendedAttributes.vignetteDetailPath.value;

		pp->SetVignette( vignetteEffect );
	}

	if( blendedAttributes.depthOfFieldScale.enabled )
	{
		Tr2PPDepthOfFieldEffectPtr dofEffect;
		dofEffect.CreateInstance();
		dofEffect->m_scale = blendedAttributes.depthOfFieldScale.value;
		dofEffect->m_cocScale = 1.0f;
		dofEffect->m_focalDistance = blendedAttributes.depthOfFieldFocalDistance.value;
		dofEffect->m_focalLength = blendedAttributes.depthOfFieldFocalLength.value;
		dofEffect->m_bokehShape = blendedAttributes.depthOfFieldShape.value;
		pp->SetDepthOfField( dofEffect );
	}

	return pp;
}

} // namespace PostProcess