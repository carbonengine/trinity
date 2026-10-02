// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildModifierShake.h"
#include "Include/TriMath.h"

EveChildModifierShake::EveChildModifierShake( IRoot* lockobj ) :
	m_frequency( 1.f ),
	m_amplitude( 1.f )
{
}

EveChildModifierShake::~EveChildModifierShake()
{
}

Matrix EveChildModifierShake::ApplyTransform( const Matrix& transform, size_t, const Float4x3* ) const
{
	// Randomized direction on the sphere + signed radius from multi-octave noise.
	// Radius crosses through zero so the offset reaches every signed axis from the origin.
	// Absolute time only — never accumulated — so the object cannot drift.
	const double time = TimeAsDouble( BeOS->GetCurrentFrameTime() );
	const double seed = double( uintptr_t( this ) & 0xfff );
	const int octaves = 8;

	auto unitNoise = [&]( double phaseOffset ) -> float {
		const float n = float( PerlinNoise1D( ( time + seed + phaseOffset ) * double( m_frequency ), 2.0, 2.0, octaves ) );
		// Fold any Perlin range into [0, 1] via a bounded sin so direction sampling stays well-defined.
		return 0.5f * ( sinf( n * 17.13f ) + 1.f );
	};

	const float u = unitNoise( 1.1 );
	const float v = unitNoise( 10.1 );
	const float w = unitNoise( 18.3 );

	// Uniform-ish direction on the unit sphere (theta azimuth, phi from cos mapping).
	const float theta = u * ( 2.f * TRI_PI );
	const float cosPhi = 2.f * v - 1.f;
	const float sinPhi = sqrtf( cosPhi * cosPhi > 1.f ? 0.f : ( 1.f - cosPhi * cosPhi ) );

	// Signed radius in [-amplitude, +amplitude] so motion passes through the origin
	// into the opposite octant (up/down, left/right, forward/back).
	const float radius = m_amplitude * sinf( w * ( 2.f * TRI_PI ) );

	const Vector3 offset(
		radius * sinPhi * cosf( theta ),
		radius * cosPhi,
		radius * sinPhi * sinf( theta ) );

	Matrix result = transform;
	result.GetTranslation() += offset;
	return result;
}
