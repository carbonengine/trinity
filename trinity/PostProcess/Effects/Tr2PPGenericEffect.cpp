// Copyright © 2026 CCP ehf.

#include "Tr2PPGenericEffect.h"
#include "../Tr2PostProcessAttributes.h"
#include "../Shader/Tr2Effect.h"
#include "../Shader/Tr2Shader.h"
#include "../Shader/Parameter/Tr2FloatParameter.h"
#include "../Shader/Parameter/Tr2Vector2Parameter.h"
#include "../Shader/Parameter/Tr2Vector3Parameter.h"
#include "../Shader/Parameter/Tr2Vector4Parameter.h"


namespace
{

struct EffectInstance
{
	Tr2PPGenericEffect* effect;
	PostProcessEnums::Priority priority;
	float intensity;
};

template <typename Param, typename Value>
struct BlendableParameter
{
	BluePtr<Param> parameter;
	Value value;
};

struct EffectBucket
{
	EffectBucket() = default;
	explicit EffectBucket( const Tr2PPGenericEffect& effect )
	{
		for( auto& param : effect.blendableFloatParameters )
		{
			floatValues.push_back( param.defaultValue );
		}
		for( auto& param : effect.blendableVector2Parameters )
		{
			vector2Values.push_back( param.defaultValue );
		}
		for( auto& param : effect.blendableVector3Parameters )
		{
			vector3Values.push_back( param.defaultValue );
		}
		for( auto& param : effect.blendableVector4Parameters )
		{
			vector4Values.push_back( param.defaultValue );
		}
	}

	void Accumulate( const Tr2PPGenericEffect& effect, float weight )
	{
		auto AccumulateValues = [&]( auto& params, auto& values ) {
			for( auto& param : params )
			{
				auto value = ( param.parameter->GetValue() - param.defaultValue ) * weight;
				values[&param - params.data()] += value;
			}
		};
		AccumulateValues( effect.blendableFloatParameters, floatValues );
		AccumulateValues( effect.blendableVector2Parameters, vector2Values );
		AccumulateValues( effect.blendableVector3Parameters, vector3Values );
		AccumulateValues( effect.blendableVector4Parameters, vector4Values );
	}

	std::vector<EffectInstance> effects;
	std::vector<float> floatValues;
	std::vector<Vector2> vector2Values;
	std::vector<Vector3> vector3Values;
	std::vector<Vector4> vector4Values;
};

std::vector<EffectBucket> PopulateBuckets( Tr2PostProcess2& postprocess, std::vector<Tr2PostProcessAttributes*>& sources )
{
	std::vector<EffectBucket> buckets;

	if( auto deprecated = postprocess.GetGenericEffectIfAvailable() )
	{
		if( deprecated->IsActive() )
		{
			deprecated->UpdateEffectParameters();
			if( deprecated->IsValid() )
			{
				auto& bucket = buckets.emplace_back( *deprecated );
				bucket.effects.push_back( EffectInstance{ deprecated, PostProcessEnums::SCENE_DEFAULT_PRIORITY, 1.0f } );
			}
		}
	}

	for( auto& src : sources )
	{
		if( src->intensity <= 0 )
		{
			continue;
		}

		for( auto& effect : src->genericEffects )
		{
			if( !effect || !effect->IsActive() )
			{
				continue;
			}
			effect->UpdateEffectParameters();
			if( !effect->IsValid() )
			{
				continue;
			}
			auto found = std::find_if( buckets.begin(), buckets.end(), [&]( const EffectBucket& bucket ) {
				return effect->CanBeMerged( *bucket.effects.front().effect );
			} );
			if( found == buckets.end() )
			{
				buckets.emplace_back( *effect );
				found = std::prev( buckets.end() );
			}
			found->effects.push_back( EffectInstance{ effect, src->priority, src->intensity } );
		}
	}
	return buckets;
}

bool IsBlendableParameter( const Tr2EffectConstant& constant, const Tr2Shader& shader )
{
	if( constant.type != Tr2EffectConstant::FLOAT || constant.elements > 1 || constant.dimension < 1 || constant.dimension > 4 )
	{
		return false;
	}
	if( auto annotations = shader.GetParameterAnnotations( constant.name.c_str() ) )
	{
		auto found = std::find_if( annotations->begin(), annotations->end(), []( const Tr2EffectParameterAnnotation& annotation ) {
			return strcmp( annotation.name, Tr2PPGenericEffect::IsBlendableAnnotationName ) == 0;
		} );
		return found != annotations->end() && found->type == Tr2EffectParameterAnnotation::BOOL && found->boolValue;
	}
	return false;
}

}

