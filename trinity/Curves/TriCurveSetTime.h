// Copyright © 2026 CCP ehf.

#pragma once
#ifndef TriCurveSetTime_h
#define TriCurveSetTime_h

#include <cmath>

// Wraps time into [rangeMin, rangeMax), so a time before the start of a looped
// range comes round from its end instead of walking off it. A degenerate range
// collapses to its start.
inline double WrapTimeIntoRange( double time, double rangeMin, double rangeMax )
{
	const double span = rangeMax - rangeMin;
	if( span <= 0.0 )
	{
		return rangeMin;
	}

	double wrapped = std::fmod( time - rangeMin, span );
	if( wrapped < 0.0 )
	{
		wrapped += span;
	}
	return wrapped + rangeMin;
}

// The time a curve set is applying its bindings at, together with the looped
// range it plays in, so that a binding can sample its source at an offset from
// that time without knowing how the curve set keeps time.
struct TriCurveSetTime
{
	double time;
	bool looped;
	double rangeMin;
	double rangeMax;

	// The curve set's time shifted back by offset seconds, wrapped into the
	// looped range if there is one.
	double Shifted( float offset ) const
	{
		const double shifted = time - (double)offset;
		return looped ? WrapTimeIntoRange( shifted, rangeMin, rangeMax ) : shifted;
	}
};

#endif // TriCurveSetTime_h
