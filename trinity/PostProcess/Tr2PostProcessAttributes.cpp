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

Tr2PostProcessAttributes::Tr2PostProcessAttributes( IRoot* lockobj ) :
	intensity(0.0f),
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
	depthOfFieldForegroundBlurNeeded( Attribute( false ) )
{
}

Tr2PostProcessAttributes::~Tr2PostProcessAttributes()
{
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

inline bool Zero( bool )
{
	return false;
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
		// if the value is already in the values, update the weight
		for( size_t i = 0; i < m_result.count; ++i )
		{
			if( m_result.values[i].value == value )
			{
				m_result.values[i].weight += weight;
				return;
			}
		}


		if( m_result.count < N )
		{
			m_result.values[m_result.count].value = value;
			m_result.values[m_result.count].weight = weight;
			++m_result.count;
			std::sort( m_result.values.begin(), m_result.values.end(), []( const WeightedValue& a, const WeightedValue& b ) { return a.weight > b.weight; });
		}
		else
		{
			// insert the new value in the correct place
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

	postprocess.m_whiteTemperature = whiteTemperature;
	postprocess.m_whiteTint = whiteTint;
	postprocess.m_colorSaturation = colorSaturation;
	postprocess.m_colorContrast = colorContrast;
	postprocess.m_colorGamma = colorGamma;
	postprocess.m_colorGain = colorGain;
	postprocess.m_colorOffset = colorOffset;
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
	// important to nuke any old values
	Reset();

	intensity = inIntensity;
	priority = inPriority;

	if( !postProcess )
	{
		return;
	}

	if( auto signalLoss = postProcess->GetSignalLoss() )
	{
		if( signalLoss->IsActive() )
		{
			signalLossIntensity = Attribute( signalLoss->m_strength, true );
		}
	}
	
	if( auto bloom = postProcess->GetBloom() )
	{
		if( bloom->IsActive() )
		{
			bloomBrightness = Attribute( bloom->m_bloomBrightness, true );
			bloomLuminanceScale = Attribute( bloom->m_luminanceScale, true );
			bloomLuminanceThreshold = Attribute( bloom->m_luminanceThreshold, true );
			grimeIntensity = Attribute( bloom->m_grimeWeight, true );
			grimePath = Attribute( bloom->m_grimePath, true );
		}
	}
	if( auto filmGrain = postProcess->GetFilmGrain() )
	{
		if( filmGrain->IsActive() )
		{
			filmGrainIntensity = Attribute( filmGrain->m_intensity, true );
			filmGrainSize = Attribute( filmGrain->m_grainSize, true );
			filmGrainDensity = Attribute( filmGrain->m_grainDensity, true );
			filmGrainContrast = Attribute( filmGrain->m_grainContrast, true );
			filmGrainBrightnessModifier = Attribute( filmGrain->m_brightnessModifier, true );
			filmGrainColored = Attribute( filmGrain->m_colored, true );
			filmGrainColorAmount = Attribute( filmGrain->m_colorAmount, true );
		}
	}
	if( auto desaturate = postProcess->GetDesaturate() )
	{
		if( desaturate->IsActive() )
		{ 
			// negative is desaturation, positive is saturation, so move the zero point to 0.0 from 1.0
			saturation = Attribute( desaturate->m_intensity - 1.0f, true );
		}
	}
	if( auto fade = postProcess->GetFade() )
	{
		if( fade->IsActive() )
		{ 
			fadeIntensity = Attribute( fade->m_intensity, true );
			fadeColor = Attribute( fade->m_color, true );
		}

	}
	if( auto vignette = postProcess->GetVignette() )
	{
		if( vignette->IsActive() )
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

	}
	if( auto depthOfField = postProcess->GetDepthOfField() )
	{
		if( depthOfField->IsActive() )
		{ 
			depthOfFieldScale = Attribute( depthOfField->m_scale, true );
			depthOfFieldFocalDistance = Attribute( depthOfField->m_focalDistance, true );
			depthOfFieldFocalLength = Attribute( depthOfField->m_focalLength, true );
			depthOfFieldShape = Attribute( depthOfField->m_bokehShape, true );
		}

	}

	auto luts = std::vector<const Tr2PPLutEffect*>();
	postProcess->GetLuts( luts );
	std::sort( luts.begin(), luts.end(), []( const Tr2PPLutEffect* a, const Tr2PPLutEffect* b ) { return a->m_influence < b->m_influence; } );
	for( auto lut : luts )
	{
		// just get the maximum lut intensity
		lutIntensity = Attribute( lut->m_influence, true );
		lutPath = Attribute( lut->m_path, true );
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