Tr2PPGenericEffect::Tr2PPGenericEffect( IRoot* )
{
}

void Tr2PPGenericEffect::UpdateEffectParameters()
{
	if( !m_effect )
	{
		return;
	}
	auto shader = m_effect->GetShaderStateInterface();
	if( !shader )
	{
		return;
	}
	auto hash = m_effect->GetHashValue();
	if( hash == m_lastHashValue && m_lastShader == shader )
	{
		return;
	}
	m_lastHashValue = hash;
	m_lastShader = shader;
	m_nonBlendableHash = m_effect->GetNonBlendableHashValue();
	blendableFloatParameters.clear();
	blendableVector2Parameters.clear();
	blendableVector3Parameters.clear();
	blendableVector4Parameters.clear();
	m_requiresSourceTexture = false;

	auto& desc = shader->GetEffectDescription();
	for( auto& technique : desc.techniques )
	{
		for( auto& pass : technique.passes )
		{
			for( auto& stage : pass.stageInputs )
			{
				for( auto& constant : stage.constants )
				{
					if( !IsBlendableParameter( constant, *shader ) )
					{
						continue;
					}
					AddBlendableParameter( constant, reinterpret_cast<const uint8_t*>( stage.constantValues ) );
				}
				if( !m_requiresSourceTexture )
				{
					m_requiresSourceTexture = find_if( stage.resources.begin(), stage.resources.end(), []( const auto& texture ) { return strcmp( texture.second.name, SourceTextureName ) == 0; } ) != stage.resources.end();
				}
			}
		}
	}
}

bool Tr2PPGenericEffect::IsValid() const
{
	return m_effect && m_effect->GetShaderStateInterface();
}

bool Tr2PPGenericEffect::CanBeMerged( const Tr2PPGenericEffect& other ) const
{
	if( m_executionSlot != other.m_executionSlot || m_quality != other.m_quality )
	{
		return false;
	}
	if( m_effect->GetShaderStateInterface() != other.m_effect->GetShaderStateInterface() )
	{
		return false;
	}
	return m_nonBlendableHash == other.m_nonBlendableHash;
}

bool Tr2PPGenericEffect::RequiresSourceTexture() const
{
	return m_requiresSourceTexture;
}

void Tr2PPGenericEffect::AddBlendableParameter( const Tr2EffectConstant& constant, const uint8_t* defaultValues )
{
	auto AddParameter = [&]( auto& blendableParameters ) {
		using Element = typename std::decay_t<decltype( blendableParameters )>::value_type;
		using ParamType = typename Element::ParameterType;
		using ValueType = typename Element::ValueType;
		ValueType defaultValue;
		memcpy( &defaultValue, defaultValues + constant.offset, sizeof( ValueType ) );
		BluePtr<ParamType> param = BlueCastPtr( m_effect->GetParameterByName( constant.name.c_str() ) );
		if( !param )
		{
			m_effect->SetParameter( constant.name, defaultValue );
			param = BlueCastPtr( m_effect->GetParameterByName( constant.name.c_str() ) );
			if( !param )
			{
				return;
			}
		}
		blendableParameters.push_back( { param, defaultValue } );
	};

	switch( constant.dimension )
	{
	case 1:
		AddParameter( blendableFloatParameters );
		break;
	case 2:
		AddParameter( blendableVector2Parameters );
		break;
	case 3:
		AddParameter( blendableVector3Parameters );
		break;
	case 4:
		AddParameter( blendableVector4Parameters );
		break;
	default:
		break;
	}
}

