////////////////////////////////////////////////////////////
//
//    Created:   October 2024
//    Copyright: CCP 2024
//
#pragma once

#include <vector>

/*
Blends values based on priority and intensity.
Expects T to be a struct with the following members:
numeric priority;
numeric intensity;
any value;
The value type needs to support + and * operators.
The sources are assumed to be sorted by priority: high to low.
*/
template <typename T>
decltype( T::value ) PriorityBlend( const std::vector<T>& sources, decltype( T::value ) result = {} )
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
