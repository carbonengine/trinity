// Copyright © 2026 CCP ehf.

#pragma once
#ifndef TriCurveSampler_h
#define TriCurveSampler_h

#include <ITriFunction.h>
#include "ITr2SampledCurve.h"

// Samples a typed curve's value at any time without moving the curve's current
// value - so a curve shared with others, or one that fires events as it
// advances, is unaffected.
//
// Keyed curves (ITr2SampledCurve) are sampled through key segment caches of the
// sampler's own, so a binding sampling a shared curve at its offset time finds
// its keys as quickly as if it had a copy of the curve, and leaves the curve's
// own caches to the curve's own UpdateValue. Other curves are sampled through
// their function interface.
//
// Holds a raw curve pointer found once, so it is only valid while the source
// object is, and must be found again whenever the source changes.
class TriCurveSampler
{
public:
	// A sampler for source when valueEntry is its value attribute - a scalar,
	// vector, quaternion or color curve - or an empty sampler otherwise.
	static TriCurveSampler Find( IRoot* source, const Be::VarEntry* valueEntry );

	explicit operator bool() const
	{
		return m_kind != Kind::None;
	}

	// The curve's value at time, laid out from x as in its currentValue: a scalar
	// fills x, a vector xyz, a quaternion or color all four.
	Vector4 Sample( double time );

private:
	enum class Kind : uint8_t
	{
		None,
		Keyed,
		Scalar,
		Vector,
		Quaternion,
		Color
	};

	Kind m_kind = Kind::None;
	Tr2CurveChannelSegments m_segments = {};
	union
	{
		ITr2SampledCurve* keyed;
		ITriScalarFunction* scalar;
		ITriVectorFunction* vector;
		ITriQuaternionFunction* quaternion;
		ITriColorFunction* color;
	} m_curve = { nullptr };
};

#endif // TriCurveSampler_h