void Tr2AccumulatedGenericEffects::GenericEffectInstance::SetParameters()
{
	for( auto& param : effect->blendableFloatParameters )
	{
		auto value = param.parameter->GetValue();
		std::swap( floatValues[&param - effect->blendableFloatParameters.data()], value );
		param.parameter->SetValue( value );
	}
	for( auto& param : effect->blendableVector2Parameters )
	{
		auto value = param.parameter->GetValue();
		std::swap( vector2Values[&param - effect->blendableVector2Parameters.data()], value );
		param.parameter->SetValue( value );
	}
	for( auto& param : effect->blendableVector3Parameters )
	{
		auto value = param.parameter->GetValue();
		std::swap( vector3Values[&param - effect->blendableVector3Parameters.data()], value );
		param.parameter->SetValue( value );
	}
	for( auto& param : effect->blendableVector4Parameters )
	{
		auto value = param.parameter->GetValue();
		std::swap( vector4Values[&param - effect->blendableVector4Parameters.data()], value );
		param.parameter->SetValue( value );
	}
}

void Tr2AccumulatedGenericEffects::GenericEffectInstance::RestoreParameters()
{
	SetParameters();
}




void AccumulateGenericEffects( Tr2PostProcess2& postprocess, std::vector<Tr2PostProcessAttributes*>& sources )
{
	std::vector<EffectBucket> buckets = PopulateBuckets( postprocess, sources );

	for( auto& bucket : buckets )
	{
		float remainingWeight = 1.0f;

		std::sort( bucket.effects.begin(), bucket.effects.end(), []( const EffectInstance& a, const EffectInstance& b ) {
			return a.priority > b.priority;
		} );

		for( auto it = begin( bucket.effects ); it != end( bucket.effects ); )
		{
			// figure out the range of sources with the same priority
			auto jt = it;
			while( jt != end( bucket.effects ) && ( *jt ).priority == ( *it ).priority )
			{
				++jt;
			}

			float totalPriorityIntensity = 0.0f;
			for( auto kt = it; kt != jt; ++kt )
			{
				totalPriorityIntensity += kt->intensity;
			}
			if( totalPriorityIntensity == 0.0f )
			{
				it = jt;
				continue;
			}

			float normalizationFactor = 1.f / std::max( totalPriorityIntensity, 1.0f ) * remainingWeight;

			for( auto kt = it; kt != jt; ++kt )
			{
				float weight = kt->intensity * normalizationFactor;
				bucket.Accumulate( *kt->effect, weight );
			}

			remainingWeight -= totalPriorityIntensity;
			it = jt;
			if( remainingWeight <= 0 )
			{
				break;
			}
		}
	}

	for( auto& effects : postprocess.m_genericEffects.effects )
	{
		effects.clear();
	}

	for( auto& bucket : buckets )
	{
		Tr2AccumulatedGenericEffects::GenericEffectInstance instance;
		instance.effect = bucket.effects.front().effect;
		instance.floatValues = std::move( bucket.floatValues );
		instance.vector2Values = std::move( bucket.vector2Values );
		instance.vector3Values = std::move( bucket.vector3Values );
		instance.vector4Values = std::move( bucket.vector4Values );
		postprocess.m_genericEffects.effects[instance.effect->m_executionSlot].push_back( instance );
	}
	for( auto& each : postprocess.m_genericEffects.effects )
	{
		std::sort( each.begin(), each.end(), []( const auto& a, const auto& b ) {
			return a.effect->m_order < b.effect->m_order;
		} );
	}
}