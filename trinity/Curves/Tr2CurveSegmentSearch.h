// Copyright © 2026 CCP ehf.

#pragma once
#ifndef Tr2CurveSegmentSearch_h
#define Tr2CurveSegmentSearch_h

#include <algorithm>
#include <array>

// One key segment cache per channel, for sampling curves built from up to four
// scalar channel curves (Tr2CurveVector3, Tr2CurveColor, Tr2CurveEulerRotation)
// at times of the caller's own. Channel i of the curve uses element i.
using Tr2CurveChannelSegments = std::array<size_t, 4>;

// Index i of the key segment [keys[i], keys[i + 1]) that contains t, for a curve
// with at least two keys. Falls back to the last segment when none contains t.
//
// cachedSegment is a hint, updated to the segment found: curves sampled at
// steadily moving times find their segment at or next to the cached one. Each
// caller sampling at its own times - a curve's own UpdateValue, or a binding
// with a time offset - should keep its own hint, or they keep moving each
// other's and every lookup has to search.
template <typename Keys>
size_t FindCurveSegment( const Keys& keys, float t, size_t& cachedSegment )
{
	// This runs for every curve evaluation, so the cache checks are written out
	// rather than going through helpers that unoptimised builds would call.
	const size_t count = keys.size();
	const size_t cached = cachedSegment;

	if( cached + 1 < count )
	{
		const float start = keys[cached].m_time;
		const float end = keys[cached + 1].m_time;
		if( t >= start && t < end )
		{
			return cached;
		}
		if( t >= end && cached + 2 < count && t < keys[cached + 2].m_time )
		{
			return cachedSegment = cached + 1;
		}
		if( t < start && cached > 0 && t >= keys[cached - 1].m_time )
		{
			return cachedSegment = cached - 1;
		}
	}

	// Keys are kept sorted by time, so binary search for the segment.
	const auto next = std::upper_bound( keys.begin(), keys.end(), t, []( float time, const auto& key ) { return time < key.m_time; } );
	const size_t found = size_t( next - keys.begin() );
	if( found > 0 && found < count && t >= keys[found - 1].m_time && t < keys[found].m_time )
	{
		return cachedSegment = found - 1;
	}

	// Keys edited out of order without being re-sorted: try every segment.
	for( size_t i = 0; i + 1 < count; ++i )
	{
		if( t >= keys[i].m_time && t < keys[i + 1].m_time )
		{
			return cachedSegment = i;
		}
	}
	return cachedSegment = count - 2;
}

#endif // Tr2CurveSegmentSearch_h
