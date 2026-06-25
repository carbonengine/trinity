// Copyright © 2024 CCP ehf.

#pragma once

#include "Eve/EveComponentRegistry.h"
#include <vector>


namespace PriorityBlend
{
template <typename T>
struct Attribute
{
	T value;
	bool enabled;

	Attribute( T value ) :
		value( value ),
		enabled( false )
	{
	}

	Attribute( T value, bool enabled ) :
		value( value ),
		enabled( enabled )
	{
	}
};


template <typename T>
T Zero( T )
{
	return {};
}

Vector2 Zero( Vector2 );
Vector3 Zero( Vector3 );
Vector4 Zero( Vector4 );
Color Zero( Color );
bool Zero( bool );


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
			std::sort( m_result.values.begin(), m_result.values.end(), []( const WeightedValue& a, const WeightedValue& b ) { return a.weight > b.weight; } );
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
class AttributesDebugObserver
{
public:
	AttributesDebugObserver()
	{
		m_debugObject = PyDict_New();
	}
	~AttributesDebugObserver()
	{
		Py_XDECREF( m_debugObject );
	}


	void BeginAttribute( const char* name )
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

	void Influence( T* attributes, float weight )
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

	void EndAttribute( PyObject* value )
	{
		if( value && m_currentAttribute )
		{
			PyDict_SetItemString( m_currentAttribute, "value", value );
		}
		m_currentAttribute = nullptr;
		m_currentInfluencers = nullptr;
	}

	BluePy GetDict() const
	{
		return BluePy( m_debugObject, true );
	}

private:
	PyObject* m_debugObject = nullptr;
	PyObject* m_currentAttribute = nullptr;
	PyObject* m_currentInfluencers = nullptr;
};


template <typename T, typename V>
void EndAttribute( AttributesDebugObserver<T>& observer, const V& value )
{
	auto pyValue = BlueWrapReturnValue( {}, value );
	observer.EndAttribute( pyValue );
	Py_DECREF( pyValue );
}

template <typename T>
void EndAttribute( AttributesDebugObserver<T>& observer, const typename MaxNWeightsAccumulator<BlueSharedString, 4>::ResultType& value )
{
	auto pyList = PyList_New( 0 );
	for( size_t i = 0; i < value.count; ++i )
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

template <typename T, typename A, typename Accumulator = typename DefaultAccumulator<A>::Type>
typename Accumulator::ResultType Accumulate( const Attribute<A> T::* attr, const std::vector<T*>& sources, AttributesDebugObserver<T>* observer, const char* attributeName, Accumulator accumulator = {} )
{
	// sources are assumed to be sorted by priority: high to low

	float remainingWeight = 1.0f;

	if( observer )
	{
		observer->BeginAttribute( attributeName );
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

template <typename T, typename A, typename Accumulator = typename DefaultAccumulator<A>::Type>
typename Accumulator::ResultType Accumulate( const Attribute<A> T::* attr, const std::vector<T*>& sources, Accumulator accumulator = {} )
{
	return Accumulate( attr, sources, (AttributesDebugObserver<T>*)nullptr, nullptr, accumulator );
}
}




/*
Simplified priority blend that blends values based on priority and intensity.
Expects T to be a struct with the following members:
- numeric priority;
- numeric intensity;
- any value;
The value type needs to support + and * operators.
The sources are assumed to be sorted by priority: high to low.
*/
template <typename T>
decltype( T::value ) SimplePriorityBlend( const std::vector<T>& sources, decltype( T::value ) result = {} )
{
	// sources are assumed to be sorted by priority: high to low
	float remainingWeight = 1.0f;

	for( auto it = begin( sources ); it != end( sources ); )
	{
		// figure out the range of sources with the same priority
		auto jt = it;
		while( jt != end( sources ) && jt->priority == it->priority )
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
			result = result + kt->value * weight;
		}

		remainingWeight -= totalPriorityIntensity;
		it = jt;
		if( remainingWeight <= 0 )
		{
			break;
		}
	}
	return result;
}
