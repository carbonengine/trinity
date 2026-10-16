// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierShake.h"
#include "Include/TriMath.h"

EveChildModifierShake::EveChildModifierShake( IRoot* lockobj ) :
	m_frequency( 1.f ),
	m_amplitude( 1.f ),
	m_timeOffset( 0.f )
{
}

EveChildModifierShake::~EveChildModifierShake()
{
}

Matrix EveChildModifierShake::ApplyTransform( const Matrix& transform, size_t, const Float4x3* ) const
{
	// Absolute time only — never accumulated — so the object cannot drift.
	const double time = TimeAsDouble( BeOS->GetCurrentFrameTime() ) + double( m_timeOffset );
	const int octaves = 8;

	// One independent noise channel per axis, each within (-amplitude, +amplitude).
	// Channels are offset after the frequency scale so they stay decorrelated at any frequency,
	// and the offsets differ in their fractional part so the axes never cross zero together.
	auto noise = [&]( double channelOffset ) -> float {
		return m_amplitude * float( PerlinNoise1D( time * double( m_frequency ) + channelOffset, 2.0, 2.0, octaves ) );
	};

	Matrix result = transform;
	result.GetTranslation() += Vector3( noise( 17.31 ), noise( 53.77 ), noise( 89.13 ) );
	return result;
}
