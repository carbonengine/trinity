// Copyright © 2026 CCP ehf.

#pragma once
#ifndef ITr2SampledCurve_h
#define ITr2SampledCurve_h

#include <array>

// One key segment cache per channel of a curve, owned by whoever samples it.
// Curves built from scalar channel curves (Tr2CurveVector3, Tr2CurveColor,
// Tr2CurveEulerRotation) use element i for channel i; others use the first.
using Tr2CurveChannelSegments = std::array<size_t, 4>;

// A keyed curve that can be sampled at any time without moving its current
// value, looking its keys up from the caller's segment caches. A caller
// sampling a shared curve at times of its own - a time offset binding - so
// finds its keys as quickly as the curve's own UpdateValue does, without the
// two evicting each other's cached segments.
BLUE_INTERFACE( ITr2SampledCurve ) :
	public IRoot
{
	// The value at time, laid out from x as in the curve's currentValue: a
	// scalar fills x, a vector xyz, a quaternion or color all four.
	virtual Vector4 SampleValue( double time, Tr2CurveChannelSegments& segmentCaches ) const = 0;
};

#endif // ITr2SampledCurve_h
